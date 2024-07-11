--TEST--
RayAOP extension is loaded
--FILE--
<?php
var_dump(extension_loaded('rayaop'));
var_dump(function_exists('method_intercept'));
var_dump(function_exists('method_intercept_init'));
var_dump(function_exists('enable_method_intercept'));
var_dump(interface_exists('Ray\\Aop\\MethodInterceptorInterface'));

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
