<?php

class MethodInterceptor implements Ray\Aop\MethodInterceptorInterface
{
    public function intercept(object $object, string $method, array $params): mixed
    {
        echo "Intercepted: " . get_class($object) . "::{$method}\n";
        echo "Arguments: " . json_encode($params) . "\n";

        // 元のメソッドを呼び出し、結果を返す
        return call_user_func_array([$object, $method], $params);
    }
}

$interceptor = new MethodInterceptor();
method_intercept('TestClass', 'testMethod', $interceptor);

class TestClass
{
    public function __construct()
    {
        echo "TestClass constructed\n";
    }

    public function testMethod($arg)
    {
        echo "TestClass::testMethod($arg) called\n";
        return "Result: $arg";
    }

    public function nonInterceptedMethod($arg)
    {
        echo "TestClass::nonInterceptedMethod($arg) called\n";
        return "Non-intercepted result: $arg";
    }
}

echo "Creating TestClass instance\n";
$test = new TestClass();

echo "Calling testMethod (should be intercepted)\n";
$result1 = $test->testMethod("test");
echo "Result: $result1\n";

echo "\nCalling nonInterceptedMethod (should not be intercepted)\n";
$result2 = $test->nonInterceptedMethod("test");
echo "Result: $result2\n";

echo "\nScript execution completed\n";

