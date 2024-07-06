# Ray.Aop PHP Extension

[![Build and Test PHP Extension](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml/badge.svg)](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml)

<img src="https://ray-di.github.io/images/logo.svg" alt="ray-di logo" width="150px;">

A PHP extension that provides Aspect-Oriented Programming (AOP) functionality for method interception, designed to complement Ray.Aop.

## Features

- Efficient method interception for specific classes
- Apply custom logic before and after method execution
- Works with `final` classes and methods

## Requirements

- PHP 8.1 or higher

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

This PECL extension is designed to enhance Ray.Aop by providing method interception capabilities at a lower level. While it can be used independently, it's primarily intended to be used in conjunction with Ray.Aop for optimal functionality.

Key points:
- The extension intentionally binds only one interceptor per method for simplicity and performance.
- Multiple interceptor chaining should be implemented in PHP code, either using Ray.Aop or custom implementation.
- The main advantages are the ability to intercept `final` classes/methods and unrestricted use of the `new` keyword.

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
        echo "Before method execution\n";
        $result = call_user_func_array([$object, $method], $params);
        echo "After method execution\n";
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

$interceptor = new MyInterceptor();
method_intercept('TestClass', 'testMethod', $interceptor);

$test = new TestClass();
$result = $test->testMethod("test");
echo "Final result: $result\n";
```

Output:
```
Before method execution
TestClass::testMethod(test) called
After method execution
Final result: Result: test
```

## Integration with Ray.Aop

For more complex AOP scenarios, it's recommended to use this extension in combination with [Ray.Aop](https://github.com/ray-di/Ray.Aop). Ray.Aop provides a higher-level API for managing multiple interceptors and more advanced AOP features.

## The Power of AOP

Aspect-Oriented Programming (AOP) is a powerful paradigm that complements Object-Oriented Programming (OOP) in building more flexible and maintainable software systems. By using AOP:

1. **Separation of Concerns**: You can cleanly separate cross-cutting concerns (like logging, security, or transaction management) from your core business logic.

2. **Enhanced Modularity**: AOP allows you to modularize system-wide concerns that would otherwise be scattered across multiple classes.

3. **Improved Code Reusability**: Aspects can be reused across different parts of your application, reducing code duplication.

4. **Easier Maintenance**: By centralizing certain behaviors, AOP can make your codebase easier to maintain and evolve over time.

5. **Non-invasive Changes**: You can add new behaviors to existing code without modifying the original classes, adhering to the Open/Closed Principle.

6. **Dynamic Behavior Modification**: With this PECL extension, you can even apply aspects to final classes and methods, providing unprecedented flexibility in your system design.

By combining the strengths of OOP and AOP, developers can create more robust, flexible, and easier-to-maintain software architectures. This PECL extension, especially when used in conjunction with Ray.Aop, opens up new possibilities for structuring your PHP applications, allowing you to tackle complex problems with elegance and efficiency.

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

