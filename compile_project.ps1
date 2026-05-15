# PowerShell script to compile the STM32F411 TAS5815 project using Keil MDK-ARM AC6 compiler

# Define paths
$UV4Path = "C:\Keil_v5\UV4\UV4.exe"
$ProjectPath = "MDK-ARM\stm32f411totas5815.uvprojx"

# Check if UV4 exists
if (-Not (Test-Path $UV4Path)) {
    Write-Host "Error: Keil UV4 not found at $UV4Path" -ForegroundColor Red
    exit 1
}

# Check if project file exists
if (-Not (Test-Path $ProjectPath)) {
    Write-Host "Error: Project file not found at $ProjectPath" -ForegroundColor Red
    exit 1
}

# Compile the project
Write-Host "Compiling STM32F411 TAS5815 Project..." -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

& $UV4Path -j0 -b $ProjectPath

# Check the result
if ($LASTEXITCODE -eq 0) {
    Write-Host "`nBuild completed successfully!" -ForegroundColor Green
    Write-Host "================================" -ForegroundColor Green
    
    # Show size information if available
    $axfPath = "MDK-ARM\stm32f411totas5815\stm32f411totas5815.axf"
    if (Test-Path $axfPath) {
        $size = (Get-Item $axfPath).Length
        Write-Host "Output file size: $size bytes" -ForegroundColor Cyan
    }
} else {
    Write-Host "`nBuild failed with exit code $LASTEXITCODE" -ForegroundColor Red
    Write-Host "=====================================" -ForegroundColor Red
    exit $LASTEXITCODE
}