default:
    just --list

config:
    cmake -B build .

build:
    cd build && make

configweb:
    source ../emsdk/emsdk_env.sh && \
    emcmake cmake -B embuild .

buildweb:
    source ../emsdk/emsdk_env.sh && \
    cd embuild && make
