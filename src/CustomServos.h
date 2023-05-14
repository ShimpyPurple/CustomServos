#ifndef CustomServos_h
#define CustomServos_h

#include "Arduino.h"
#include "CustomTimers.h"

#if !defined( __AVR_ATmega328P__ ) && !defined( __AVR_ATmega2560__ )
#warning "CustomServos is only tested for ATmega328P and ATmega2560"
#endif

#define MAX_SERVOS 12
#define MIN_PULSE 1000
#define MAX_PULSE 5000
#define CYCLE_MARGIN 100

#if ( MAX_PULSE + CYCLE_MARGIN ) * MAX_SERVOS > UINT16_MAX
#error ( MAX_PULSE + CYCLE_MARGIN ) * MAX_SERVOS **must** be <= UINT16_MAX
#endif

class ServoManager {
    public:
        ServoManager( uint8_t timer );
        ServoManager( BaseTimer16 *timer16 );
        ServoManager( BaseTimer8Async *timer8 );
        ServoManager( GenericTimer *timer );
        void begin();
        void write( uint8_t pin , float percent );
        void writeMicros( uint8_t pin , uint16_t us );
        void writeTicks( uint8_t pin , uint16_t ticks );
        void remove( uint8_t pin );
        
    
    private:
        GenericTimer *timer;
        uint8_t cycleIndex;
        uint16_t minMicros;
        uint16_t maxMicros;
        uint8_t pins[MAX_SERVOS];
        uint16_t durrations[MAX_SERVOS] = { 0 };
        static void compAISR( void *object );
        static void compBISR( void *object );
    
};

#endif
