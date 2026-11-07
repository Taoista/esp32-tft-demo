@echo off
setlocal
cd /d "%~dp0web"

where py >nul 2>nul
if errorlevel 1 (
  echo Python 3 no esta instalado. Instala Python 3 y vuelve a ejecutar este archivo.
  pause
  exit /b 1
)

start "Servidor ESP32 TFT" cmd /k "py -3 -m http.server 8765 --bind 127.0.0.1"
timeout /t 2 /nobreak >nul
start "" "http://127.0.0.1:8765/"
echo Demo abierta. Deja la ventana del servidor abierta mientras la uses.
