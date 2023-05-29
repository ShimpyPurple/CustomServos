#include "CustomServos.h"

#define MIN_PULSE 1000
#define MAX_PULSE 5000
#define CYCLE_MARGIN 100

#if ( MAX_PULSE + CYCLE_MARGIN ) * MAX_SERVOS > UINT16_MAX
#error ( MAX_PULSE + CYCLE_MARGIN ) * MAX_SERVOS **must** be <= UINT16_MAX
#endif

ServoManager::ServoManager( uint8_t timer ):
    timer( new GenericTimer(timer , true) ) ,
    cycleIndex( 0 )
{}

ServoManager::ServoManager( BaseTimer16 *timer16 ):
    timer( new GenericTimer(timer16) ) ,
    cycleIndex( 0 )
{}

ServoManager::ServoManager( BaseTimer8Async *timer8 ):
    timer( new GenericTimer(timer8 , true) ) ,
    cycleIndex( 0 )
{}

ServoManager::ServoManager( GenericTimer *timer ):
    timer( timer ) ,
    cycleIndex( 0 )
{}

void ServoManager::begin() {
    timer->setMode( WGM_NORMAL );
    timer->setCounter( 0 );
    timer->setOutputCompareA( 0 );
    timer->setOutputCompareB( 0xFF );
    timer->attachInterrupt( COMPARE_MATCH_A , compAISR , this );
    timer->attachInterrupt( COMPARE_MATCH_B , compBISR , this );
    timer->setClockSource( CLOCK_8 );
    
    minMicros = CYCLE_MARGIN / timer->getTickRate() * 1e6;
    maxMicros = MAX_PULSE    / timer->getTickRate() * 1e6;
}

void ServoManager::write( uint8_t pin , float percent ) {
    if ( percent > 100 ) percent = 100;
    if ( percent < 0 ) percent = 0;
    
    uint16_t durration = percent/100 * (MAX_PULSE - MIN_PULSE) + MIN_PULSE ;
    writeTicks( pin , durration );
}

void ServoManager::writeMicros( uint8_t pin , uint16_t us ) {
    if ( us > maxMicros ) us = maxMicros;
    if ( us < minMicros ) us = minMicros;
    
    uint16_t durration = us * timer->getTickRate() / 1e6;
    writeTicks( pin , durration );
}

void ServoManager::writeTicks( uint8_t pin , uint16_t ticks ) {
    if ( ticks > MAX_PULSE ) ticks = MAX_PULSE;
    if ( ticks < MIN_PULSE ) ticks = MIN_PULSE;
    
    uint8_t insertIndex = 0xFF;
    for ( uint8_t i=0 ; i<MAX_SERVOS ; ++i ) {
        if ( durrations[i] == 0 ) {
            insertIndex = i;
        } else if ( pins[i] == pin ) {
            durrations[i] = ticks;
            return;
        }
    }
    if ( insertIndex != 0xFF ) {
        pins[insertIndex] = pin;
        durrations[insertIndex] = ticks;
        pinMode( pin , OUTPUT );
    }
}

void ServoManager::remove( uint8_t pin ) {
    for ( uint8_t i=0 ; i<MAX_SERVOS ; ++i ) {
        if ( pins[i] == pin ) {
            durrations[i] = 0;
            return;
        }
    }
}

void ServoManager::compAISR( void *object ) {
    ServoManager *sm = ( ServoManager* )( object );
    
    if ( sm->cycleIndex == MAX_SERVOS-1 ) {
        sm->timer->setCounter( 0 );
        sm->cycleIndex = 0;
    } else {
        sm->cycleIndex += 1;
    }
    
    sm->timer->setOutputCompareA( (sm->cycleIndex + 1) * (MAX_PULSE + CYCLE_MARGIN) );
    if ( sm->durrations[sm->cycleIndex] != 0 ) {
        sm->timer->setOutputCompareB( sm->cycleIndex * (MAX_PULSE + CYCLE_MARGIN) + sm->durrations[sm->cycleIndex] );
        digitalWrite( sm->pins[sm->cycleIndex] , HIGH );
    }
}

void ServoManager::compBISR( void *object ) {
    ServoManager *sm = ( ServoManager* )( object );
    
    if ( sm->durrations[sm->cycleIndex] != 0 ) {
        digitalWrite( sm->pins[sm->cycleIndex] , LOW );
    }
}
