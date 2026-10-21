@echo off
rem ===========================================================================
rem  SCAPMS launcher - runs the built application with the repository data set.
rem  Usage: run.bat                 starts the GUI
rem         run.bat --selftest      runs the 279-check test suite
rem         run.bat --capture screenshots
rem ===========================================================================
setlocal
set PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.8.3\mingw_64\bin;%PATH%
"%~dp0build\SCAPMS.exe" --data "%~dp0data" %*
endlocal
