QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += multimedia

TARGET = hibikase
TEMPLATE = app
CONFIG += c++14

win32-msvc* {
    msvc:QMAKE_CXXFLAGS += /utf-8
}

include(../external/dr_libs.pri)
include(../external/rubberband.pri)

copydata.commands = $$quote($(COPY_DIR) \"$$shell_path($$PWD/../data)\" \"$$shell_path($$OUT_PWD/data)\")
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata

SOURCES += main.cpp \
    AudioFile.cpp \
    MainWindow.cpp \
    KaraokeData/Song.cpp \
    KaraokeData/SoramimiSong.cpp \
    KaraokeContainer/Container.cpp \
    KaraokeContainer/PlainContainer.cpp \
    KaraokeData/VsqxParser.cpp \
    LyricsEditor.cpp \
    PlaybackWidget.cpp \
    Settings.cpp \
    TextTransform/Syllabify.cpp \
    TextTransform/RomanizeHangul.cpp \
    TextTransform/HangulUtils.cpp \
    LineTimingDecorations.cpp

HEADERS  += MainWindow.h \
    AudioFile.h \
    KaraokeData/Song.h \
    KaraokeData/SoramimiSong.h \
    KaraokeContainer/Container.h \
    KaraokeContainer/PlainContainer.h \
    KaraokeData/ReadOnlySong.h \
    KaraokeData/VsqxParser.h \
    LyricsEditor.h \
    PlaybackWidget.h \
    Settings.h \
    TextTransform/Syllabify.h \
    TextTransform/RomanizeHangul.h \
    TextTransform/HangulUtils.h \
    LineTimingDecorations.h

FORMS    += MainWindow.ui
