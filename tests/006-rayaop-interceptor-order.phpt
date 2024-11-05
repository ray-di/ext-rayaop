--TEST--
RayAOP interceptor execution order verification
--FILE--
<?php
// Test to verify the execution order of multiple interceptors
// This test ensures that interceptors are executed in the correct order (LIFO)
class OrderTestInterceptor implements Ray\Aop\MethodInterceptorInterface {
    private $order;
    public function __construct($order) {
        $this->order = $order;
    }
    public function intercept(object $object, string $method, array $params): mixed {
        echo "Interceptor {$this->order} start\n";
        $result = $object->$method(...$params);
        echo "Interceptor {$this->order} end\n";
        return $result;
    }
}

class TestClass {
    public function method() {
        echo "Original method\n";
    }
}

// Register multiple interceptors in sequence
method_intercept(TestClass::class, 'method', new OrderTestInterceptor(1));
method_intercept(TestClass::class, 'method', new OrderTestInterceptor(2));

$test = new TestClass();
$test->method();
?>
--EXPECT--
Interceptor 2 start
Original method
Interceptor 2 end
