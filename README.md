# Ray.Aop PHP Extension

[![Build and Test PHP Extension](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml/badge.svg)](https://github.com/ray-di/ext-rayaop/actions/workflows/build.yml)

<img src="https://ray-di.github.io/images/logo.svg" alt="ray-di logo" width="150px;">

A PHP extension that provides Aspect-Oriented Programming (AOP) functionality for method interception.

## Features

- Intercept method calls on specific classes
- Apply custom logic before and after method execution
- Modify method arguments and return values

## Requirements

- PHP 8.0 or higher
- php-dev package installed

## Installation

2.Compile the extension:

```
phpize
./configure
make
```

3. Install the extension:

```
sudo make install
```

4. Add the following line to your php.ini file:

```
extension=rayaop.so
```

## About this Extension

This PECL extension is designed to enhance the performance of Ray.Aop by eliminating the need for CodeGen, resulting in faster execution speeds. While it is primarily created for Ray.Aop, it can also be used to implement custom AOP solutions independently of Ray.Aop.

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

### Example

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

## Running Tests

To run the tests, use the following command:

```
php -dextension=./modules/rayaop.so test/rayaop_test.php
```

## Build Script

You can use the build.sh script for various build operations:

```sh
./build.sh clean   # Clean the build environment
./build.sh prepare # Prepare the build environment
./build.sh build   # Build the extension
./build.sh run     # Run the extension
./build.sh all     # Execute all the above steps
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.