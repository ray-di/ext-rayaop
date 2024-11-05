--TEST--
RayAOP maximum recursion and error handling
--SKIPIF--
<?php
if (!extension_loaded('rayaop')) die('skip rayaop extension not available');
?>
--FILE--
<?php
// Test for maximum execution depth and error handling
// This test verifies that the extension properly handles recursive method calls
// and prevents infinite recursion
class RecursiveInterceptor implements Ray\Aop\MethodInterceptorInterface {
    public function intercept(object $object, string $method, array $params): mixed {
        // Deliberately cause recursive interceptor call
        return $object->$method(...$params);
    }
}

class TestClass {
    public function recursiveMethod() {
        return $this->recursiveMethod();
    }
}

method_intercept(TestClass::class, 'recursiveMethod', new RecursiveInterceptor());
$test = new TestClass();
try {
    $test->recursiveMethod();
} catch (Throwable $e) {
    echo "Caught expected error:" . $e::class  . "\n";
}

// Test handling of NULL parameters
// Verifies proper error handling for invalid input
try {
    @method_intercept(null, null, null);
} catch (Throwable $e) {
    echo "Caught null parameter error\n";
}
?>
--EXPECT--
Caught expected error:Error
Caught null parameter error
