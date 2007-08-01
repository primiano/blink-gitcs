!IF !defined(BUILDSTYLE)
BUILDSTYLE=Release
!ELSEIF "$(BUILDSTYLE)"=="Debug"
BUILDSTYLE=Debug_Internal
!ENDIF

install:
    set BuildBot=1
	set WebKitLibrariesDir="$(SRCROOT)\AppleInternal"
	set WebKitOutputDir=$(OBJROOT)
	devenv "JavaScriptCore.sln" /rebuild $(BUILDSTYLE)
	xcopy "$(OBJROOT)\bin\*" "$(DSTROOT)\AppleInternal\bin\" /e/v/i/h/y
	xcopy "$(OBJROOT)\include\*" "$(DSTROOT)\AppleInternal\include\" /e/v/i/h/y	
	xcopy "$(OBJROOT)\lib\*" "$(DSTROOT)\AppleInternal\lib\" /e/v/i/h/y	
