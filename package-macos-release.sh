#!/usr/bin/env bash

set -e

ARGC=$#

if [[ $ARGC -ne 1 ]]; then
    echo "\
Usage: ./package-macos-release.sh <hibikase.app>

<hibikase.app> needs to already have been built using Qt Creator, or qmake +
make. You probably want a Release build, and you probably should do a clean
first to avoid packaging any unneccessary files. It will be modified in-place,
so make a copy first if you do not want this.

The result will be a hibikase.dmg file in the same directory. Any existing
hibikase.dmg will be deleted."
    exit -1
fi

HIBIKASE_DOT_APP=$1

if ! [[ -d $HIBIKASE_DOT_APP ]]; then
    echo "\
Provided hibikase.app directory path does not exist or is not a directory,
exiting."
    exit -1
fi

if ! [[ -x $HIBIKASE_DOT_APP/Contents/MacOS/hibikase ]]; then
    echo "\
Provided hibikase.app directory path does not contain the hibikase executable,
exiting."
    exit -1
fi

if ! command -v macdeployqt &>/dev/null; then
    echo 'macdeployqt (comes with Qt Creator) is not in PATH, exiting.'
    exit -1
fi

HIBIKASE_DOT_DMG=`dirname ${HIBIKASE_DOT_APP%/}`/hibikase.dmg
rm -f $HIBIKASE_DOT_DMG

macdeployqt $HIBIKASE_DOT_APP -dmg

if [[ -f $HIBIKASE_DOT_DMG ]]; then
    echo "$HIBIKASE_DOT_DMG has been created, enjoy! 🎶"
else
    echo "The expected '$HIBIKASE_DOT_DMG' file is not there?"
    exit -1
fi
