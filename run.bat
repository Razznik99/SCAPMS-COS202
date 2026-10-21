set PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.8.3\mingw_64\bin;%PATH%
cd build
SCAPMS.exe                          :: GUI
SCAPMS.exe --selftest               :: 279 automated checks
SCAPMS.exe --capture screenshots    :: regenerate screenshots