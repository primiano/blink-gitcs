all:
    touch "$(WEBKITOUTPUTDIR)\buildfailed"
    -mkdir 2>NUL "$(WEBKITOUTPUTDIR)\include\WebKit2"
    xcopy /y /d "..\UIProcess\API\C\WebKit2.h" "$(WEBKITOUTPUTDIR)\include\WebKit2"
    xcopy /y /d "..\UIProcess\API\C\WKBase.h" "$(WEBKITOUTPUTDIR)\include\WebKit2"
    xcopy /y /d "..\UIProcess\API\C\WKContext.h" "$(WEBKITOUTPUTDIR)\include\WebKit2"
    xcopy /y /d "..\UIProcess\API\C\WKFrame.h" "$(WEBKITOUTPUTDIR)\include\WebKit2"
    xcopy /y /d "..\UIProcess\API\C\WKFramePolicyListener.h" "$(WEBKITOUTPUTDIR)\include\WebKit2"
    xcopy /y /d "..\UIProcess\API\C\WKPage.h" "$(WEBKITOUTPUTDIR)\include\WebKit2"
    xcopy /y /d "..\UIProcess\API\C\WKPageNamespace.h" "$(WEBKITOUTPUTDIR)\include\WebKit2"
    xcopy /y /d "..\UIProcess\API\C\WKPreferences.h" "$(WEBKITOUTPUTDIR)\include\WebKit2"
    xcopy /y /d "..\UIProcess\API\win\WKBaseWin.h" "$(WEBKITOUTPUTDIR)\include\WebKit2"
    xcopy /y /d "..\UIProcess\API\win\WKView.h" "$(WEBKITOUTPUTDIR)\include\WebKit2"
    -del "$(WEBKITOUTPUTDIR)\buildfailed"

clean:
    -del "$(WEBKITOUTPUTDIR)\buildfailed"
    -del /s /q "$(WEBKITOUTPUTDIR)\include\WebKit2"
