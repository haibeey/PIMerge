#!/bin/bash
set -e

rm -rf iosbuild/*
mkdir -p iosbuild/stitcher
mkdir -p iosbuild/libturbojpeg

STITCHER_BUILD_PATH=/Users/haibeey/p-h/lendo-stuff/stitcher/installs

SIM_ARM_STITCHER_LIB=$STITCHER_BUILD_PATH/native-stitcher/ios-sim/arm64/lib/libNativeStitcher.a
SIM_X86_STITCHER_LIB=$STITCHER_BUILD_PATH/native-stitcher/ios-sim/x86_64/lib/libNativeStitcher.a
ARM_STITCHER_LIB=$STITCHER_BUILD_PATH/native-stitcher/ios/arm64/lib/libNativeStitcher.a

SIM_ARM_TURBOJPEG_LIB=$STITCHER_BUILD_PATH/libturbojpeg/ios-sim/arm64/lib/libturbojpeg.a
SIM_X86_TURBOJPEG_LIB=$STITCHER_BUILD_PATH/libturbojpeg/ios-sim/x86_64/lib/libturbojpeg.a
ARM_TURBOJPEG_LIB=$STITCHER_BUILD_PATH/libturbojpeg/ios/arm64/lib/libturbojpeg.a

SIM_ARM_STITCHER_HEADER=$STITCHER_BUILD_PATH/native-stitcher/ios-sim/arm64/include/
SIM_X86_STITCHER_HEADER=$STITCHER_BUILD_PATH/native-stitcher/ios-sim/x86_64/include/
ARM_STITCHER_HEADER=$STITCHER_BUILD_PATH/native-stitcher/ios/arm64/include/

dirs=("SIM_ARM_STITCHER_HEADER" "SIM_X86_STITCHER_HEADER" "ARM_STITCHER_HEADER")
files=("jpeglib.h" "jconfig.h" "turbojpeg.h" "jerror.h" "jmorecfg.h")

for dir_var in "${dirs[@]}"; do
    dir_path="${!dir_var}"
    if [[ -d "$dir_path" ]]; then
        for file in "${files[@]}"; do
            target="$dir_path/$file"
            if [[ -f "$target" ]]; then
                echo "Deleting $target"
                rm "$target"
            else
                echo "Not found: $target"
            fi
        done
    else
        echo "Directory not found or not set: $dir_var"
    fi
done

SIM_ARM_TURBOJPEG_HEADER=$STITCHER_BUILD_PATH/libturbojpeg/ios-sim/arm64/include/
SIM_X86_TURBOJPEG_HEADER=$STITCHER_BUILD_PATH/libturbojpeg/ios-sim/x86_64/include/
ARM_TURBOJPEG_HEADER=$STITCHER_BUILD_PATH/libturbojpeg/ios/arm64/include/




pushd iosbuild/libturbojpeg
lipo -create $SIM_ARM_TURBOJPEG_LIB $SIM_X86_TURBOJPEG_LIB  -output libturbojpeg-sim.a
xcodebuild -create-xcframework   -library $ARM_TURBOJPEG_LIB  -headers $ARM_TURBOJPEG_HEADER   -library libturbojpeg-sim.a  -headers  $SIM_ARM_TURBOJPEG_HEADER   -output libturbojpeg.xcframework

popd


pushd iosbuild/stitcher
lipo -create $SIM_X86_STITCHER_LIB $SIM_ARM_STITCHER_LIB  -output libNativeStitcher-sim.a
xcodebuild -create-xcframework   -library $ARM_STITCHER_LIB -headers $ARM_STITCHER_HEADER    -library libNativeStitcher-sim.a -headers $SIM_X86_STITCHER_HEADER    -output NativeStitcher.xcframework

popd
