#ifndef CustomServos_h
#define CustomServos_h

#include "Arduino.h"
#include "CustomTimers.h"

#if !defined( __AVR_ATmega328P__ ) && !defined( __AVR_ATmega2560__ )
#warning "CustomServos is only tested for ATmega328P and ATmega2560"
#endif

struct Servo {
    uint8_t pin;
    uint16_t ocrb;
    Servo( uint8_t pin , uint16_t ocrb ): pin( pin ) , ocrb( ocrb ) {}
};

class ServoManager {
    public:
        ServoManager( uint8_t timer );
        ServoManager( BaseTimer16 *timer16 );
        ServoManager( BaseTimer8Async *timer8 );
        ServoManager( GenericTimer *timer );
        void begin();
        void kill();
        void write( uint8_t pin , float percent );
        void remove( uint8_t pin );
    
    private:
        uint8_t numServos;
        Servo **servos;
        GenericTimer *timer;
        bool noTimer;
        bool timerReserved;
        static void compAISR( void *object );
        static void compBISR( void *object );
    
};

#endif
