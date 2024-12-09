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
- Comprehensive error handling and debugging capabilities
- Memory-efficient design with proper resource management

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

; Optional: Enable debug mode
; rayaop.debug_level = 1
```

4. Verify installation:
```bash
php -m | grep rayaop
```

## API Reference

### Core Functions

#### method_intercept_init()
Initializes the interception system. Must be called before registering any interceptors.

```php
bool method_intercept_init()
```

Returns `true` on success, `false` on failure.

#### method_intercept()
Registers an interceptor for a specific class method.

```php
bool method_intercept(string $class_name, string $method_name, Ray\Aop\MethodInterceptorInterface $interceptor)
```

- If an interceptor is already registered for the method, it will be replaced
- Returns `true` on success, `false` on failure

#### method_intercept_enable()
Enables or disables method interception globally.

```php
void method_intercept_enable(bool $enable)
```

### Thread Safety

The extension is fully thread-safe and uses the following mechanisms:

- Mutex-based resource protection
- Thread-local storage for global state
- Safe initialization and cleanup in multi-threaded environments

When using in multi-threaded environments (e.g., PHP-FPM):
- Ensure ZTS (Zend Thread Safety) is enabled in PHP
- Each thread maintains its own interception state
- Resource cleanup is handled automatically per thread

## Error Handling

### Error Codes

The extension defines the following error codes:

- `RAYAOP_E_MEMORY_ALLOCATION (1)`: Memory allocation failure
- `RAYAOP_E_HASH_UPDATE (2)`: Hash table update failure
- `RAYAOP_E_INVALID_HANDLER (3)`: Invalid interceptor handler
- `RAYAOP_E_MAX_DEPTH_EXCEEDED (4)`: Maximum interception depth exceeded
- `RAYAOP_E_NULL_POINTER (5)`: Null pointer error
- `RAYAOP_E_INVALID_STATE (6)`: Invalid internal state

Errors are reported through PHP's error reporting system. Enable error reporting to catch and handle these errors.

## Debug Mode

Debug mode can be enabled by setting the debug level in php.ini:

```ini
rayaop.debug_level = 1
```

Debug output includes:
- Interceptor registration events
- Method interception traces
- Resource allocation/deallocation
- Error conditions and stack traces

## Performance Considerations

### Execution Depth

The extension limits the maximum interception depth to prevent infinite recursion:
```php
#define MAX_EXECUTION_DEPTH 100
```

Consider this limit when designing nested interceptors.

### Memory Management

The extension implements careful memory management:
- Automatic cleanup of interceptor resources
- Proper reference counting for PHP objects
- Immediate resource release when interceptors are replaced
- Cleanup on request shutdown

### Performance Impact

Method interception adds minimal overhead:
- Direct method calls: ~0.1-0.2μs additional overhead
- Intercepted calls with simple interceptors: ~1-2μs overhead
- Complex interceptors may add more overhead depending on their implementation

Performance tips:
- Register interceptors during application initialization
- Avoid registering/unregistering interceptors frequently
- Use simple interceptors for performance-critical code
- Consider disabling interception when not needed

## Design Decisions

This extension provides minimal, high-performance method interception capabilities:

- One interceptor per method: The extension supports a single active interceptor per method, with the last registered interceptor taking precedence
    - When registering a new interceptor for a method, the previous one is automatically unregistered
    - This design ensures predictable behavior and optimal performance
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

// Initialize and enable interception
method_intercept_init();
method_intercept_enable(true);

// Register the interceptor
method_intercept(TestClass::class, 'testMethod', new LoggingInterceptor());
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

## Known Limitations

1. Single interceptor per method
2. Maximum execution depth of 100 levels
3. No built-in pattern matching for method interception
4. Interceptors must be registered individually for each method

## License

[MIT License](LICENSE)

## Author

Akihito Koriyama

This extension was developed with the assistance of AI pair programming, which helped navigate the complexities of PHP extension development and PECL standards.

## Acknowledgments

This project was created using [JetBrains CLion](https://www.jetbrains.com/clion/), which is available for free with an [Open Source License](https://www.jetbrains.com/community/opensource/).

We'd like to express our gratitude to JetBrains for providing such a powerful and user-friendly development environment.