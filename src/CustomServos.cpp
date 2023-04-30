#include "CustomServos.h"

ServoManager::ServoManager( uint8_t timer ):
    begun( false ) ,
    timer( new GenericTimer(timer , true) ) ,
    cycleIndex( 0 )
{}

ServoManager::ServoManager( BaseTimer16 *timer16 ):
    begun( false ) ,
    timer( new GenericTimer(timer16) ) ,
    cycleIndex( 0 )
{}

ServoManager::ServoManager( BaseTimer8Async *timer8 ):
    begun( false ) ,
    timer( new GenericTimer(timer8 , true) ) ,
    cycleIndex( 0 )
{}

ServoManager::ServoManager( GenericTimer *timer ):
    begun( false ) ,
    timer( timer ) ,
    cycleIndex( 0 )
{}

void ServoManager::begin() {
    begun = true;
    
    if ( timer->reserve() ) {
        timerReserved = true;
    } else {
        timerReserved = false;
        return;
    }
    
    timer->setMode( NORMAL );
    timer->setCounter( 0 );
    timer->setOutputCompareA( 0 );
    timer->setOutputCompareB( 0xFF );
    timer->attachInterrupt( COMPARE_MATCH_A , compAISR , this );
    timer->attachInterrupt( COMPARE_MATCH_B , compBISR , this );
    timer->setClockSource( CLOCK_8 );
}

void ServoManager::kill() {
    if ( timerReserved ) {
        timer->release();
    }
    for ( uint8_t i=0 ; i<MAX_SERVOS ; ++i ) {
        durrations[i] = 0;
    }
}

void ServoManager::write( uint8_t pin , float percent ) {
    if ( percent > 100 ) percent = 100;
    if ( percent < 0 ) percent = 0;
    
    if ( !begun ) begin();
    
    uint16_t durration = percent/100 * (MAX_PULSE - MIN_PULSE) + MIN_PULSE ;
    
    uint8_t insertIndex = 0xFF;
    for ( uint8_t i=0 ; i<MAX_SERVOS ; ++i ) {
        if ( durrations[i] == 0 ) {
            insertIndex = i;
        } else if ( pins[i] == pin ) {
            durrations[i] = durration;
            return;
        }
    }
    if ( insertIndex != 0xFF ) {
        pins[insertIndex] = pin;
        durrations[insertIndex] = durration;
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

static void ServoManager::compAISR( void *object ) {
    ServoManager *sm = ( ServoManager* )( object );
    
    if ( sm->cycleIndex == MAX_SERVOS-1 ) {
        sm->timer->setCounter( 0 );
        sm->cycleIndex = 0;
    } else {
        sm->cycleIndex += 1;
    }
    
    sm->timer->setOutputCompareA( (sm->cycleIndex + 1) * CYCLE_LENGTH );
    if ( sm->durrations[sm->cycleIndex] != 0 ) {
        sm->timer->setOutputCompareB( sm->cycleIndex * CYCLE_LENGTH + sm->durrations[sm->cycleIndex] );
        digitalWrite( sm->pins[sm->cycleIndex] , HIGH );
    }
}

static void ServoManager::compBISR( void *object ) {
    ServoManager *sm = ( ServoManager* )( object );
    
    if ( sm->durrations[sm->cycleIndex] != 0 ) {
        digitalWrite( sm->pins[sm->cycleIndex] , LOW );
    }
}
