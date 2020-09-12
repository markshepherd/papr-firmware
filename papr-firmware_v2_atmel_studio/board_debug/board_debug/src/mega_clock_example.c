
 */
#include "compiler.h"

/* set the correct clock frequency based on fuse settings or external clock/crystal
 * has to be done before including delay.h */
#include "conf_clock.h"
#include <util/delay.h>

#ifndef __OPTIMIZE__
#warning "Please use compiler optimization for correct operation of the delay \
functions."
#endif

 /**
 * \brief Maximum possible delay is 262.14 ms / F_CPU in MHz.
 *
 * Please refer to
 * <A href="http://www.nongnu.org/avr-libc/user-manual/group__util__delay.html">
 * libc</A> for more information.
 */
#define MAX_MS_DELAY (262.14 / (F_CPU / 1000000UL))

/**
 * \brief Maximum possible delay is 768 us / F_CPU in MHz.
 *
 * Please refer to
 * <A href="http://www.nongnu.org/avr-libc/user-manual/group__util__delay.html">
 * libc</A> for more information.
 */
#define MAX_US_DELAY (768 / (F_CPU / 1000000UL))

/**
 * \brief Example application on how to use the libc delay functions.
 *
 * This application shows the basic usage of the delay functions from the
 * avr libc library.
 *
 * \note The delay functions will not operate correctly if compiled without
 * optimization.
 */
int main(void)
{
	// busy wait the maximum number of milliseconds
	_delay_ms(MAX_MS_DELAY);

	// busy wait the maximum number of microseconds
	_delay_us(MAX_US_DELAY);

	// busy wait for 10us
	_delay_us(10);

	// busy wait for 10ms
	_delay_ms(10);

	// loop for ever
	while (true);
}
