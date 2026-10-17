#!/bin/zsh
# install.sh <apk>: installs our build next to the official app. The first time, copies the official app's user
# folder (config, saves, game settings) to ours; the game folders still have to be granted again in the app.
A=~/Library/Android/sdk/platform-tools/adb
PKG=org.dolphinemu.dolphinemu.odin3
SRC=/storage/emulated/0/Android/data/org.dolphinemu.dolphinemu/files
DST=/storage/emulated/0/Android/data/$PKG/files
$A install -r "$1" || exit 1
if ! $A shell "[ -d $DST/Config ]"; then
  $A shell "mkdir -p $DST && cp -r $SRC/. $DST/" && echo "copied the official app's user folder"
fi
