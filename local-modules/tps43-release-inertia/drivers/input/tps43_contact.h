/* SPDX-License-Identifier: MIT */
#pragma once
#include <zephyr/sys/atomic.h>
/* ABS_MISC's standard code; Zephyr 3.5 does not define the symbolic name. */
#define TPS43_INPUT_CONTACT_CODE 0x28

/* Fail closed until a valid sensor report confirms that all fingers are up. */
extern atomic_t tps43_contact_present;
