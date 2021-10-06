@echo off
echo Extracting Notio version from GcUpgrade.h file.

SETLOCAL ENABLEDELAYEDEXPANSION

REM Get the library version line from GcUpgrade.h file. VERSION_NK = "version_number"
REM The delimiter is the space character.
REM #define is the first token.
REM VERSION_NK is the second token.
REM version_number is the third token.
for /F "tokens=3" %%E in ('
    FINDSTR /R /C:"^#define VERSION_NK " "..\..\src\Core\GcUpgrade.h"
') do set version_nk=%%~E

echo version_nk = %version_nk%

echo Extracting Golden Cheetah version from GcUpgrade.h file.

REM Get the library version line from GcUpgrade.h file. VERSION_STRING = "version_number"
REM The delimiter is the space character.
REM #define is the first token.
REM VERSION_STRING is the second token.
REM version_number is the third token.
for /F "tokens=3" %%E in ('
    FINDSTR /R /C:"^#define VERSION_STRING " "..\..\src\Core\GcUpgrade.h"
') do set version_string=%%~E

echo version_string = %version_string%

REM Call installer script passing the versions as parameters.
"C:\Program Files (x86)\NSIS\makensis" /NOCD  /DPRODUCT_BUILD=%version_string% /DPRODUCT_VERSION=%version_nk% ..\..\src\Resources\win32\GC-Notio-x64.nsi
