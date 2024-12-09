<?php

declare(strict_types=1);

namespace Demo;

/**
 * Service interface for handling billing operations
 */
interface BillingService
{
    /**
     * Charges an order
     *
     * @return string The charge confirmation message
     */
    #[WeekendBlock]
    public function chargeOrder(): string;
}
