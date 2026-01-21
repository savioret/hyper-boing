@echo off
setlocal enabledelayedexpansion
for %%i in (*.bmp) do (
    convert "%%i" "%%~ni.png"
)