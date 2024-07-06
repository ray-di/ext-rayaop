### Ray.Aop PECL Extension Specification

#### Overview
Ray.Aop is a PECL extension that provides Aspect-Oriented Programming (AOP) capabilities to PHP. This specification outlines the features, usage, and interfaces of the Ray.Aop extension.

#### Features
1. **Interception Mechanism**:
   The `intercept` method of the specified intercept handler is called at runtime for designated classes and methods.

2. **Intercept Handlers**:
   Intercept handlers are used to insert custom logic before and after method execution.

#### Interface

##### MethodInterceptorInterface

Ray.Aop provides an interface for managing interception. This interface should be implemented by PHP users to insert custom logic for specific classes and methods.

- **Namespace**: `Ray\Aop`
- **Method**:
    - `intercept(object $object, string $method, array $params): mixed`

#### Function

##### method_intercept
Registers an intercept handler for the specified class and method.

- **Function Name**: `method_intercept`
- **Parameters**:
    - `string $className`: Target class name
    - `string $methodName`: Target method name
    - `Ray\Aop\MethodInterceptorInterface $handler`: Intercept handler to register
- **Return Value**: `bool` (true if registration is successful, false if it fails)

#### Usage

1. **Implementing an Intercept Handler**:
   Create a class that implements `Ray\Aop\MethodInterceptorInterface`.

```php
<?php

namespace Ray\Aop;

class MyInterceptor implements MethodInterceptorInterface
{
    public function intercept(object $object, string $method, array $params): mixed
    {
        // Pre-execution logic
        echo "Before method execution\n";
        
        // Call the original method
        $result = call_user_func_array([$object, $method], $params);
        
        // Post-execution logic
        echo "After method execution\n";
        
        return $result;
    }
}
```

2. **Registering an Intercept Handler**:
   Use the `method_intercept` function to register an intercept handler for a specific class and method.

```php
<?php

$interceptor = new Ray\Aop\MyInterceptor();
$result = method_intercept('MyClass', 'myMethod', $interceptor);

if ($result) {
    echo "Interceptor registered successfully\n";
} else {
    echo "Failed to register interceptor\n";
}
```

3. **Interception in Action**:
   When a method of the registered class is called, the `intercept` method of the intercept handler is automatically executed.

```php
<?php

$myObject = new MyClass();
$myObject->myMethod(); // This call will trigger the interceptor
```

#### Important Notes

- The intercept handler is executed every time the target method is called.
- If multiple intercept handlers are registered for the same method, only the last registered one will be effective.
- When calling the original method within an intercept handler, always use `call_user_func_array` or `call_user_func`.
- The extension does not currently support method invocation chaining or multiple interceptors per method.
- Interceptors are applied globally and affect all instances of the intercepted class.