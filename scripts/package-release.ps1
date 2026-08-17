[CmdletBinding()]
param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build\release"
$packageDirectory = Join-Path $projectRoot "dist\401_demo"
$application = Join-Path $buildDirectory "401_demo.exe"

# CMake is often launched outside Qt Creator, so load the same VS2017 x86
# environment used by the Qt 5.14.2 kit before configuring the project.
$vcVars = "D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
if (-not (Test-Path -LiteralPath $vcVars)) {
    throw "The VS2017 x86 build environment was not found: $vcVars"
}
$buildEnvironment = & cmd.exe /d /s /c ('call "' + $vcVars + '" x86 >nul && set')
foreach ($entry in $buildEnvironment) {
    if ($entry -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}
$ninjaDirectory = "D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
if (-not (Test-Path -LiteralPath (Join-Path $ninjaDirectory "ninja.exe"))) {
    throw "Ninja was not found: $ninjaDirectory"
}
$env:Path = "$ninjaDirectory;$env:Path"

if (Test-Path -LiteralPath $packageDirectory) {
    if (-not $Clean) {
        throw "Release directory already exists: $packageDirectory. Run again with -Clean to recreate it."
    }
    Remove-Item -LiteralPath $packageDirectory -Recurse -Force
}

cmake --preset release
if ($LASTEXITCODE -ne 0) { throw "Release configuration failed." }

cmake --build --preset release
if ($LASTEXITCODE -ne 0) { throw "Release build failed." }

if (-not (Test-Path -LiteralPath $application)) {
    throw "Release executable was not produced: $application"
}

New-Item -ItemType Directory -Path $packageDirectory | Out-Null
Copy-Item -LiteralPath $application -Destination $packageDirectory

$qtDeploy = "D:\QT5\5.14.2\msvc2017\bin\windeployqt.exe"
if (-not (Test-Path -LiteralPath $qtDeploy)) {
    throw "windeployqt was not found: $qtDeploy"
}
& $qtDeploy --release --no-translations --compiler-runtime --dir $packageDirectory (Join-Path $packageDirectory "401_demo.exe")
if ($LASTEXITCODE -ne 0) { throw "Qt runtime deployment failed." }

# The hardware SDK DLLs and the RFID Drivers directory are placed beside the
# executable by CMake during the Release build. They are runtime requirements,
# unlike .pdb files and test executables, which are intentionally not packaged.
Get-ChildItem -LiteralPath $buildDirectory -Filter *.dll -File | Copy-Item -Destination $packageDirectory
$drivers = Join-Path $buildDirectory "Drivers"
if (Test-Path -LiteralPath $drivers) {
    Copy-Item -LiteralPath $drivers -Destination $packageDirectory -Recurse
}

Write-Host "Release package created: $packageDirectory"
Write-Host "Main application: $(Join-Path $packageDirectory '401_demo.exe')"
