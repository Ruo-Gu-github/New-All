param(
    [switch]$Prefer64Bit
)

$ErrorActionPreference = 'Stop'

function Write-Result([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "MSBuild not found at: $path"
    }
    Write-Output $path
    exit 0
}

# 1) Respect explicit override.
if ($env:MSBUILD_EXE -and (Test-Path -LiteralPath $env:MSBUILD_EXE)) {
    Write-Result $env:MSBUILD_EXE
}

# 2) Try PATH.
try {
    $cmd = Get-Command msbuild.exe -ErrorAction Stop
    if ($cmd -and (Test-Path -LiteralPath $cmd.Source)) {
        Write-Result $cmd.Source
    }
} catch {
    # ignore
}

# 3) Common install locations (including this workspace's unusual VS "18" layout).
$programFiles = ${env:ProgramFiles}
$programFilesX86 = ${env:ProgramFiles(x86)}

$candidates = @(
    (Join-Path $programFiles 'Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'),
    (Join-Path $programFiles 'Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe'),
    (Join-Path $programFiles 'Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'),
    (Join-Path $programFiles 'Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'),
    (Join-Path $programFilesX86 'Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'),
    (Join-Path $programFilesX86 'Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe'),
    (Join-Path $programFilesX86 'Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe')
)

foreach ($candidate in $candidates) {
    if ($candidate -and (Test-Path -LiteralPath $candidate)) {
        Write-Result $candidate
    }
}

# 4) Last resort: light recursive search inside Visual Studio folders only.
$roots = @(
    (Join-Path $programFiles 'Microsoft Visual Studio'),
    (Join-Path $programFilesX86 'Microsoft Visual Studio')
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

foreach ($root in $roots) {
    $found = Get-ChildItem -Path $root -Filter MSBuild.exe -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\MSBuild\\Current\\Bin(\\amd64)?\\MSBuild\.exe$' } |
        Select-Object -First 1

    if ($found) {
        Write-Result $found.FullName
    }
}

Write-Error "MSBuild.exe not found. Install Visual Studio (C++ workload) or Visual Studio Build Tools, or set MSBUILD_EXE to the full path."
exit 1
