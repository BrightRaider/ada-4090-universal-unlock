@echo off
setlocal
cd /d "%~dp0"
title Universal GPU Unlock Setup (Ada, Ampere, Turing)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Setup-AdaUnlock.ps1"
if errorlevel 1 pause
