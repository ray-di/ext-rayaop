--TEST--
RayAOP final class/method interception
--SKIPIF--
<?php
if (!extension_loaded('rayaop')) die('skip rayaop extension not available');
?>
--FILE--
<?php
// Test interception of final classes and methods - a key feature of RayAOP
final class FinalTestClass {
    final public function finalMethod($arg) {
        return "Final method: $arg";
    }
}

class TestInterceptor implements Ray\Aop\MethodInterceptorInterface {
    public function intercept(object $object, string $method, array $params): mixed {
        echo "Before final method\n";
        $result = $object->$method(...$params);
        echo "After final method\n";
        return $result;
    }
}

method_intercept_init();
// Verify initialization
var_dump(method_intercept_enable());

method_intercept(FinalTestClass::class, 'finalMethod', new TestInterceptor());

$test = new FinalTestClass();
$result = $test->finalMethod("test");
echo "Result: $result\n";

// Test edge case with null parameter
$result = $test->finalMethod(null);
echo "Null test: $result\n";

// Cleanup
method_intercept_enable(false);

?>
--EXPECT--
Before final method
After final method
Result: Final method: test