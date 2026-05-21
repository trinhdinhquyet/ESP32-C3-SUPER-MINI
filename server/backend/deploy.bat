@echo off
REM =========================================
REM DWM API Server - Windows Deploy Script
REM =========================================

echo.
echo ╔═══════════════════════════════════════════════╗
echo ║   DWM API Server - Deployment Script         ║
echo ╚═══════════════════════════════════════════════╝
echo.

REM Configuration - Modify these paths as needed
set TOMCAT_HOME=C:\apache-tomcat-10
set PROJECT_DIR=%~dp0
set WAR_FILE=dwm-api-server.war

echo [1/5] Cleaning previous build...
cd /d "%PROJECT_DIR%"
call mvn clean
if %ERRORLEVEL% NEQ 0 (
    echo ❌ Clean failed!
    pause
    exit /b 1
)

echo.
echo [2/5] Building WAR file...
call mvn package -DskipTests
if %ERRORLEVEL% NEQ 0 (
    echo ❌ Build failed!
    pause
    exit /b 1
)

echo.
echo [3/5] Stopping Tomcat...
if exist "%TOMCAT_HOME%\bin\shutdown.bat" (
    call "%TOMCAT_HOME%\bin\shutdown.bat"
    timeout /t 5 /nobreak
) else (
    echo ⚠️ Tomcat not found at %TOMCAT_HOME%
)

echo.
echo [4/5] Deploying WAR to Tomcat...
if exist "%TOMCAT_HOME%\webapps" (
    copy /Y "target\%WAR_FILE%" "%TOMCAT_HOME%\webapps\"
    echo ✅ WAR deployed successfully!
) else (
    echo ❌ Tomcat webapps directory not found!
    pause
    exit /b 1
)

echo.
echo [5/5] Starting Tomcat...
if exist "%TOMCAT_HOME%\bin\startup.bat" (
    call "%TOMCAT_HOME%\bin\startup.bat"
    echo ✅ Tomcat started!
) else (
    echo ❌ Tomcat startup script not found!
)

echo.
echo ╔═══════════════════════════════════════════════╗
echo ║   ✅ Deployment Complete!                     ║
echo ║                                               ║
echo ║   Access: http://localhost:8080/%WAR_FILE:~0,-4%/      ║
echo ║   Logs:   %TOMCAT_HOME%\logs\           ║
echo ╚═══════════════════════════════════════════════╝
echo.

pause

