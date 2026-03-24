param(
    [string]$Message = ("Update " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss"))
)

$ErrorActionPreference = "Stop"

Set-Location $PSScriptRoot

git add -A

$changes = git status --porcelain
if (-not $changes) {
    Write-Host "No changes to commit."
    exit 0
}

git commit -m $Message
git push
