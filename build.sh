cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/MyApp/android-ndk-r27d/build/cmake/android.toolchain.cmake
cd build
ninja -j12
