--TEST--
Test method_intercept_init function
--SKIPIF--
<?php
if (!extension_loaded("rayaop")) {
    die("skip RayAOP extension is not loaded.");
}
?>
--FILE--
<?php
// Ensure the RayAOP extension is loaded
if (!extension_loaded("rayaop")) {
    die("RayAOP extension is not loaded.");
}

function test_intercept_init() {
    // Initialize intercept table
    method_intercept_init();
    // Intercept a method
    $interceptor = new class {
        public function intercept($object, $method, $params) {
            echo "Intercepted: " . get_class($object) . "::" . $method . "\n";
        }
    };

    // Register intercept
    method_intercept("Test", "testMethod", $interceptor);

    // Call the method to ensure interception works
    $testObject = new class {
        public function testMethod() {
            echo "Original method executed.\n";
        }
    };

    $testObject->testMethod();

    // Re-initialize intercept table
    method_intercept_init();

    // Call the method again to ensure no interception after init
    $testObject->testMethod();
}

class Test {
    public function testMethod() {
        echo "Original method executed.\n";
    }
}

// Run the test
test_intercept_init();
?>
--EXPECT--
Original method executed.
Original method executed.
