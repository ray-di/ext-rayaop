# HelloWorld PHP Extension

[![Build and Test PHP Extension](https://github.com/koriym/ext-rayaop/actions/workflows/build.yml/badge.svg)](https://github.com/koriym/ext-rayaop/actions/workflows/build.yml)

A simple PHP extension that demonstrates basic and advanced "Hello World" functionality.

## Features

- Basic `rayaop()` function
- Advanced `rayaop_advanced()` function with configurable greeting

## Run

1. Compile the extension:

    ```
    // make clean
    // phpize --clean
    phpize
    ./configure --enable-rayaop-advanced
    make
    ```

2. Run

    ```
    % php -d extension=./modules/rayaop.so -i | grep rayaop

    rayaop
    rayaop support => enabled
    rayaop.greeting => Hello AOP! => Hello AOP!

    % php -d extension=./modules/rayaop.so php/rayaop.php
    Hello AOP!
    Hello AOP!
   ```

## Usage

### Basic Function

```php
<?php
rayaop();
// Output: Hello AOP!
```

### Advanced Function

```php
<?php
rayaop_advanced();
// Output: [Configurable greeting from php.ini]

```

To configure the greeting, add the following to your php.ini:

```
rayaop.greeting = "Your custom greeting!"
```

## Build script

You can use the build.sh script for various build operations:

```sh
./build.sh clean   # Clean the build environment
./build.sh prepare # Prepare the build environment
./build.sh build   # Build the extension
./build.sh run     # Run the extension
./build.sh all     # Execute all the above steps
```