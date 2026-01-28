@echo off
REM ============================================================================
REM start_server.bat
REM Quick launcher for StructuredLogging HTTP Server
REM Developer: Marcus Daley
REM Date: January 25, 2026
REM ============================================================================

echo Starting StructuredLogging HTTP Server...
echo.
cd /d "%~dp0"
python LogServer.py
pause
