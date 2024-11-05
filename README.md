# Ray.Aop PHP Extension

[![Build and Test PHP Extension](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml/badge.svg)](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml)

<img src="https://ray-di.github.io/images/logo.svg" alt="ray-di logo" width="150px;">

Low-level PHP extension that provides core method interception functionality for [Ray.Aop](https://github.com/ray-di/Ray.Aop). While this extension can be used standalone, it is designed to be a foundation for Ray.Aop's more sophisticated AOP features.

## Features

- Efficient low-level method interception
- Support for intercepting final classes and methods
- Full parameter and return value modification support
- Works seamlessly with the `new` keyword
- Thread-safe operation support

## Requirements

- PHP 8.1 or higher
- Linux, macOS, or Windows with appropriate build tools
- Thread-safe PHP build recommended for multi-threaded environments

## Installation

1. Clone the repository:
```bash
git clone https://github.com/ray-di/ext-rayaop.git
cd ext-rayaop
```

2. Build and install the extension:
```bash
phpize
./configure
make
make install
```

3. Add the following line to your php.ini file:
```ini
extension=rayaop.so  # For Unix/Linux
extension=rayaop.dll # For Windows
```

4. Verify installation:
```bash
php -m | grep rayaop
```

## Design Decisions

This extension provides minimal, high-performance method interception capabilities:

- One interceptor per method: The extension supports a single active interceptor per method, with the last registered interceptor taking precedence
- Final class support: Can intercept final classes and methods, unlike pure PHP implementations
- Raw interception: No built-in matching or conditions (use Ray.Aop for these features)
- Thread-safe: Safe to use in multi-threaded environments like PHP-FPM

## Relationship with Ray.Aop

This extension provides low-level method interception, while [Ray.Aop](https://github.com/ray-di/Ray.Aop) offers high-level AOP features:

Ray.Aop provides:
- Conditional interception using Matchers
- Multiple interceptors per method
- Attribute/Annotation based interception
- Sophisticated AOP features

When both are used together:
- Ray.Aop handles the high-level AOP logic
- This extension provides the low-level interception mechanism
- Ray.Aop automatically utilizes this extension when available for better performance

## Basic Usage

### Simple Interceptor
```php
class LoggingInterceptor implements Ray\Aop\MethodInterceptorInterface
{
    public function intercept(object $object, string $method, array $params): mixed
    {
        echo "Before {$method}\n";
        $result = $object->$method(...$params);
        echo "After {$method}\n";
        return $result;
    }
}

// Register the interceptor
method_intercept(TestClass::class, 'testMethod', new LoggingInterceptor());
```

### Method Interception Setup
```php
// Initialize the interception system
method_intercept_init();

// Enable method interception
method_intercept_enable(true);

// Register interceptors
method_intercept(MyClass::class, 'myMethod', new MyInterceptor());
```

## Development

### Build Script
```bash
./build.sh clean   # Clean build environment
./build.sh prepare # Prepare build environment
./build.sh build   # Build extension
./build.sh run     # Run extension
./build.sh all     # Execute all steps
```

### Testing
```bash
make test
```

For specific tests:
```bash
make test TESTS="-v tests/your_specific_test.phpt"
```

## License

[MIT License](LICENSE)

## Author

Akihito Koriyama

This extension was developed with the assistance of AI pair programming, which helped navigate the complexities of PHP extension development and PECL standards.
