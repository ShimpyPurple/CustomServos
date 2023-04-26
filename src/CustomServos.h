#ifndef CustomServos_h
#define CustomServos_h

#include "Arduino.h"

#if !defined( __AVR_ATmega328P__ ) && !defined( __AVR_ATmega2560__ )
#warning "CustomServos is only tested for ATmega328P and ATmega2560"
#endif

#define TIMER_1 1
#define TIMER_2 2
#if defined( __AVR_ATmega2560__ )
#define TIMER_3 3
#define TIMER_4 4
#define TIMER_5 5
#endif

class ServoManager {
	public:
		ServoManager( uint8_t timer );
}

#endif