@echo off
REM =========================================
REM DWM API Server - Run Script
REM Auto-set UTF-8 encoding and run Maven
REM =========================================

REM Set console encoding to UTF-8
chcp 65001 >nul 2>&1

REM Set Java encoding
set JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF-8

REM Run Maven Spring Boot
call mvn spring-boot:run

