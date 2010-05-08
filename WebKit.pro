TEMPLATE = subdirs
CONFIG += ordered

include(WebKit.pri)

SUBDIRS += \
        JavaScriptCore \
        WebCore

# If the source exists, built it
exists($$PWD/WebKitTools/QtLauncher): SUBDIRS += WebKitTools/QtLauncher
exists($$PWD/JavaScriptCore/jsc.pro): SUBDIRS += JavaScriptCore/jsc.pro
exists($$PWD/WebKit/qt/tests): SUBDIRS += WebKit/qt/tests
exists($$PWD/WebKitTools/DumpRenderTree/qt/DumpRenderTree.pro): SUBDIRS += WebKitTools/DumpRenderTree/qt/DumpRenderTree.pro
exists($$PWD/WebKitTools/DumpRenderTree/qt/ImageDiff.pro): SUBDIRS += WebKitTools/DumpRenderTree/qt/ImageDiff.pro

!win32:!symbian {
    exists($$PWD/WebKitTools/DumpRenderTree/qt/TestNetscapePlugin/TestNetscapePlugin.pro): SUBDIRS += WebKitTools/DumpRenderTree/qt/TestNetscapePlugin/TestNetscapePlugin.pro
}

build-qtscript {
    SUBDIRS += \
        JavaScriptCore/qt/api/QtScript.pro \
        JavaScriptCore/qt/tests
}

symbian {
    # Forward the install target to WebCore. A workaround since INSTALLS is not implemented for symbian
    install.commands = $(MAKE) -C WebCore install
    QMAKE_EXTRA_TARGETS += install
}

include(WebKit/qt/docs/docs.pri)
