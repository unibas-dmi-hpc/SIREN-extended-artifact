#!/bin/bash
set -e
set -x

mkdir -p libraries
cd libraries
LIB_PATH=$(pwd)

# Download and install SSDeep
[ -d ssdeep ] || git clone https://github.com/ssdeep-project/ssdeep.git
git -C ssdeep checkout df3b860f8918261b3faeec9c7d2c8a241089e6e6
cd ssdeep
autoreconf -vfi
CFLAGS="-fPIC -std=gnu99" ./configure --prefix=$LIB_PATH
make
make install
cd ..

# Download and install libelf
[ -d libelf ] || git clone https://github.com/WolfgangSt/libelf.git
git -C libelf checkout e71156563fd4779ff77f9297b22b8df4a1f31334
cd libelf
CFLAGS="-fPIC -std=gnu99 -Wno-implicit-int -Wno-error=implicit-int -Wno-implicit-function-declaration -Wno-error=implicit-function-declaration" \
    ./configure --prefix=$LIB_PATH --enable-static --disable-shared --host=$(gcc -dumpmachine)
make
make install
cd ..

# Download xxHash
[ -d xxHash ] || git clone https://github.com/Cyan4973/xxHash.git
git -C xxHash checkout e573d4d2aaeaba0f3e5a0a9a54144a1f2b4b56e7
cd ..

# Build demo files
cd demo
bash make.sh
cd ..

# Build siren shared object
cd code
bash make.sh
cd ..

# Build Go UDP listener (go-sqlite3 requires CGO)
cd listener
CGO_ENABLED=1 go build -o siren_listener .
cd ..