
set -x

rm -rf $(pwd)/bulid/*

cd $(pwd)/build &&
    cmake ..
    make
    