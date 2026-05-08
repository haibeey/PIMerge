#!/bin/bash
set -e


if [ -e! "$STITCHER_BUILD_PATH" ]; then
  echo "Path not found: $STITCHER_BUILD_PATH"
  exit 1
fi

if [ -e! "$ANDRIOD_REPO_LIB_PATH" ]; then
  echo "Path not found: $ANDRIOD_REPO_LIB_PATH"
  exit 1
fi


for arch in arm64-v8a armeabi-v7a x86 x86_64; do
    cp $STITCHER_BUILD_PATH/libturbojpeg/android/$arch/lib/libturbojpeg.a $ANDRIOD_REPO_LIB_PATH/libturbojpeg/$arch/lib/libturbojpeg.a
done


for arch in arm64-v8a armeabi-v7a x86 x86_64; do
    cp $STITCHER_BUILD_PATH/native-stitcher/android/$arch/lib/libNativeStitcher.a $ANDRIOD_REPO_LIB_PATH/stitcher/$arch/lib/libNativeStitcher.a
done
