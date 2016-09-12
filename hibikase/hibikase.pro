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
    KaraokeData/VsqxParser.cpp

HEADERS  += MainWindow.h \
    KaraokeData/Song.h \
    KaraokeData/SoramimiMoonCatSong.h \
    KaraokeContainer/Container.h \
    KaraokeContainer/PlainContainer.h \
    KaraokeData/ReadOnlySong.h \
    KaraokeData/VsqxParser.h

FORMS    += MainWindow.ui
