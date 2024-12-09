<?php

declare(strict_types=1);

namespace Demo;

use Ray\Aop\MethodInterceptor;
use Ray\Aop\MethodInvocation;
use RuntimeException;
use function getdate;
use const PHP_EOL;

class WeekendBlocker implements MethodInterceptor
{
    /** {@inheritDoc} */
    public function invoke(MethodInvocation $invocation)
    {
        $today = getdate();
        if ($today['weekday'][0] === 'S') {
            throw new RuntimeException(
                $invocation->getMethod()->getName() . ' not allowed on weekends!'
            );
        }

        echo $invocation->getMethod()->getName() . ' allowed on weekday.' . PHP_EOL;

        return $invocation->proceed();
    }
}
