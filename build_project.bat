@echo off
REM STM32F411 TAS5815 Project Build Script
REM This script compiles the project using Keil MDK-ARM AC6 compiler

echo Building STM32F411 TAS5815 Project...
echo ======================================

REM Set Keil UV4 path
set UV4_PATH=C:\Keil_v5\UV4\UV4.exe

REM Change to project directory
cd /d "%~dp0"

REM Compile the project and display output directly
"%UV4_PATH%" -j0 -b "MDK-ARM\stm32f411totas5815.uvprojx"

REM Check if build was successful
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build completed successfully!
    echo =============================
) else (
    echo.
    echo Build failed with error code %ERRORLEVEL%
    echo =====================================
)

pause