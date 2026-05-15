@echo off
REM Simple compile command for STM32F411 TAS5815 project
REM Uses Keil MDK-ARM AC6 compiler

echo Compiling STM32F411 TAS5815 Project...
echo ======================================

REM Run Keil UV4 compiler
"C:\Keil_v5\UV4\UV4.exe" -j0 -b "MDK-ARM\stm32f411totas5815.uvprojx"

REM Check result
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build completed successfully!
    echo =============================
) else (
    echo.
    echo Build failed with error code %ERRORLEVEL%
    echo =====================================
    exit /b %ERRORLEVEL%
)