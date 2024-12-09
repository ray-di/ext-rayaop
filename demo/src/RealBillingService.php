<?php

declare(strict_types=1);

namespace Demo;

use const PHP_EOL;

class RealBillingService implements BillingService
{
    public function chargeOrder(): string
    {
        return 'Charged.' . PHP_EOL;
    }
}
