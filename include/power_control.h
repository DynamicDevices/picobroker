#ifndef __POWER_CONTROL_H_
#define __POWER_CONTROL_H_

// Includes

#include "board_defs.h"

// Definitions

//#define SUPPORT_POWER_CONTROL

#ifdef CONFIG_SUPPORT_POWER_CONTROL
#undef SUPPORT_POWER_CONTROL
#define SUPPORT_POWER_CONTROL CONFIG_SUPPORT_POWER_CONTROL
#endif

#ifdef CONFIG_PWR_CONTROL_PIN
#undef PWR_CONTROL_PIN
#define PWR_CONTROL_PIN CONFIG_PWR_CONTROL_PIN
#endif

#define PWR_CONTROL_PIN_SEL  ((1ULL<<PWR_CONTROL_PIN))

// Prototypes

void init_power_control(void);

#endif // __POWER_CONTROL_H_
