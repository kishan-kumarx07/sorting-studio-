$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $projectRoot 'build'
$exePath = Join-Path $buildDir 'smart_sorting_studio.exe'
$port = 8080

$existingServer = Get-Process -Name 'smart_sorting_studio' -ErrorAction SilentlyContinue
if ($existingServer) {
    throw 'A Smart Sorting Studio process is already running. Stop it manually before starting another instance.'
}

cmake -S $projectRoot -B $buildDir
cmake --build $buildDir

if (-not (Test-Path $exePath)) {
    throw "Build failed: executable not found at $exePath"
}

Start-Process -FilePath $exePath -ArgumentList $port -WorkingDirectory $projectRoot | Out-Null
Start-Process "http://localhost:$port" | Out-Null

Write-Host "Smart Sorting Studio started on http://localhost:$port"
