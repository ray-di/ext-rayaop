--TEST--
Test case: 008-rayaop-safety.phpt
RayAOP concurrent operation safety test
--SKIPIF--
<?php
if (!extension_loaded('rayaop')) {
    die('skip: rayaop extension not available');
}
?>
--FILE--
<?php
// Test concurrent operation safety
class SafetyTestInterceptor implements Ray\Aop\MethodInterceptorInterface {
    private $id;

    public function __construct($id) {
        $this->id = $id;
    }

    public function intercept(object $object, string $method, array $params): mixed {
        echo "Interceptor {$this->id} executing\n";
        $result = $object->$method(...$params);
        echo "Interceptor {$this->id} completed\n";
        return $result;
    }
}

class TestClass {
    public function method() {
        echo "Original method executed\n";
        return true;
    }
}

// Test rapid registration and execution
method_intercept_init();

// Register multiple interceptors rapidly
for ($i = 0; $i < 3; $i++) {
    $result = method_intercept(
        TestClass::class,
        'method',
        new SafetyTestInterceptor($i)
    );
    echo "Interceptor {$i} registered: " . ($result ? 'true' : 'false') . "\n";
}

// Execute method multiple times
$test = new TestClass();
for ($i = 0; $i < 2; $i++) {
    echo "\nExecution #{$i}:\n";
    $result = $test->method();
    echo "Result: " . ($result ? 'true' : 'false') . "\n";
}

?>
--EXPECT--
Interceptor 0 registered: true
Interceptor 1 registered: true
Interceptor 2 registered: true

Execution #0:
Interceptor 2 executing
Original method executed
Interceptor 2 completed
Result: true

Execution #1:
Interceptor 2 executing
Original method executed
Interceptor 2 completed
Result: true