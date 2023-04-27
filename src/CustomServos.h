#ifndef CustomServos_h
#define CustomServos_h

#include "Arduino.h"
#include "CustomTimers.h"

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

struct Servo {
    uint8_t pin;
    uint16_t ocrb;
    Servo( uint8_t pin , uint16_t ocrb ): pin( pin ) , ocrb( ocrb ) {}
};

class ServoManager {
	public:
		ServoManager( uint8_t timer );
        void write( uint8_t pin , float percent );
        void remove( uint8_t pin );
        
        uint8_t numServos;
        Servo **servos;
    
    private:
        GenericTimer *timer;
        bool timerReserved;
        uint8_t tcnt8ExtraByte;
        uint8_t ocra8ExtraByte;
        uint8_t ocrb8ExtraByte;
        
        static void timer16CompA( void *object );
        static void timer16CompB( void *object );
    
};

#endif
