!include "FileFunc.nsh"

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "Golden Cheetah Notio"
!define PRODUCT_FILENAME "GCNotio.exe"
!define PRODUCT_BIT "64"
!define PRODUCT_STARTMENU "Golden Cheetah Notio"
!define PRODUCT_WEB_SITE_FILENAME "GoldenCheetah WebSite"
!define PRODUCT_WEB_SITE_URL "http://www.goldencheetah.org"
!define PRODUCT_WEB_SITE_NK_FILENAME "Notio WebSite"
!define PRODUCT_WEB_SITE_NK_URL "https://www.notio.ai"
!define PRODUCT_WIKI_FILENAME "GoldenCheetah Wiki"
!define PRODUCT_WIKI_URL "http://www.goldencheetah.org/wiki.html"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\${PRODUCT_FILENAME}"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_SETTINGS_KEY "Software\goldencheetah.org"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; Old version definitions
!define OLD_PRODUCT_NAME "Golden Cheetah Notio Konect"
!define OLD_PRODUCT_FILENAME "GoldenCheetahNotioKonect.exe"
!define OLD_PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\${OLD_PRODUCT_FILENAME}"

Function .onInit
  System::Call 'kernel32::CreateMutex(p 0, i 0, t "GC-Installer") p .r1 ?e'
  Pop $R0

  StrCmp $R0 0 +3 uninstallPrevious
    MessageBox MB_OK|MB_ICONEXCLAMATION "The installer is already running."
  Abort
  
  ; Verify if an older version with old name is installed.
  uninstallPrevious:
  
  ReadRegStr $R1 HKLM "${OLD_PRODUCT_DIR_REGKEY}" ""
  IfFileExists "$R1" appFound
    Return
  appFound:
    MessageBox MB_OK|MB_ICONEXCLAMATION \
    "An old version has been found. Make \
	sure to backup of your athlete library. \
	Please uninstall any previous version \
	in order to continue the update process."
	Abort
FunctionEnd


;--------------------------------
;Include Modern UI

!include "MUI2.nsh"

;--------------------------------
;General

Name "${PRODUCT_NAME} ${PRODUCT_VERSION} (${PRODUCT_BIT} bit)"
!system 'md ..\Installer'
OutFile "..\Installer\GCNotio_${PRODUCT_BUILD}_${PRODUCT_VERSION}_${PRODUCT_BIT}bit.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" "Install_Dir"
Icon "..\..\src\Resources\win32\gc.ico"
RequestExecutionLevel admin

;--------------------------------
;Interface Settings

!define MUI_ABORTWARNING
!define MUI_ICON "..\..\src\Resources\win32\gc.ico"
!define MUI_UNICON "..\..\src\Resources\win32\gc.ico"

;--------------------------------
;Pages

; Welcome page
!insertmacro MUI_PAGE_WELCOME

; License page
!insertmacro MUI_PAGE_LICENSE "..\..\src\Resources\win32\license.txt"

; Directory page
!insertmacro MUI_PAGE_DIRECTORY

; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_NOAUTOCLOSE
; Finish page
;!define MUI_FINISHPAGE_RUN "$INSTDIR\GCNotio.exe" - don't offer run - since we may be in wrong Shell Context
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------


Section "Golden Cheetah (required)" SEC01
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\${PRODUCT_STARTMENU}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_STARTMENU}\GC Notio.lnk" "$INSTDIR\${PRODUCT_FILENAME}" "--platform windows:dpiawareness=0"
  CreateShortCut "$DESKTOP\GC Notio.lnk" "$INSTDIR\${PRODUCT_FILENAME}" "--platform windows:dpiawareness=0"

  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  File /r /x "moc" /x "obj" /x "rcc" /x "*.pdb" "*"
  File "..\..\src\Resources\win32\gc.ico"
  File "..\..\src\Resources\win32\LICENSE.TXT"
  
  ;Add missing OpenSSL DLL. This is a temporary fix. It needs to be standardized.
  ;Uncomment the following 2 lines if using QT5.12.3 and earlier versions.
  ;File "C:\Program Files\Git\mingw64\bin\ssleay32.dll"
  ;File "C:\Program Files\Git\mingw64\bin\libeay32.dll"
  File "$%QT_HOME%\Tools\OpenSSL\Win_x64\bin\libssl-1_1-x64.dll"
  File "$%QT_HOME%\Tools\OpenSSL\Win_x64\bin\libcrypto-1_1-x64.dll"
  File "/oname=OpenSSL_License.txt" "$%QT_HOME%\Tools\OpenSSL\src\LICENSE"
  
  ;Qt OpenSSL 1.1.1d Toolkit dependency.
  ;For some reason, the QT didn't provide binairies built with more recent runtimes.
  File /nonfatal "$%QT_HOME%\vcredist\vcredist_msvc2010_x64.exe"
  
  ;Install MSVC redistributable
  IfFileExists "$INSTDIR\vc_redist.x64.exe" PathGood
    ExecWait '"$INSTDIR\vcredist_x64.exe" /passive /norestart'
  PathGood:
    ExecWait '"$INSTDIR\vc_redist.x64.exe" /passive /norestart'

  IfFileExists "$INSTDIR\vcredist_msvc2010_x64.exe" PathGood2
    Return
  PathGood2:
    ExecWait '"$INSTDIR\vcredist_msvc2010_x64.exe" /passive /norestart'
  
SectionEnd

Section -AdditionalIcons
  SetShellVarContext all
  SetOutPath $INSTDIR
  WriteIniStr "$INSTDIR\${PRODUCT_WEB_SITE_FILENAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE_URL}"
  WriteIniStr "$INSTDIR\${PRODUCT_WEB_SITE_NK_FILENAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE_NK_URL}"
  WriteIniStr "$INSTDIR\${PRODUCT_WIKI_FILENAME}.url" "InternetShortcut" "URL" "${PRODUCT_WIKI_URL}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_STARTMENU}\Need help - Check the Wiki.lnk" "$INSTDIR\${PRODUCT_WIKI_FILENAME}.url"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_STARTMENU}\GoldenCheetah Website.lnk" "$INSTDIR\${PRODUCT_WEB_SITE_FILENAME}.url"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_STARTMENU}\Notio Website.lnk" "$INSTDIR\${PRODUCT_WEB_SITE_NK_FILENAME}.url"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_STARTMENU}\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\${PRODUCT_FILENAME}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\${PRODUCT_FILENAME}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_BUILD}_${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE_NK_URL}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NotioURL" "${PRODUCT_WEB_SITE_NK_URL}"
SectionEnd


Section Uninstall
  SetShellVarContext all
  Delete "$INSTDIR\*"

  RMDir /r "$INSTDIR\*"
  RMDir "$INSTDIR"

  SetShellVarContext all
  Delete "$SMPROGRAMS\${PRODUCT_STARTMENU}\*.*"
  Delete "$SMPROGRAMS\${PRODUCT_STARTMENU}\Uninstall.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_STARTMENU}\Need help - Check the Wiki.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_STARTMENU}\GoldenCheetah Website.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_STARTMENU}\Notio Website.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_STARTMENU}\GC Notio.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_STARTMENU}\*.lnk"
  RMDir /r "$SMPROGRAMS\${PRODUCT_STARTMENU}"
  Delete "$DESKTOP\GC Notio.lnk"
  ;Delete "$DESKTOP\Golden Cheetah - HighDPI.lnk"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  ;no not delete the User Preferences in UnInstall
  ;DeleteRegKey HKCU "${PRODUCT_SETTINGS_KEY}"
  SetAutoClose false
SectionEnd