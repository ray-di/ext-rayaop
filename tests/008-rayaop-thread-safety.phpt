--TEST--
RayAOP concurrent operation safety test with extensive conditions
--FILE--
<?php
echo "<TEST>" . PHP_EOL;

class SafetyTestInterceptor implements Ray\Aop\MethodInterceptorInterface {
    private $id;
    public function __construct($id) {
        $this->id = $id;
    }
    public function intercept(object $object, string $method, array $params): mixed {
        $paramsOutput = $params ? implode(', ', $params) : '';
        echo "<Interceptor {$this->id} executing with params: {$paramsOutput}>" . PHP_EOL;
        $result = $object->$method(...$params);
        echo "<Interceptor {$this->id} completed>" . PHP_EOL;
        return $result;
    }
}

class TestClass {
    public function method($param = null) {
        if ($param === 'error') {
            throw new Exception("Error condition triggered!");
        }
        echo "<Original method executed with param: {$param}>" . PHP_EOL;
        return true;
    }
}

method_intercept_init();

for ($i = 0; $i < 3; $i++) {
    $res = method_intercept(
        TestClass::class,
        'method',
        new SafetyTestInterceptor($i)
    );
    echo "<Interceptor {$i} registered: " . ($res ? 'true' : 'false') . ">" . PHP_EOL;
}

$test = new TestClass();
$paramsArray = [null, 'test', 'error', 'another test'];

foreach ($paramsArray as $i => $param) {
    $paramString = isset($param) ? $param : '';
    echo "<Execution #{$i} with param: {$paramString}>" . PHP_EOL;

    try {
        $result = $test->method($param);
        echo "<Result: " . ($result ? 'true' : 'false') . ">" . PHP_EOL;
    } catch (Exception $e) {
        echo "<Caught Exception: " . $e->getMessage() . ">" . PHP_EOL;
    }
}

echo "<END TEST>" . PHP_EOL;
--EXPECT--
<TEST>
<Interceptor 0 registered: true>
<Interceptor 1 registered: true>
<Interceptor 2 registered: true>
<Execution #0 with param: >
<Interceptor 2 executing with params: >
<Original method executed with param: >
<Interceptor 2 completed>
<Result: true>
<Execution #1 with param: test>
<Interceptor 2 executing with params: test>
<Original method executed with param: test>
<Interceptor 2 completed>
<Result: true>
<Execution #2 with param: error>
<Interceptor 2 executing with params: error>
<Caught Exception: Error condition triggered!>
<Execution #3 with param: another test>
<Interceptor 2 executing with params: another test>
<Original method executed with param: another test>
<Interceptor 2 completed>
<Result: true>
<END TEST>