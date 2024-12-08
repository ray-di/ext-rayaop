<?php

class MethodInterceptor implements Ray\Aop\MethodInterceptorInterface
{
    public function intercept(object $object, string $method, array $params): mixed
    {
        echo "Intercepted: " . get_class($object) . "::{$method}\n";
        echo "Arguments: " . json_encode($params) . "\n";

        // Call the original method and return the result
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
echo "$result1\n";

echo "\nCalling nonInterceptedMethod (should not be intercepted)\n";
$result2 = $test->nonInterceptedMethod("test");
echo "$result2\n";

echo "\nScript execution completed\n";

$success =  $result1 === 'Result: test' && $result2 === 'Non-intercepted result: test';
echo $success ? 'Test Success.' : 'Test Failure.';
echo PHP_EOL;
exit($success ? 0 : 1);
