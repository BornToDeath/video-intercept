1, 从github下载源码
git clone https://github.com/libjpeg-turbo/libjpeg-turbo.git
2，将代码切换到2.0.5版本
git checkout 2.0.5
3，根据云智车机的要求修改代码
注释点文件jdmarker.c的926行代码，代码如下：
WARNMS2(cinfo, JWRN_EXTRANEOUS_DATA, cinfo->marker->discarded_bytes, c);
4，通过cmake编译64位和32位库
mkdir build
cd build
32位编译：
cmake -DCMAKE_TOOLCHAIN_FILE=/Users/jianxi.zjx/Library/Android/sdk/ndk-bundle/build/cmake/android.toolchain.cmake -DANDROID_ABI=armeabi-v7a -DANDROID_ARM_NEON=ON -DAPP_PLATFORM=19 ..
make -j

64位编译：
cmake -DCMAKE_TOOLCHAIN_FILE=/Users/jianxi.zjx/Library/Android/sdk/ndk-bundle/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=ON -DAPP_PLATFORM=19 ..
make -j
