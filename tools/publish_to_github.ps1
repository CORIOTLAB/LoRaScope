<#
.SYNOPSIS
  Publishes the LoRaScope repository to GitHub from Windows PowerShell.

.DESCRIPTION
  Initialises git (if needed), runs the local checks, makes the first commit and
  pushes to GitHub. If the GitHub CLI (gh) is installed and authenticated it also
  creates the repository; otherwise create an empty repository on github.com first
  and pass its URL with -RemoteUrl.

.EXAMPLE
  .\tools\publish_to_github.ps1 -Owner "my-org" -Name "LoRaScope"

.EXAMPLE
  .\tools\publish_to_github.ps1 -Owner "my-org" -RemoteUrl "https://github.com/my-org/LoRaScope.git"
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)][string]$Owner,
  [string]$Name = "LoRaScope",
  [string]$RemoteUrl = "",
  [ValidateSet("private", "public")][string]$Visibility = "private",
  [string]$Description = "Open modular hardware platform for joint LoRaWAN coverage mapping and GNSS positioning-quality assessment (firmware, 3D parts, datasets, figure scripts)."
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

function Test-Tool($tool) { return [bool](Get-Command $tool -ErrorAction SilentlyContinue) }

if (-not (Test-Tool git)) { throw "git is not installed. Install it from https://git-scm.com/download/win and reopen PowerShell." }

Write-Host "== 0/6  CI workflow" -ForegroundColor Cyan
if (Test-Path "tools/ci.yml") {
  New-Item -ItemType Directory -Force -Path ".github/workflows" | Out-Null
  Move-Item -Force "tools/ci.yml" ".github/workflows/ci.yml"
  Write-Host "Installed .github/workflows/ci.yml"
}

Write-Host "== 1/6  Local checks" -ForegroundColor Cyan
if (Test-Tool python) { python tools/check_data.py; if ($LASTEXITCODE -ne 0) { throw "Dataset check failed." } }
else { Write-Warning "python not found: skipping dataset check." }
if (Test-Tool node) { node tests/decoder.test.js; if ($LASTEXITCODE -ne 0) { throw "Decoder test failed." } }
else { Write-Warning "node not found: skipping decoder test." }

Write-Host "== 2/6  Placeholders" -ForegroundColor Cyan
foreach ($f in @("README.md", "CITATION.cff")) {
  $text = Get-Content -Raw -Encoding UTF8 $f
  $text = $text -replace "OWNER/LoRaScope", "$Owner/$Name"
  [System.IO.File]::WriteAllText((Join-Path $Root $f), $text, (New-Object System.Text.UTF8Encoding($false)))
}

Write-Host "== 3/6  Git repository" -ForegroundColor Cyan
if (-not (Test-Path ".git")) { git init -b main | Out-Null }
$userName = git config user.name
$userEmail = git config user.email
if (-not $userName)  { $userName  = Read-Host "git user.name  (your full name)";  git config user.name  $userName }
if (-not $userEmail) { $userEmail = Read-Host "git user.email (use your GitHub noreply address to keep your email private)"; git config user.email $userEmail }

if (git ls-files | Select-String "A152_CONFIG\.h$") { throw "A152_CONFIG.h is tracked; remove it before publishing." }

Write-Host "== 4/6  Commit" -ForegroundColor Cyan
git add --all
git status --short
git diff --cached --quiet
if ($LASTEXITCODE -ne 0) {
  git commit -m "Initial release: firmware, hardware, datasets and analysis scripts" | Out-Null
} else { Write-Host "Nothing new to commit." }

Write-Host "== 5/6  Remote" -ForegroundColor Cyan
$hasOrigin = (git remote) -contains "origin"
if (-not $hasOrigin) {
  if ($RemoteUrl) {
    git remote add origin $RemoteUrl
  } elseif (Test-Tool gh) {
    gh auth status
    if ($LASTEXITCODE -ne 0) { throw "Run 'gh auth login' first." }
    gh repo create "$Owner/$Name" "--$Visibility" --description $Description --source . --remote origin
  } else {
    throw "Install the GitHub CLI (https://cli.github.com) or create an empty repository on github.com and pass -RemoteUrl."
  }
}

Write-Host "== 6/6  Push" -ForegroundColor Cyan
git branch -M main
git push -u origin main
Write-Host "Done. Repository: https://github.com/$Owner/$Name" -ForegroundColor Green
if ($Visibility -eq "private") { Write-Host "The repository is PRIVATE. Confirm licences and the missing BOM details with the authors before making it public." -ForegroundColor Yellow }
