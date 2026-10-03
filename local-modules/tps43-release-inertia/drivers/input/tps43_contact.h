/* SPDX-License-Identifier: MIT */
#pragma once
#include <zephyr/sys/atomic.h>

/* Fail closed until a valid sensor report confirms that all fingers are up. */
extern atomic_t tps43_contact_present;
