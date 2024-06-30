<?php

require __DIR__ . '/php-src/InterceptedInterface.php';

class TestClass
{
    public function testMethod($arg)
    {
        return "Result: $arg";
    }

    public function nonInterceptedMethod($arg)
    {
        return "Non-intercepted result: $arg";
    }
}

class Intercepted implements Ray\Aop\MethodInterceptorInterface
 {
     public function intercept(object $object, string $method, array $params): mixed
     {
         echo "Intercepted: " . get_class($object) . "::{$method}\n";
         echo "Arguments: " . json_encode($params) . "\n";

         return call_user_func_array([$object, $method], $params);
     }
 }

 $intercepted = new Intercepted();
 method_intercept('TestClass', 'testMethod', $intercepted);

$test = new TestClass();
$result1 = $test->testMethod("test");
