#!/bin/sh
rm resources/icon.icns
mkdir icon.iconset
cp resources/logo.png icon.iconset/icon_256x256.png
iconutil -c icns -o icon.icns icon.iconset
rm -r icon.iconset
mv icon.icns resources/
