<?php

namespace Ray\Aop;

/**
 * Method Interceptor Interface
 *
 * This interface defines the contract for method interceptors in the Ray.Aop framework.
 */
interface MethodInterceptorInterface
{
    /**
     * Intercept method
     *
     * This method is called when an intercepted method is invoked.
     *
     * @param object       $object The object on which the method was called
     * @param string       $method The name of the method being called
     * @param array<mixed> $params An array of parameters passed to the method
     *
     * @return mixed The result of the method invocation
     */
    public function intercept(object $object, string $method, array $params): mixed;
}
