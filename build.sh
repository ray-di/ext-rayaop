#!/bin/bash

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
    echo "Run smoke..."
    php -dextension=modules/rayaop.so -ddisplay_errors=1 smoke.php
}

demo() {
    echo "Run demo..."
    php -dextension=modules/rayaop.so -ddisplay_errors=1 Ray.Aop/demo/05-pecl.php
}

debug() {
    echo "Run demo..."
    lldb -o run -- php -dextension=modules/rayaop.so -ddisplay_errors=1 Ray.Aop/demo/05-pecl.php
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
    demo)
        demo
        ;;
    debug)
        debug
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
