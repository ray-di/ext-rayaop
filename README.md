# Ray.Aop PHP Extension

[![Build and Test PHP Extension](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml/badge.svg)](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml)

<img src="https://ray-di.github.io/images/logo.svg" alt="ray-di logo" width="150px;">

A high-performance PHP extension that provides Aspect-Oriented Programming (AOP) functionality for method interception.

## Features

- Efficient method interception for specific classes
- Apply custom logic before and after method execution
- Modify method arguments and return values
- Optimized for performance, eliminating the need for CodeGen

## Requirements

- PHP 8.1 or higher
- php-dev package installed

## Installation

1. Clone the repository:
   ```
   git clone https://github.com/ray-di/ext-rayaop.git
   cd ext-rayaop
   ```

2. Build and install the extension:
   ```
   phpize
   ./configure
   make
   make install
   ```

3. Add the following line to your php.ini file:
   ```
   extension=rayaop.so
   ```

## About this Extension

This PECL extension is designed to enhance the performance of Ray.Aop by eliminating the need for CodeGen, resulting in faster execution speeds. While primarily created for Ray.Aop, it can also be used to implement custom AOP solutions independently.

By using this extension, developers can achieve high-performance method interception without the overhead of generating and compiling additional code.

## Usage

### Defining an Interceptor

Create a class that implements the `Ray\Aop\MethodInterceptorInterface`:

```php
namespace Ray\Aop {
    interface MethodInterceptorInterface
    {
        public function intercept(object $object, string $method, array $params): mixed;
    }
}

class MyInterceptor implements Ray\Aop\MethodInterceptorInterface
{
    public function intercept(object $object, string $method, array $params): mixed
    {
        echo "Intercepted: " . get_class($object) . "::{$method}\n";
        echo "Arguments: " . json_encode($params) . "\n";
        
        // Call the original method
        $result = call_user_func_array([$object, $method], $params);
        
        echo "Method execution completed.\n";
        
        return $result;
    }
}
```

### Registering an Interceptor

Use the `method_intercept` function to register an interceptor for a specific class and method:

```php
$interceptor = new MyInterceptor();
method_intercept('TestClass', 'testMethod', $interceptor);
```

### Complete Example

```php
class TestClass
{
    public function testMethod($arg)
    {
        echo "TestClass::testMethod($arg) called\n";
        return "Result: $arg";
    }
}

$test = new TestClass();
$result = $test->testMethod("test");
echo "Result: $result\n";
```

## Build Script

The `build.sh` script provides various operations for building and managing the extension:

```sh
./build.sh clean   # Clean the build environment
./build.sh prepare # Prepare the build environment
./build.sh build   # Build the extension
./build.sh run     # Run the extension
./build.sh all     # Execute all the above steps
```

Use `./build.sh all` for a complete build and installation process.

[... Previous content remains the same ...]

## Running Tests

To run the tests for this extension, use the following command:

```sh
make test
```

This command will execute the PHP extension's test suite, which is the standard method for testing PHP extensions.

If you need to run specific tests or want more verbose output, you can use:

```sh
make test TESTS="-v tests/your_specific_test.phpt"
```

Replace `your_specific_test.phpt` with the actual test file you want to run.

## Performance Considerations

- The extension is optimized for minimal overhead during method interception.
- It bypasses the need for runtime code generation, leading to faster execution.
- Consider the impact of complex interceptor logic on overall performance.
