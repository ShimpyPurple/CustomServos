#include "CustomServos.h"

ServoManager::ServoManager( uint8_t timer ):
    began( false ) ,
    timer( new GenericTimer(timer) )
{}

ServoManager::ServoManager( BaseTimer16 *timer16 ):
    began( false ) ,
    timer( new GenericTimer(timer16) )
{}

ServoManager::ServoManager( BaseTimer8Async *timer8 ):
    began( false ) ,
    timer( new GenericTimer(timer8 , true) )
{}

ServoManager::ServoManager( GenericTimer *timer ):
    began( false ) ,
    timer( timer )
{}

void ServoManager::begin() {
    began = true;
    
    if ( timer->reserve() ) {
        timerReserved = true;
    } else {
        timerReserved = false;
        return;
    }
    
    timer->setMode( CTC_OCA );
    timer->setClockSource( CLOCK_256 );
    timer->setOutputCompareA( 1250 );
    timer->setOutputCompareB( UINT16_MAX );
    timer->attachInterrupt( COMPARE_MATCH_A , compAISR , this );
    timer->attachInterrupt( COMPARE_MATCH_B , compBISR , this );
}

void ServoManager::kill() {
    if ( timerReserved ) {
        timer->release();
    }
}

void ServoManager::write( uint8_t pin , float percent ) {
    if ( percent > 100 ) percent = 100;
    if ( percent < 0 ) percent = 0;
    
    if ( !began ) begin();
    
    
}

void ServoManager::remove( uint8_t pin ) {
    
}

static void ServoManager::compAISR( void *object ) {
    ServoManager *sm = ( ServoManager* )( object );
    
    
}

static void ServoManager::compBISR( void *object ) {
    ServoManager *sm = ( ServoManager* )( object );
    
    
}
