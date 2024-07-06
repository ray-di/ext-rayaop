--TEST--
RayAOP error handling
--SKIPIF--
<?php
if (!extension_loaded('rayaop')) die('skip rayaop extension not available');
?>
--FILE--
<?php
// Try to register an interceptor for a non-existent class
$result = method_intercept('NonExistentClass', 'someMethod', new stdClass());
var_dump($result);

// Try to register with an invalid interceptor object
class TestClass {
    public function testMethod() {}
}

$result = method_intercept(TestClass::class, 'testMethod', new stdClass());
var_dump($result);

?>
--EXPECTF--
bool(true)
bool(true)
