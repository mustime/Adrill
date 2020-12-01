
targetArch=x86
buildDir=build

cmake -S . -B $buildDir -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=21 -DCMAKE_ANDROID_ARCH_ABI=${targetArch} -DCMAKE_ANDROID_NDK=$ANDROID_NDK_ROOT
cmake --build $buildDir --parallel 4 --target adrill
