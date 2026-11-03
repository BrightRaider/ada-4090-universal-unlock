@echo off
setlocal
title Universal GPU Unlock Status Check
color 0F
cls

set "PYTHON_EXE="

:: 1. Check known unsloth studio environment
if exist "C:\Users\%USERNAME%\.unsloth\studio\unsloth_studio\Scripts\python.exe" (
    set "PYTHON_EXE=C:\Users\%USERNAME%\.unsloth\studio\unsloth_studio\Scripts\python.exe"
    goto :RunCheck
)

:: 2. Check active Conda environment
if defined CONDA_PREFIX (
    if exist "%CONDA_PREFIX%\python.exe" (
        set "PYTHON_EXE=%CONDA_PREFIX%\python.exe"
        goto :RunCheck
    )
)

:: 3. Check system PATH python
where python.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set "PYTHON_EXE=python.exe"
    goto :RunCheck
)

:: 4. Check Windows Python launcher py
where py.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set "PYTHON_EXE=py.exe"
    goto :RunCheck
)

echo ====================================================================
echo   [ERROR] Python was not found on this system.
echo   Please install Python (https://www.python.org) and PyTorch.
echo ====================================================================
pause
exit /b 1

:RunCheck
set "SCRIPT_DIR=%~dp0"
set "CHECK_SCRIPT=%SCRIPT_DIR%tools\quick_check.py"
if not exist "%CHECK_SCRIPT%" (
    if exist "%SCRIPT_DIR%quick_check.py" (
        set "CHECK_SCRIPT=%SCRIPT_DIR%quick_check.py"
    )
)

"%PYTHON_EXE%" "%CHECK_SCRIPT%"
if %ERRORLEVEL% equ 0 (
    color 0A
) else (
    color 0C
)
echo.
pause
