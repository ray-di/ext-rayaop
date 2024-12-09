<?php

declare(strict_types=1);

namespace Demo;

interface BillingService
{
    #[WeekendBlock]
    public function chargeOrder();
}
