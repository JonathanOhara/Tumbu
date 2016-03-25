!define VERSION "1.50" 
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Tumbu"
Name "Tumbu Beta ${VERSION}"

OutFile "Tumbu BETA Installer - ${VERSION}.exe"

InstallDir $PROGRAMFILES\Shyds
DirText "This will install the game Tumbu Beta ${VERSION} on your computer. Choose a directory"

SetCompressor /SOLID lzma

Section "Install"
	SetOutPath $INSTDIR
	File /r "Tumbu"
	
	WriteUninstaller $INSTDIR\Tumbu\Uninstall.exe
	
	SetOutPath "$INSTDIR\Tumbu\bin\Release\"
	
	ExecWait "$INSTDIR\Tumbu\dependencies\vcredist_x86.exe"
	
	CreateDirectory $PROFILE\Tumbu
	CopyFiles $INSTDIR\Tumbu\bin\Release\ogre.cfg $PROFILE\Tumbu
	
	CreateDirectory "$SMPROGRAMS\Shyds"
	CreateDirectory "$SMPROGRAMS\Shyds\Tumbu"
	
	CreateShortCut "$DESKTOP\Tumbu.lnk" "$INSTDIR\Tumbu\bin\Release\Tumbu.exe" "$INSTDIR\Tumbu\bin\Release\"
	CopyFiles "$DESKTOP\Tumbu.lnk" "$SMPROGRAMS\Shyds\Tumbu\Tumbu.lnk"
	CopyFiles  "$INSTDIR\Tumbu\ShyDS Games.url" "$SMPROGRAMS\Shyds\Tumbu\ShyDS Games.url"
	
	WriteRegStr HKLM "${UNINST_KEY}" "DisplayName" "Tumbu Beta ${VERSION}"
	WriteRegStr HKLM "${UNINST_KEY}" "UninstallString" "$INSTDIR\Tumbu\Uninstall.exe"
	
	MessageBox MB_OK "Install Successful."
SectionEnd

Section "Uninstall"

	Delete $INSTDIR\Uninstall.exe
	Delete $DESKTOP\Tumbu.lnk
	Delete "$SMPROGRAMS\Shyds\Tumbu\Tumbu.lnk"
	Delete "$SMPROGRAMS\Shyds\Tumbu\ShyDS Games.url"
	
	RMDir /r $PROFILE\Tumbu
	RMDir /r $SMPROGRAMS\Shyds\Tumbu
	RMDir /r $INSTDIR
	
	DeleteRegKey HKLM "${UNINST_KEY}"
	
	MessageBox MB_OK "Uninstall Successful."
SectionEnd
