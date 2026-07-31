cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/MyApp/android-ndk-r27d/build/cmake/android.toolchain.cmake -DANDROID_TOOLCHAIN=clang -DANDROID_NATIVE_API_LEVEL=30 -DANDROID_STL=c++_shared -DANDROID_ABI=arm64-v8a -DANDROID_CPP_FEATURES=rtti
cd build
ninja -j12

adb logcat -c
adb push better_auto_player /data/local/tmp
adb shell "su -c chmod 777 /data/local/tmp/better_auto_player"
start "" /B "adb" "shell" "su -c /data/local/tmp/better_auto_player"

cd ..

adb logcat -s shasha_ap