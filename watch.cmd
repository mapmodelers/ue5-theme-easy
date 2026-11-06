@echo off
rem Start once and leave open: runs go/close requests for this repo only (dev loop).
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0_tools\watch.ps1"
