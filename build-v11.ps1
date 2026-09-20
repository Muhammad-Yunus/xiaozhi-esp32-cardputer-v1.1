# Build script for M5Stack Cardputer v1.1
# This script ensures CLEAN build every time to prevent configuration issues

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "M5Stack Cardputer v1.1 - Clean Build" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Configuration
$idfPath = "C:\Users\Asus\esp\v5.5.2\esp-idf"
$projectPath = "C:\D\DOCUMENT_BCK\GitHub\xiaozhi-esp32-cardputer-v1.1"

Set-Location $projectPath

# Step 1: CLEAN BUILD - Remove old config and build artifacts
Write-Host "[1/5] Cleaning previous build artifacts..." -ForegroundColor Yellow
if (Test-Path "sdkconfig") {
    Remove-Item "sdkconfig" -Force -ErrorAction SilentlyContinue
    Write-Host "  -> Deleted sdkconfig" -ForegroundColor Green
}
if (Test-Path "build") {
    Remove-Item "build" -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "  -> Deleted build/" -ForegroundColor Green
}

# Step 2: Set up ESP-IDF environment
Write-Host "[2/5] Setting up ESP-IDF environment..." -ForegroundColor Yellow
$env:IDF_PATH = $idfPath
& "$idfPath\export.ps1" 2>$null

# Step 3: Verify sdkconfig.defaults has correct settings
Write-Host "[3/5] Verifying configuration..." -ForegroundColor Yellow
$defaultsContent = Get-Content "sdkconfig.defaults.esp32s3" -Raw
$checks = @(
    @{ Name="8MB Flash"; Pattern="CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y" },
    @{ Name="No PSRAM"; Pattern="CONFIG_SPIRAM=n" },
    @{ Name="Cardputer V11"; Pattern="CONFIG_BOARD_TYPE_M5STACK_CARDPUTER_V11=y" },
    @{ Name="English Lang"; Pattern="CONFIG_LANGUAGE_EN_US=y" },
    @{ Name="8MB Partition"; Pattern="partitions/v2/8m.csv" }
)

$allPassed = $true
foreach ($check in $checks) {
    if ($defaultsContent -match $check.Pattern) {
        Write-Host "  -> OK: $($check.Name)" -ForegroundColor Green
    } else {
        Write-Host "  -> FAIL: $($check.Name) - Pattern '$($check.Pattern)' not found!" -ForegroundColor Red
        $allPassed = $false
    }
}

if (-not $allPassed) {
    Write-Host ""
    Write-Host "ERROR: Configuration verification failed!" -ForegroundColor Red
    Write-Host "Please check sdkconfig.defaults.esp32s3" -ForegroundColor Red
    exit 1
}

# Step 4: Build firmware
Write-Host "[4/5] Building firmware..." -ForegroundColor Yellow
idf.py set-target esp32s3
idf.py build

# Step 5: Verify output files
Write-Host "[5/5] Verifying build output..." -ForegroundColor Yellow
$outputFiles = @(
    "build/xiaozhi.bin",
    "build/generated_assets.bin",
    "build/ota_data_initial.bin"
)

$buildOk = $true
foreach ($file in $outputFiles) {
    if (Test-Path $file) {
        $sizeMB = [math]::Round((Get-Item $file).Length / 1MB, 2)
        Write-Host "  -> OK: $file ($sizeMB MB)" -ForegroundColor Green
    } else {
        Write-Host "  -> MISSING: $file" -ForegroundColor Red
        $buildOk = $false
    }
}

if (-not $buildOk) {
    Write-Host ""
    Write-Host "ERROR: Build verification failed!" -ForegroundColor Red
    exit 1
}

# Summary
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Build Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Flash command:" -ForegroundColor Yellow
Write-Host "  idf.py -p COM14 flash" -ForegroundColor White
Write-Host ""
Write-Host "Monitor command:" -ForegroundColor Yellow
Write-Host "  idf.py monitor" -ForegroundColor White
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
