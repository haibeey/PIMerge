#!/bin/bash
set -e

rm -rf iosbuild/*
mkdir -p iosbuild/stitcher
mkdir -p iosbuild/libturbojpeg

SIM_ARM_STITCHER_LIB=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/ios-sim/arm64/lib/libNativeStitcher.a
SIM_X86_STITCHER_LIB=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/ios-sim/x86_64/lib/libNativeStitcher.a
ARM_STITCHER_LIB=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/ios/arm64/lib/libNativeStitcher.a

SIM_ARM_TURBOJPEG_LIB=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/libturbojpeg/ios-sim/arm64/lib/libturbojpeg.a
SIM_X86_TURBOJPEG_LIB=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/libturbojpeg/ios-sim/x86_64/lib/libturbojpeg.a
ARM_TURBOJPEG_LIB=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/libturbojpeg/ios/arm64/lib/libturbojpeg.a

SIM_ARM_STITCHER_HEADER=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/ios-sim/arm64/include/
SIM_X86_STITCHER_HEADER=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/ios-sim/x86_64/include/
ARM_STITCHER_HEADER=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/native-stitcher/ios/arm64/include/

SIM_ARM_TURBOJPEG_HEADER=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/libturbojpeg/ios-sim/arm64/include/
SIM_X86_TURBOJPEG_HEADER=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/libturbojpeg/ios-sim/x86_64/include/
ARM_TURBOJPEG_HEADER=/Users/abrahamakerele/p-h/lendostuff/stitcher/installs/libturbojpeg/ios/arm64/include/




pushd iosbuild/libturbojpeg
lipo -create $SIM_ARM_TURBOJPEG_LIB $SIM_X86_TURBOJPEG_LIB  -output libturbojpeg-sim.a
xcodebuild -create-xcframework   -library $ARM_TURBOJPEG_LIB  -headers $ARM_TURBOJPEG_HEADER   -library libturbojpeg-sim.a  -headers  $SIM_ARM_TURBOJPEG_HEADER   -output libturbojpeg.xcframework

popd


pushd iosbuild/stitcher
lipo -create $SIM_X86_STITCHER_LIB $SIM_ARM_STITCHER_LIB  -output libNativeStitcher-sim.a
xcodebuild -create-xcframework   -library $ARM_STITCHER_LIB -headers $ARM_STITCHER_HEADER    -library libNativeStitcher-sim.a -headers $SIM_X86_STITCHER_HEADER    -output NativeStitcher.xcframework

popd
