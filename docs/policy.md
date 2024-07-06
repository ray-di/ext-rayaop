# Ray.Aop PECL Extension Implementation Policy

## 1. Implementation of Interception Functionality

- Utilize Zend Engine hooks to implement functionality that intercepts calls to specified classes and methods.
- Construct a mechanism that wraps original method calls and invokes intercept handlers.

## 2. Implementation of MethodInterceptorInterface

- Enable C-level processing of the MethodInterceptorInterface defined in PHP userland.
- Properly handle the invocation of the intercept method, allowing execution of user-defined interception logic.

## 3. Implementation of method_intercept Function

- Create a mechanism to internally manage interception information by receiving class name, method name, and MethodInterceptorInterface object.
- Design an efficient data structure to store and quickly access registered interception information.

## 4. Performance Optimization

- Consider efficient code generation and caching mechanisms to minimize overhead caused by interception processing.
- Implement optimizations to skip interception processing when unnecessary.

## 5. PHP 8 Compatibility

- Initially support only PHP 8, without considering compatibility with PHP 7.
- Keep in mind extensible design for potential future PHP 7 support.

## 6. Memory Management

- Properly manage reference counts of interception information and objects to prevent memory leaks.

## 7. Error Handling

- Appropriately handle invalid arguments and runtime errors, integrating with PHP's error handling mechanism.

## 8. Extensibility

- Aim for a modularized design to accommodate future feature extensions and changes.

## 9. Userland Matching

- Assume that matching processes will be performed in userland.
- The PECL extension will call intercept handlers rather than directly calling interceptors.

## 10. Invocation of Intercept Handlers

- Call registered intercept handlers when interception occurs.
- Allow intercept handlers to control the invocation of interceptors, enabling integration with interceptors other than Ray.Aop.

By proceeding with implementation based on these policies, we will develop an efficient and reliable Ray.Aop PECL extension.