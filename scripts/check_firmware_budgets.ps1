param(
  [string[]]$Environment,
  [switch]$Build
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$budgetPath = Join-Path $PSScriptRoot 'firmware_budgets.json'
$budgets = Get-Content -LiteralPath $budgetPath -Raw | ConvertFrom-Json
if ($Environment.Count -gt 0) {
  $selected = @($budgets | Where-Object { $Environment -contains $_.environment })
  $missing = @($Environment | Where-Object { $_ -notin $selected.environment })
  if ($missing.Count -gt 0) { throw "Unknown budget environment: $($missing -join ', ')" }
} else {
  $selected = @($budgets)
}

$pio = (Get-Command platformio -ErrorAction SilentlyContinue).Source
if (-not $pio) {
  $userPio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\platformio.exe'
  if (Test-Path -LiteralPath $userPio) { $pio = $userPio }
}
if (-not $pio) { throw 'platformio executable not found' }

$failed = $false
foreach ($budget in $selected) {
  Write-Host "== $($budget.device) [$($budget.environment)] =="
  if ($Build) {
    $output = & $pio run -e $budget.environment 2>&1 | ForEach-Object {
      Write-Host $_
      $_.ToString()
    }
    if ($LASTEXITCODE -ne 0) {
      $failed = $true
      Write-Error "Build failed: $($budget.environment)"
      continue
    }
  } else {
    $output = & $pio run -e $budget.environment -t size 2>&1 | ForEach-Object {
      Write-Host $_
      $_.ToString()
    }
    if ($LASTEXITCODE -ne 0) {
      $failed = $true
      Write-Error "Size check failed: $($budget.environment)"
      continue
    }
  }

  $text = $output -join "`n"
  $ram = [regex]::Matches($text, 'RAM:\s+\[[^\]]*\]\s+[0-9.]+%\s+\(used\s+(\d+)\s+bytes') | Select-Object -Last 1
  $flash = [regex]::Matches($text, 'Flash:\s+\[[^\]]*\]\s+[0-9.]+%\s+\(used\s+(\d+)\s+bytes') | Select-Object -Last 1
  if (-not $ram -or -not $flash) {
    $failed = $true
    Write-Error "Could not parse size output: $($budget.environment)"
    continue
  }

  $ramUsed = [int64]$ram.Groups[1].Value
  $flashUsed = [int64]$flash.Groups[1].Value
  $ramOk = $ramUsed -le [int64]$budget.ram_bytes
  $flashOk = $flashUsed -le [int64]$budget.flash_bytes
  Write-Host ("RAM   {0,8} / {1,8}  {2}" -f $ramUsed, $budget.ram_bytes, $(if ($ramOk) { 'OK' } else { 'OVER' }))
  Write-Host ("Flash {0,8} / {1,8}  {2}" -f $flashUsed, $budget.flash_bytes, $(if ($flashOk) { 'OK' } else { 'OVER' }))
  if (-not $ramOk -or -not $flashOk) { $failed = $true }
}

if ($failed) { exit 1 }
