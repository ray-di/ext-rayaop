# Ray.Aop PHP Extension

[![Build and Test PHP Extension](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml/badge.svg)](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml)

<img src="https://ray-di.github.io/images/logo.svg" alt="ray-di logo" width="150px;">

Low-level PHP extension that provides core method interception functionality for [Ray.Aop](https://github.com/ray-di/Ray.Aop). While this extension can be used standalone, it is designed to be a foundation for Ray.Aop's more sophisticated AOP features.

## Features

- Method interception with comprehensive parameter access and return value modification
- Support for intercepting final classes and methods
- Works seamlessly with the `new` keyword
- Thread-safe operation support in ZTS builds
- Debug mode with configurable levels
- Memory-efficient interception handling

## Requirements

- PHP 8.1 or higher
- Linux, macOS, or Windows with appropriate build tools
- Thread-safe PHP build recommended for production use

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

3. Add the following configuration to your php.ini file:
```ini
; Required extension loading
extension=rayaop.so  # For Unix/Linux
extension=rayaop.dll # For Windows

; Optional configuration
[rayaop]
rayaop.debug_level = 0       ; 0=disabled, 1=basic, 2=verbose
```

4. Verify installation:
```bash
php -m | grep rayaop
```

## Security Considerations

### Thread Safety
The extension is designed to be thread-safe and includes proper locking mechanisms for global state. When using in a threaded environment (e.g., with PHP-FPM), ensure you're using the Thread-Safe (ZTS) version of PHP.

### Memory Management
- The extension implements careful memory management with proper allocation and deallocation
- Includes protection against memory leaks in interceptor chains
- Implements safeguards against infinite recursion
- Uses secure hash table operations for interceptor storage

### Best Practices
1. Always validate interceptor objects before registration
2. Implement proper error handling in interceptors
3. Avoid storing sensitive data in interceptor instances
4. Set appropriate memory and recursion limits in production

## Design Decisions

This extension provides minimal, high-performance method interception capabilities:

- One interceptor per method: The extension supports a single active interceptor per method, with the last registered interceptor taking precedence
- Final class support: Can intercept final classes and methods, unlike pure PHP implementations
- Raw interception: No built-in matching or conditions (use Ray.Aop for these features)
- Thread-safe design: All global state is properly protected
- Zero-copy parameter passing where possible

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

## Advanced Usage

### Error Handling
```php
class ErrorHandlingInterceptor implements Ray\Aop\MethodInterceptorInterface
{
    public function intercept(object $object, string $method, array $params): mixed
    {
        try {
            return $object->$method(...$params);
        } catch (Throwable $e) {
            // Handle or transform the error
            throw new CustomException("Error in {$method}: " . $e->getMessage());
        }
    }
}
```

### Parameter Modification
```php
class ParamValidatorInterceptor implements Ray\Aop\MethodInterceptorInterface
{
    public function intercept(object $object, string $method, array $params): mixed
    {
        // Validate and potentially modify parameters
        $sanitizedParams = array_map(
            fn($param) => is_string($param) ? htmlspecialchars($param) : $param,
            $params
        );
        
        return $object->$method(...$sanitizedParams);
    }
}
```

## API Reference

### Core Functions

#### method_intercept()
```php
bool method_intercept(string $class_name, string $method_name, object $interceptor)
```
Registers an interceptor for a specific class method. Only one interceptor can be active per method - registering a new one will replace any existing interceptor.
- **Parameters:**
    - `$class_name`: Fully qualified class name to intercept
    - `$method_name`: Name of the method to intercept
    - `$interceptor`: Object implementing Ray\Aop\MethodInterceptorInterface
- **Returns:** bool - True on successful registration, False if registration fails
- **Errors:** May generate E_WARNING on invalid handlers or memory allocation failures

#### method_intercept_init()
```php
bool method_intercept_init()
```
Initializes or resets the interception system. Cleans up any existing interceptors and reinitializes the interceptor storage.
- **Returns:** bool - True if initialization succeeds, False if memory allocation fails
- **Errors:** May generate E_ERROR on memory allocation failures

#### method_intercept_enable()
```php
void method_intercept_enable(bool $enable)
```
Enables or disables the method interception system globally. When disabled, intercepted methods will execute normally.
- **Parameters:**
    - `$enable`: True to enable the interception system, False to disable it
- **Note:** Disabling interception does not remove registered interceptors

### Basic Configuration Example
```php
// Initialize the interception system
method_intercept_init();

// Enable method interception
method_intercept_enable(true);

// Register an interceptor
$result = method_intercept(
    MyClass::class,
    'targetMethod',
    new MyInterceptor()
);

if (!$result) {
    // Handle registration failure
}
```

## Performance Considerations

### Memory Usage
- Each interceptor registration uses approximately 200 bytes of memory
- The extension maintains a hash table for quick interceptor lookups
- Memory usage scales linearly with the number of intercepted methods

### Performance Considerations
- Method interception adds some overhead to each method call
- The extension implements efficient parameter handling
- Interceptor lookup is optimized using hash tables
- Execution depth is tracked to prevent infinite recursion


## Troubleshooting

### Common Issues

1. Segmentation Faults
    - Ensure Thread-Safe PHP in multi-threaded environments
    - Check memory limits and recursion depth
    - Verify interceptor object validity

2. Memory Leaks
    - Implement proper cleanup in interceptors
    - Use method_intercept_init() when necessary
    - Monitor memory usage with debug_level = 1

3. Performance Issues
    - Review interceptor complexity
    - Check recursion depth settings
    - Monitor interception patterns

### Debug Mode
Enable debug mode for detailed logging:
```ini
rayaop.debug_level = 2
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

### Debug Builds
```bash
./configure --enable-debug
make clean
make
```

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Add appropriate tests for your changes
4. Ensure all tests pass (`make test`)
5. Run memory leak checks (`make test TESTS="-m")
6. Commit your changes
7. Push to the branch
8. Create a Pull Request

## Support and Feedback

- Issues: Report bugs via GitHub Issues
- Questions: Use GitHub Discussions
- Security: Report security issues directly to maintainers
- Performance: Share benchmarks and optimization suggestions

## License

[MIT License](LICENSE)

## Credits

- Akihito Koriyama 