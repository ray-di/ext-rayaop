--TEST--
RayAOP void return handling test
--FILE--
<?php
class VoidTestInterceptor implements Ray\Aop\MethodInterceptorInterface {
    public function intercept(object $object, string $method, array $params): mixed {
        echo "Before void method\n";
        $result = $object->$method(...$params);
        echo "After void method\n";
        return $result;
    }
}

class TestClass {
    public function voidMethod() {
        echo "Executing void method\n";
        // 明示的なreturnなし
    }

    public function emptyReturnMethod() {
        echo "Executing empty return method\n";
        return;  // 明示的な空return
    }
}

method_intercept_init();
method_intercept(TestClass::class, 'voidMethod', new VoidTestInterceptor());
method_intercept(TestClass::class, 'emptyReturnMethod', new VoidTestInterceptor());
method_intercept_enable(true);

$test = new TestClass();

echo "Testing void method:\n";
$result = $test->voidMethod();
var_dump($result);

echo "\nTesting empty return method:\n";
$result = $test->emptyReturnMethod();
var_dump($result);

--EXPECT--
Testing void method:
Before void method
Executing void method
After void method
NULL

Testing empty return method:
Before void method
Executing empty return method
After void method
NULL