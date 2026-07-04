@echo off
cd /d "%~dp0"
echo Starting Smart Resume Screening System...

if not exist resume_web.exe (
    echo Compiling the C++ web server...
    g++ -std=c++17 -Iinclude src\web_server.cpp src\screening_system.cpp -lws2_32 -o resume_web.exe
    if errorlevel 1 (
        echo.
        echo Compilation failed. Make sure MinGW g++ is installed.
        pause
        exit /b 1
    )
)

start "" http://localhost:8080
echo The application is opening at http://localhost:8080
echo Keep this window open. Press Ctrl+C to stop the application.
resume_web.exe
pause
