#!/bin/sh
rm resources/icon.icns
mkdir icon.iconset
cp resources/logo.png icon.iconset/icon_256x256.png
cp resources/logo-small-128.png icon.iconset/icon_128x128.png
cp resources/logo-small-32.png icon.iconset/icon_32x32.png
cp resources/logo-micro-16.png icon.iconset/icon_16x16.png
iconutil -c icns -o icon.icns icon.iconset
rm -r icon.iconset
mv icon.icns resources/
