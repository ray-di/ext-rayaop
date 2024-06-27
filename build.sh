#!/bin/bash

set -x  # Enable debug mode

clean() {
    echo "Cleaning..."
    make clean
    phpize --clean
}

prepare() {
    echo "Preparing..."
    phpize
    ./configure CFLAGS="-g -O0"
}


build() {
    echo "Building..."
    make
}

install() {
    echo "Installing..."
    make install
}

run() {
    echo "Run..."
    php -dextension=./modules/rayaop.so rayaop.php
}

prepate_lldb() {
    echo "Configure with lldb"
    ./configure CFLAGS="-g -O0"
}

run_lldb() {
   echo "Run with lldb"
   lldb -- php -dextension=./modules/rayaop.so rayaop.php
}

case $1 in
    clean)
        clean
        ;;
    prepare)
        prepare
        ;;
    build)
        build
        ;;
    install)
        install
        ;;
    run)
        run
        ;;
    all)
        clean
        prepare
        build
        run
        ;;
    lldb)
        clean
        prepate_lldb
        build
        run_lldb
        ;;
    *)
        echo "Usage: $0 {clean|prepare|build|install|run|all}"
        exit 1
        ;;
esac
