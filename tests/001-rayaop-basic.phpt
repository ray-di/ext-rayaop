--TEST--
RayAOP basic functionality
--SKIPIF--
<?php
if (!extension_loaded('rayaop')) {
    echo 'skip rayaop extension not loaded';
}
?>
--FILE--
<?php
var_dump(interface_exists('\Ray\Aop\MethodInterceptorInterface'));

class TestInterceptor implements \Ray\Aop\MethodInterceptorInterface
{
    public function intercept(object $object, string $method, array $params): mixed
    {
        return "Intercepted: " . $object->$method(...$params);
    }
}

class TestClass
{
    public function testMethod($param = '')
    {
        return "Original" . $param;
    }
}

// Initialize the intercept table
var_dump(method_intercept_init());

// Register the interceptor
var_dump(method_intercept('TestClass', 'testMethod', new TestInterceptor()));

11method_intercept_enable(true);

$test = new TestClass();
echo $test->testMethod(" method called") . "\n";

// Disable method interception
method_intercept_enable(false);
echo $test->testMethod(" method called without interception") . "\n";
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
Intercepted: Original method called
Original method called without interception
