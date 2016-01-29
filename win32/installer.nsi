; Script based on Off-the-Record Messaging NSI file

; todo: SetBrandingImage
; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "whatsapp4pidgin"
!define PRODUCT_VERSION "0.9.0"
!define PRODUCT_PUBLISHER "David Guillen Fandos"
!define PRODUCT_WEB_SITE "https://davidgf.net/"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
;!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_ICON "whatsapp-purple-pidgin.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "wizard.bmp"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
;!insertmacro MUI_PAGE_LICENSE 
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${PRODUCT_NAME}-${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES\whatsapp4pidgin"
InstallDirRegKey HKEY_LOCAL_MACHINE SOFTWARE\whatsapp4pidgin "Install_Dir"

Var "PidginDir"
var "DllName"

ShowInstDetails show
ShowUnInstDetails show

!macro !defineifexist _VAR_NAME _FILE_NAME
	!tempfile _TEMPFILE
	!ifdef NSIS_WIN32_MAKENSIS
		; Windows - cmd.exe
		!system 'if exist "${_FILE_NAME}" echo !define ${_VAR_NAME} > "${_TEMPFILE}"'
	!else
		; Posix - sh
		!system 'if [ -e "${_FILE_NAME}" ]; then echo "!define ${_VAR_NAME}" > "${_TEMPFILE}"; fi'
	!endif
	!include '${_TEMPFILE}'
	!delfile '${_TEMPFILE}'
	!undef _TEMPFILE
!macroend
!define !defineifexist "!insertmacro !defineifexist"

Section "MainSection" SEC01
    ;InstallDir "$PROGRAMFILES\Pidgin\plugins"

  ${!defineifexist} PIDGIN_PLUGIN "whatsapp-pidgin-plugin.dll"

  ; uninstall previous pidgin-otr install if found.
  Call UnInstOld
  ;Check for pidgin installation
  Call GetPidginInstPath
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "SOFTWARE\whatsapp4pidgin" "pidgindir" "$PidginDir"
  SetOutPath "$PidginDir\pixmaps\pidgin\protocols\16"
  SetOverwrite on
  File /oname=whatsapp.png "whatsapp16.png"

  SetOutPath "$PidginDir\pixmaps\pidgin\protocols\22"
  SetOverwrite on
  File /oname=whatsapp.png "whatsapp22.png"

  SetOutPath "$PidginDir\pixmaps\pidgin\protocols\48"
  SetOverwrite on
  File /oname=whatsapp.png "whatsapp48.png"

  SetOutPath "$INSTDIR"
  SetOverwrite on
  File "libwhatsapp.dll"

  SetOutPath "$PidginDir\"
  File "deps/*.dll"

  !ifdef PIDGIN_PLUGIN
    SetOutPath "$INSTDIR"
    SetOverwrite on
    File "whatsapp-pidgin-plugin.dll"
    !echo "Adding WhatsApp plugin for Pidgin as well!"

    StrCpy $DllName "whatsapp-pidgin-plugin.dll"
    call CopyDLL
  !endif

  ; move to pidgin plugin directory, check if not busy (pidgin is running)
  StrCpy $DllName "libwhatsapp.dll"
  call CopyDLL

  Rename "$INSTDIR\$DllName" "$PidginDir\plugins\$DllName"

  ; hard part is done, do the rest now.
  SetOverwrite on
  File "installer.nsi"
SectionEnd

Section -AdditionalIcons
  CreateDirectory "$SMPROGRAMS\WhatsApp Plugin for Pidgin"
  CreateShortCut "$SMPROGRAMS\WhatsApp Plugin for Pidgin\Uninstall.lnk" "$INSTDIR\whatsapp4pidgin-uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\whatsapp4pidgin-uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\whatsapp4pidgin-uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
 
SectionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall

  Delete "$INSTDIR\whatsapp4pidgin-uninst.exe"
  Delete "$INSTDIR\installer.nsi"
  Delete "$SMPROGRAMS\whatsapp4pidgin\Uninstall.lnk"
  RMDir "$SMPROGRAMS\whatsapp4pidgin"
  RMDir "$INSTDIR"
  
  ReadRegStr $PidginDir HKLM Software\whatsapp4pidgin "pidgindir"
  IfFileExists "$PidginDir\plugins\libwhatsapp.dll" dodelete
  ReadRegStr $PidginDir HKCU Software\whatsapp4pidgin "pidgindir"
  IfFileExists "$PidginDir\plugins\libwhatsapp.dll" dodelete
	
  ReadRegStr $PidginDir HKLM Software\whatsapp4pidgin "pidgindir"
  IfFileExists "$PidginDir\plugins\libwhatsapp.dll" dodelete
  ReadRegStr $PidginDir HKCU Software\whatsapp4pidgin "pidgindir"
  IfFileExists "$PidginDir\plugins\libwhatsapp.dll" dodelete
  MessageBox MB_OK|MB_ICONINFORMATION "Could not find pidgin plugin directory, libwhatsapp.dll not uninstalled!" IDOK ok

dodelete:
  Delete "$PidginDir\plugins\libwhatsapp.dll"
  !ifdef PIDGIN_PLUGIN
    Delete "$PidginDir\plugins\whatsapp-pidgin-plugin.dll"
  !endif
  Delete "$PidginDir\pixmaps\pidgin\protocols\16\whatsapp.png"
  Delete "$PidginDir\pixmaps\pidgin\protocols\22\whatsapp.png"
  Delete "$PidginDir\pixmaps\pidgin\protocols\48\whatsapp.png"
	
	IfFileExists "$PidginDir\plugins\libwhatsapp.dll" 0 +2
	MessageBox MB_OK|MB_ICONINFORMATION "libwhatsapp.dll is busy. Probably Pidgin is still running. Please delete $PidginDir\plugins\libwhatsapp.dll manually."
  !ifdef PIDGIN_PLUGIN
    IfFileExists "$PidginDir\plugins\whatsapp-pidgin-plugin.dll" 0 +2
    MessageBox MB_OK|MB_ICONINFORMATION "whatsapp-pidgin-plugin.dll is busy. Probably Pidgin is still running. Please delete $PidginDir\plugins\whatsapp-pidgin-plugin.dll manually."
  !endif


  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "SOFTWARE\whatsapp4pidgin\pidgindir"
ok:
SetAutoClose true
SectionEnd

Function GetPidginInstPath
  Push $0
  ReadRegStr $0 HKLM "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
	ReadRegStr $0 HKCU "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
  MessageBox MB_OK|MB_ICONINFORMATION "Failed to find Pidgin installation."
		Abort "Failed to find Pidgin installation. Please install Pidgin first."
cont:
	StrCpy $PidginDir $0
	;MessageBox MB_OK|MB_ICONINFORMATION "Pidgin plugin directory found at $PidginDir\plugins ."
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "SOFTWARE\whatsapp4pidgin" "pidgindir" "$PidginDir"
FunctionEnd

Function UnInstOld
	  Push $0
	  ReadRegStr $0 ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString"
		IfFileExists "$0" deinst cont
	deinst:
		MessageBox MB_OK|MB_ICONEXCLAMATION  "whatsapp4pidgin was already found on your system and will first be uninstalled"
		; the uninstaller copies itself to temp and execs itself there, so it can delete 
		; everything including its own original file location. To prevent the installer and
		; uninstaller racing you can't simply ExecWait.
		; We hide the uninstall because otherwise it gets really confusing window-wise
		;HideWindow
		  ClearErrors
			ExecWait '"$0" _?=$INSTDIR'
			IfErrors 0 cont
				MessageBox MB_OK|MB_ICONEXCLAMATION  "Uninstall failed or aborted"
				Abort "Uninstalling of the previous version gave an error. Install aborted."
			
		;BringToFront
	cont:
		;MessageBox MB_OK|MB_ICONINFORMATION "No old whatsapp4pidgin found, continuing."
		
FunctionEnd

Function CopyDLL
SetOverwrite try
ClearErrors
; 3 hours wasted so you guys don't need a reboot!
IfFileExists "$PidginDir\plugins\$DllName" 0 copy ; remnant or uninstall prev version failed
Delete "$PidginDir\plugins\$DllName"
copy:
ClearErrors
Rename "$INSTDIR\$DllName" "$PidginDir\plugins\$DllName"
IfErrors dllbusy
	Return
dllbusy:
	MessageBox MB_RETRYCANCEL "$DllName is busy. Please close Pidgin (including tray icon) and try again" IDCANCEL cancel
	Delete "$PidginDir\plugins\$DllName"
	Goto copy
	Return
cancel:
	Abort "Installation of whatsapp4pidgin aborted"
FunctionEnd

