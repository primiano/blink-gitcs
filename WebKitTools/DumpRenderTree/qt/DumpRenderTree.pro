TARGET = DumpRenderTree
CONFIG  -= app_bundle

include(../../../WebKit.pri)
INCLUDEPATH += /usr/include/freetype2
INCLUDEPATH += ../../../JavaScriptCore/kjs
DESTDIR = ../../../bin


QT = core gui
macx: QT += xml network

HEADERS = DumpRenderTree.h jsobjects.h testplugin.h
SOURCES = DumpRenderTree.cpp main.cpp jsobjects.cpp testplugin.cpp

unix:!mac {
    QMAKE_RPATHDIR = $$OUTPUT_DIR/lib $$QMAKE_RPATHDIR
}

qt-port:lessThan(QT_MINOR_VERSION, 4) {
    DEFINES += QT_BEGIN_NAMESPACE="" QT_END_NAMESPACE=""
}
