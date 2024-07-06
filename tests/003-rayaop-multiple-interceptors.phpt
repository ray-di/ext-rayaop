--TEST--
RayAOP multiple interceptors
--SKIPIF--
<?php
if (!extension_loaded('rayaop')) die('skip rayaop extension not available');
?>
--FILE--
<?php
class TestClass {
    public function testMethod($arg) {
        return "Original: " . $arg;
    }
}

class Interceptor1 implements Ray\Aop\MethodInterceptorInterface {
    public function intercept(object $object, string $method, array $params): mixed {
        return "Interceptor1: " . call_user_func([$object, $method], ...$params);
    }
}

class Interceptor2 implements Ray\Aop\MethodInterceptorInterface {
    public function intercept(object $object, string $method, array $params): mixed {
        return "Interceptor2: " . call_user_func([$object, $method], ...$params);
    }
}

// Register multiple interceptors
method_intercept(TestClass::class, 'testMethod', new Interceptor1());
method_intercept(TestClass::class, 'testMethod', new Interceptor2());

$test = new TestClass();
$result = $test->testMethod("Hello");
var_dump($result);

?>
--EXPECT--
string(47) "Interceptor2: Interceptor1: Original: Hello"
