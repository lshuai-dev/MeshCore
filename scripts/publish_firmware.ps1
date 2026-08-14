[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)]
  [ValidatePattern('^v\d+(?:\.\d+)+(?:[-_][0-9A-Za-z.-]+)?$')]
  [string] $Version,

  [ValidateSet('all', 'companion', 'lvgl-companion', 'repeater', 'room-server')]
  [string] $Type = 'all',

  [switch] $DisableDebug,
  [switch] $Force
)

$ErrorActionPreference = 'Stop'

function Get-ToolPath([string[]] $Candidates) {
  foreach ($candidate in $Candidates) {
    $command = Get-Command $candidate -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
  }
  return $null
}

function Invoke-BashBuild([string] $BuildCommand, [string] $PythonPath, [string] $PioPath, [string] $Python3ShimPath) {
  $bash = Get-ToolPath @('bash.exe', 'bash')
  if (-not $bash) {
    throw 'Git Bash was not found. Install Git for Windows or add bash.exe to PATH.'
  }

  $repoForBash = (Get-Location).Path.Replace('\', '/')
  if ($repoForBash -match '^([A-Za-z]):(.*)$') {
    $repoForBash = '/' + $Matches[1].ToLowerInvariant() + $Matches[2]
  }
  $pythonShimForBash = (Split-Path -Parent $Python3ShimPath).Replace('\', '/')
  $pioForBash = $PioPath.Replace('\', '/')
  if ($pythonShimForBash -match '^([A-Za-z]):(.*)$') {
    $pythonShimForBash = '/' + $Matches[1].ToLowerInvariant() + $Matches[2]
  }
  if ($pioForBash -match '^([A-Za-z]):(.*)$') {
    $pioForBash = '/' + $Matches[1].ToLowerInvariant() + $Matches[2]
  }

  $bashScript = @"
set -e
cd '$repoForBash'
export PATH='$pythonShimForBash':'$pioForBash':/usr/bin:/bin:/cmd:\$PATH
export FIRMWARE_VERSION='$Version'
$(if ($DisableDebug) { 'export DISABLE_DEBUG=1' } else { 'unset DISABLE_DEBUG' })
/usr/bin/bash build.sh $BuildCommand
"@

  & $bash -lc $bashScript
  if ($LASTEXITCODE -ne 0) {
    throw "Firmware build failed with exit code $LASTEXITCODE."
  }
}

$repo = (Get-Location).Path
$versionDirectory = Join-Path $repo $Version
$python = Get-ToolPath @('python.exe', 'python')
$pio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts'
$pioExecutable = Join-Path $pio 'pio.exe'

if (-not $python) { throw 'Python was not found. Install Python or PlatformIO.' }
if (-not (Test-Path -LiteralPath $pioExecutable)) {
  $pioCommand = Get-ToolPath @('pio.exe', 'pio')
  if ($pioCommand) { $pio = Split-Path -Parent $pioCommand }
}
if (-not (Test-Path -LiteralPath (Join-Path $pio 'pio.exe'))) {
  throw 'PlatformIO was not found. Install PlatformIO and its Python environment.'
}

$python3ShimDirectory = Join-Path ([System.IO.Path]::GetTempPath()) 'meshcore-publish-python3'
New-Item -ItemType Directory -Path $python3ShimDirectory -Force | Out-Null
$python3ShimPath = Join-Path $python3ShimDirectory 'python3'
$pythonForBash = $python.Replace('\', '/')
if ($pythonForBash -match '^([A-Za-z]):(.*)$') {
  $pythonForBash = '/' + $Matches[1].ToLowerInvariant() + $Matches[2]
}
$shimContent = @(
  '#!/usr/bin/env bash'
  ('exec ''' + $pythonForBash + ''' "$@"')
)
Set-Content -LiteralPath $python3ShimPath -Encoding ascii -Value $shimContent
if (Test-Path -LiteralPath (Join-Path $python3ShimDirectory 'python3.exe')) {
  Remove-Item -LiteralPath (Join-Path $python3ShimDirectory 'python3.exe') -Force
}

if ((Test-Path -LiteralPath $versionDirectory) -and -not $Force) {
  throw "Release directory already exists: $versionDirectory. Use -Force to replace it."
}
if (Test-Path -LiteralPath $versionDirectory) {
  Remove-Item -LiteralPath $versionDirectory -Recurse -Force
}

switch ($Type) {
  'all'         { $buildCommand = 'build-firmwares' }
  'companion'   { $buildCommand = 'build-companion-firmwares' }
  'lvgl-companion' {
    $configPath = Join-Path ([System.IO.Path]::GetTempPath()) 'meshcore-pio-config.json'
    & (Join-Path $pio 'pio.exe') project config --json-output | Out-File -Encoding utf8 $configPath
    $targets = @(& $python -c "import json,sys; d=json.load(open(sys.argv[1],encoding='utf-8-sig')); [print(s[4:]) for s,o in d if s.startswith('env:') and s[4:].endswith(('_companion_radio_usb','_companion_radio_ble')) and any(k=='build_flags' and 'HELTEC_MESH_UI' in (v if isinstance(v,str) else ' '.join(v)) for k,v in o)]" $configPath)
    if ($targets.Count -eq 0) { throw 'No LVGL companion targets were found.' }
    $buildCommand = 'build-firmware ' + ($targets -join ' ')
    Write-Host "LVGL companion targets: $($targets -join ', ')"
  }
  'repeater'    { $buildCommand = 'build-repeater-firmwares' }
  'room-server' { $buildCommand = 'build-room-server-firmwares' }
}

Write-Host "Building $Type firmware for $Version..."
Invoke-BashBuild $buildCommand $python $pio $python3ShimPath

if (-not (Test-Path -LiteralPath (Join-Path $repo 'out'))) {
  throw 'Build completed without an out directory.'
}

New-Item -ItemType Directory -Path $versionDirectory | Out-Null
Get-ChildItem -LiteralPath (Join-Path $repo 'out') -File | Move-Item -Destination $versionDirectory
$artifactCount = @(Get-ChildItem -LiteralPath $versionDirectory -File).Count
if ($artifactCount -eq 0) { throw 'No firmware artifacts were produced.' }

Write-Host "Published $artifactCount artifacts to $versionDirectory"
