@echo off
setlocal
cd /d "%~dp0"
set PYTHONIOENCODING=utf-8

echo [1/2] Validating and importing FontCreator exports...
python tools\import_fontcreator.py || exit /b 1

echo [2/2] Building all ESP8266 release profiles...
where pio >nul 2>&1
if errorlevel 1 set "PIO=%USERPROFILE%\.platformio\penv\Scripts\pio.exe"
if defined PIO (
  "%PIO%" run -e p10_1x1_release -e p10_2x1_release -e p10_3x1_release -e p10_4x1_release || exit /b 1
) else (
  pio run -e p10_1x1_release -e p10_2x1_release -e p10_3x1_release -e p10_4x1_release || exit /b 1
)

echo Release font pipeline completed successfully.
endlocal