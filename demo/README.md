# Ray.Aop Demo

This is a demonstration of [Ray.Aop](https://github.com/ray-di/Ray.Aop), an Aspect Oriented Programming (AOP) framework for PHP.

## Requirements

- PHP 8.1 or higher
- rayaop.so PHP extension (must be in ../modules/rayaop.so)

## Running the Demo

The demo can be run in several ways:

1. Using run.php:
```bash
php run.php
```

2. Directly with extension loading:
```bash
php -dextension=../modules/rayaop.so aop.php
```

3. With additional PHP settings:
```bash
php -dextension=../modules/rayaop.so -dzend_extension=xdebug.so -dxdebug.mode=debug aop.php
```

## Project Structure

- `aop.php`: Main demo file that showcases AOP functionality
- `src/`: Source code directory containing the demo classes
