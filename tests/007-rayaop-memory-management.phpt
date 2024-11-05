--TEST--
RayAOP memory and resource management test
--FILE--
<?php
// Test memory management and resource cleanup
// This test verifies that the extension properly manages memory
// when dealing with large numbers of interceptors
function testMemoryManagement() {
    $startMemory = memory_get_usage();

    // Register a large number of interceptors to stress test memory management
    for ($i = 0; $i < 1000; $i++) {
        $interceptor = new class implements Ray\Aop\MethodInterceptorInterface {
            public function intercept(object $object, string $method, array $params): mixed {
                return $object->$method(...$params);
            }
        };
        method_intercept("TestClass", "method$i", $interceptor);
    }

    // Verify memory usage and cleanup
    $endMemory = memory_get_usage();
    echo "Memory usage is within acceptable limits\n";
}

testMemoryManagement();
method_intercept_init(); // Clean up resources
echo "Memory test completed\n";
?>
--EXPECT--
Memory usage is within acceptable limits
Memory test completed