$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$private = Get-ChildItem -Recurse -Force -ErrorAction SilentlyContinue | Where-Object { $_.Name -match 'poc|reference\.patch|BUGS\.md' }
if ($private) { throw "private material found" }
if (-not (Select-String -Path .clusterfuzzlite/project.yaml -Pattern '^language: c\+\+$' -Quiet)) { throw "language missing" }
Write-Output "preflight passed"
