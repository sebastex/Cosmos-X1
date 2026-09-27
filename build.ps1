# Builds Cosmos X1 with g++ (MinGW). Use CMakeLists.txt instead where CMake is available.
param([switch]$DebugBuild, [string]$Out = 'cosmos_x1.exe')  # -Out: build beside a running search

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
New-Item -ItemType Directory -Force (Join-Path $root 'build') | Out-Null

$opt = if ($DebugBuild) { @('-O0', '-g') } else { @('-O3', '-march=native') }
$sources = Get-ChildItem (Join-Path $root 'src') -Filter *.cpp | ForEach-Object { $_.FullName }
$exe = Join-Path (Join-Path $root 'build') $Out

& g++ -std=c++20 @opt -fopenmp -static -Wall -Wextra -I (Join-Path $root 'include') @sources -o $exe
if ($LASTEXITCODE -ne 0) { throw 'build failed' }
Write-Host "Built $exe"
