--TEST--
RayAOP basic functionality
--FILE--
<?php

// Define test class and interceptor
class TestClass {
    public function testMethod($arg) {
        return "Original: " . $arg;
    }
}

class TestInterceptor implements Ray\Aop\MethodInterceptorInterface {
    public function intercept(object $object, string $method, array $params): mixed {
        return "Intercepted: " . $object->$method(...$params);
    }
}

// Register the interceptor
$result = method_intercept(TestClass::class, 'testMethod', new TestInterceptor());
var_dump($result);

// Call the intercepted method
$test = new TestClass();
$result = $test->testMethod("Hello");
var_dump($result);

?>
--EXPECTF--
bool(true)
string(28) "Intercepted: Original: Hello"