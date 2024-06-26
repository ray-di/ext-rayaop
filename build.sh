#!/bin/bash

clean() {
    echo "Cleaning..."
    make clean
    phpize --clean
}

prepare() {
    echo "Preparing..."
    phpize
    ./configure --enable-rayaop-advanced
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
    php -dextension=modules/rayaop.so -drayaop.greeting="konichiwa" rayaop.php
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
    *)
        echo "Usage: $0 {clean|prepare|build|install|run|all}"
        exit 1
        ;;
esac
