[CmdletBinding()]
param(
    [switch]$IncludeClangCL,
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')]
    [string]$Configuration = 'Debug',
    [string]$OutputRoot = '',
    [ValidateRange(1, 64)]
    [int]$Jobs = 2
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# PowerShell 7.3+ can convert native failures into exceptions before we record
# their exit code. Check every native exit code ourselves on both PS 5.1 and 7.
if (Test-Path variable:PSNativeCommandUseErrorActionPreference) {
    $PSNativeCommandUseErrorActionPreference = $false
}

$sourceRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $sourceRoot 'artifacts/windows-host'
}
$runName = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 8)
$runRoot = Join-Path ([IO.Path]::GetFullPath($OutputRoot)) $runName
$null = New-Item -ItemType Directory -Path $runRoot -Force
$results = New-Object 'System.Collections.Generic.List[object]'
$report = [ordered]@{
    schema_version = 1
    scope = 'host-only synthetic tests; no GPU or EFI validation'
    started_utc = (Get-Date).ToUniversalTime().ToString('o')
    finished_utc = $null
    configuration = $Configuration
    status = 'failed'
    error = $null
    toolchains = $results
}

function Invoke-LoggedNative {
    param(
        [string]$Executable,
        [string[]]$Arguments,
        [string]$LogPath,
        [string]$Stage,
        [System.Collections.Generic.List[object]]$Steps
    )
    # Argument arrays preserve spaces in paths. This display line is evidence,
    # not a shell command that is evaluated again.
    ($Executable + ' ' + ($Arguments -join ' ')) | Out-File -FilePath $LogPath -Encoding utf8
    $oldPreference = $ErrorActionPreference
    try {
        # Windows PowerShell represents redirected native stderr as ErrorRecord
        # objects; let the process finish so its actual exit code is preserved.
        $ErrorActionPreference = 'Continue'
        $global:LASTEXITCODE = $null
        & $Executable @Arguments 2>&1 | Tee-Object -FilePath $LogPath -Append | Out-Host
        $nativeCode = $global:LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $oldPreference
    }
    $Steps.Add([pscustomobject]@{
        stage = $Stage
        exit_code = $nativeCode
        log = $LogPath
    })
    if ($null -eq $nativeCode) {
        throw "$Stage did not return a native exit code. See $LogPath"
    }
    if ($nativeCode -ne 0) {
        throw "$Stage failed with exit code $nativeCode. See $LogPath"
    }
}

try {
    if ($env:OS -ne 'Windows_NT') {
        throw 'Run this script on Windows with Visual Studio 2022 C++ build tools.'
    }
    $cmake = (Get-Command cmake -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
    $ctest = (Get-Command ctest -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
    $toolSteps = New-Object 'System.Collections.Generic.List[object]'
    Invoke-LoggedNative $cmake @('--version') (Join-Path $runRoot 'cmake-version.log') 'cmake-version' $toolSteps
    Invoke-LoggedNative $ctest @('--version') (Join-Path $runRoot 'ctest-version.log') 'ctest-version' $toolSteps

    $toolsets = @('v143')
    if ($IncludeClangCL) { $toolsets += 'ClangCL' }
    foreach ($toolset in $toolsets) {
        $toolRoot = Join-Path $runRoot $toolset
        $buildRoot = Join-Path $toolRoot 'build'
        $null = New-Item -ItemType Directory -Path $toolRoot -Force
        $steps = New-Object 'System.Collections.Generic.List[object]'
        $result = [ordered]@{
            toolset = $toolset
            architecture = 'x64'
            configuration = $Configuration
            configure_status = 'not_run'
            build_status = 'not_run'
            test_status = 'not_run'
            status = 'failed'
            error = $null
            steps = $steps
        }
        $results.Add($result)
        $stage = 'configure'
        try {
            Invoke-LoggedNative $cmake @('-S', $sourceRoot, '-B', $buildRoot, '-G', 'Visual Studio 17 2022', '-A', 'x64', '-T', $toolset, '-DSANITIZE=OFF', '-DFUZZ=OFF') (Join-Path $toolRoot 'configure.log') $stage $steps
            $result.configure_status = 'passed'
            # CMake's compiler identification records the selected compiler even
            # when cl.exe is not on the calling shell's PATH.
            $compilerFiles = Get-ChildItem -Path (Join-Path $buildRoot 'CMakeFiles') -Filter CMakeCCompiler.cmake -Recurse
            if (@($compilerFiles).Count -eq 0) { throw 'CMake compiler identification file is missing.' }
            $compilerFiles | Select-String -Pattern '^set\(CMAKE_C_(COMPILER |COMPILER_ID |COMPILER_VERSION |COMPILER_ARCHITECTURE_ID |STANDARD_COMPUTED_DEFAULT )' |
                ForEach-Object { $_.Line } | Out-File -FilePath (Join-Path $toolRoot 'compiler.log') -Encoding utf8

            $stage = 'build'
            Invoke-LoggedNative $cmake @('--build', $buildRoot, '--config', $Configuration, '--parallel', "$Jobs") (Join-Path $toolRoot 'build.log') $stage $steps
            $result.build_status = 'passed'
            $stage = 'test'
            Invoke-LoggedNative $ctest @('--test-dir', $buildRoot, '-C', $Configuration, '--output-on-failure', '--no-tests=error', '--output-junit', (Join-Path $toolRoot 'ctest.xml')) (Join-Path $toolRoot 'test.log') $stage $steps
            $result.test_status = 'passed'
            $result.status = 'passed'
        }
        catch {
            $result[($stage + '_status')] = 'failed'
            $result.error = $_.Exception.Message
            Write-Warning $result.error
        }
    }
    if (@($results | Where-Object { $_.status -ne 'passed' }).Count -eq 0) {
        $report.status = 'passed'
    }
}
catch {
    $report.error = $_.Exception.Message
    Write-Warning $report.error
}
finally {
    $report.finished_utc = (Get-Date).ToUniversalTime().ToString('o')
    $report | ConvertTo-Json -Depth 8 | Out-File -FilePath (Join-Path $runRoot 'result.json') -Encoding utf8
    Write-Host "Host test result: $($report.status). Artifacts: $runRoot"
}

if ($report.status -ne 'passed') { exit 1 }
exit 0
