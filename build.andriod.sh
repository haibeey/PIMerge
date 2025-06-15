#!/bin/bash
set -e

STITCHER_PATH=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs
ANDRIOD_REPO_LIB_PATH=/Users/abrahamakerele/AndroidStudioProjects/PanoramaCapture/app/src/main/cpp/libs



for arch in arm64-v8a armeabi-v7a x86 x86_64; do
    cp $STITCHER_PATH/libturbojpeg/android/$arch/lib/libturbojpeg.a $ANDRIOD_REPO_LIB_PATH/libturbojpeg/$arch/lib/libturbojpeg.a
done


for arch in arm64-v8a armeabi-v7a x86 x86_64; do
    cp $STITCHER_PATH/native-stitcher/android/$arch/lib/libNativeStitcher.a $ANDRIOD_REPO_LIB_PATH/stitcher/$arch/lib/libNativeStitcher.a
done
