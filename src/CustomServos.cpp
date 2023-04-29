#include "CustomServos.h"

ServoManager::ServoManager( uint8_t timer ):
    began( false ) ,
    servos(   new Servo*[0]  ) , numServos( 0 ) ,
    toAdd(    new Servo*[0]  ) , numToAdd( 0 ) ,
    toRemove( new uint8_t[0] ) , numToRemove( 0 ) ,
    timer( new GenericTimer(timer) )
{}

ServoManager::ServoManager( BaseTimer16 *timer16 ):
    began( false ) ,
    servos(   new Servo*[0]  ) , numServos( 0 ) ,
    toAdd(    new Servo*[0]  ) , numToAdd( 0 ) ,
    toRemove( new uint8_t[0] ) , numToRemove( 0 ) ,
    timer( new GenericTimer(timer16) )
{}

ServoManager::ServoManager( BaseTimer8Async *timer8 ):
    began( false ) ,
    servos(   new Servo*[0]  ) , numServos( 0 ) ,
    toAdd(    new Servo*[0]  ) , numToAdd( 0 ) ,
    toRemove( new uint8_t[0] ) , numToRemove( 0 ) ,
    timer( new GenericTimer(timer8 , true) )
{}

ServoManager::ServoManager( GenericTimer *timer ):
    began( false ) ,
    servos(   new Servo*[0]  ) , numServos( 0 ) ,
    toAdd(    new Servo*[0]  ) , numToAdd( 0 ) ,
    toRemove( new uint8_t[0] ) , numToRemove( 0 ) ,
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
    
    if ( numServos+numToAdd-numToRemove == 100 ) return; // Thats too many. stopit.
    
    for ( uint8_t i=0 ; i<numToAdd ; ++i ) {
        if ( toAdd[i]->pin == pin ) {
            delete toAdd[i];
            toAdd[i] = new Servo( pin , percent/100*125 + 38 );
            return;
        }
    }
    
    for ( uint8_t i=0 ; i<numServos ; ++i ) {
        if ( servos[i]->pin == pin ) {
            remove( pin );
        }
    }
    
    Servo **oldToAdd = toAdd;
    numToAdd += 1;
    toAdd = new Servo*[numToAdd];
    for ( uint8_t i=0 ; i<numToAdd-1 ; ++i ) {
        toAdd[i] = oldToAdd[i];
    }
    toAdd[ numToAdd-1 ] = new Servo( pin , percent/100*125 + 38 );
    pinMode( pin , OUTPUT );
    delete[] oldToAdd;
}

void ServoManager::remove( uint8_t pin ) {
    uint8_t *oldToRemove = toRemove;
    numToRemove += 1;
    toRemove = new uint8_t[numToRemove];
    for ( uint8_t i=0 ; i<numToRemove-1 ; ++i ) {
        toRemove[i] = oldToRemove[i];
    }
    toRemove[ numToRemove-1 ] = pin;
    pinMode( pin , OUTPUT );
    delete[] oldToRemove;
}

static void ServoManager::compAISR( void *object ) {
    ServoManager *sm = ( ServoManager* )( object );
    
    uint16_t next = UINT16_MAX;
    for ( uint8_t i=0 ; i<sm->numServos ; ++i ) {
        digitalWrite( sm->servos[i]->pin , HIGH );
        if ( sm->servos[i]->ocrb < next ) {
            next = sm->servos[i]->ocrb;
        }
    }
    sm->timer->setOutputCompareB( next );
}

static void ServoManager::compBISR( void *object ) {
    ServoManager *sm = ( ServoManager* )( object );
    
    uint16_t tcnt = sm->timer->getCounter();
    uint16_t next = UINT16_MAX;
    
    for ( uint8_t i=0 ; i<sm->numServos ; ++i ) {
        if ( tcnt - sm->servos[i]->ocrb < 20 ) {
            digitalWrite( sm->servos[i]->pin , LOW );
        }
        if ( (sm->servos[i]->ocrb > tcnt) && (sm->servos[i]->ocrb < next) ) {
            next = sm->servos[i]->ocrb;
        }
    }
    sm->timer->setOutputCompareB( next );
}
