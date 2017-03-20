QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = hibikase
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    KaraokeData/Song.cpp \
    KaraokeData/SoramimiMoonCatSong.cpp \
    KaraokeContainer/Container.cpp \
    KaraokeContainer/PlainContainer.cpp \
    KaraokeData/VsqxParser.cpp \
    LyricsEditor.cpp \
    Settings.cpp \
    TextTransform/Syllabify.cpp \
    TextTransform/HangulUtils.cpp

HEADERS  += MainWindow.h \
    KaraokeData/Song.h \
    KaraokeData/SoramimiMoonCatSong.h \
    KaraokeContainer/Container.h \
    KaraokeContainer/PlainContainer.h \
    KaraokeData/ReadOnlySong.h \
    KaraokeData/VsqxParser.h \
    LyricsEditor.h \
    Settings.h \
    TextTransform/Syllabify.h \
    TextTransform/HangulUtils.h

FORMS    += MainWindow.ui
