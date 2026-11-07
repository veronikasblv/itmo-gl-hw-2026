## Build

- Create and go to build folder `mkdir -p build-release; cd build-release`;
- Run CMake `cmake .. -G <generator-name> -DCMAKE_PREFIX_PATH=<path-to-qt-installation> -DCMAKE_BUILD_TYPE=Release`;
- Run build `ninja -j<number-of-threads-to-build>`;
- Execute `./itmo-gl-hw-2026`.
