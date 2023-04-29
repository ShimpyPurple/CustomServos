#include "CustomServos.h"

ServoManager::ServoManager( uint8_t timer ):
    began( false ) ,
    numServos( 0 ) ,
    servos( new Servo*[0] ) ,
    timer( new GenericTimer(timer) )
{}

ServoManager::ServoManager( BaseTimer16 *timer16 ):
    began( false ) ,
    numServos( 0 ) ,
    servos( new Servo*[0] ) ,
    timer( new GenericTimer(timer16) )
{}

ServoManager::ServoManager( BaseTimer8Async *timer8 ):
    began( false ) ,
    numServos( 0 ) ,
    servos( new Servo*[0] ) ,
    timer( new GenericTimer(timer8 , true) )
{}

ServoManager::ServoManager( GenericTimer *timer ):
    began( false ) ,
    numServos( 0 ) ,
    servos( new Servo*[0] ) ,
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
    
    for ( uint8_t i=0 ; i<numServos ; ++i ) {
        if ( servos[i]->pin == pin ) {
            servos[i]->ocrb = percent/100*125 + 38;
            return;
        }
    }
    
    if ( numServos == 100 ) return; // Thats too many. stopit.
    Servo **oldServos = servos;
    numServos += 1;
    servos = new Servo*[numServos];
    for ( uint8_t i=0 ; i<numServos-1 ; ++i ) {
        servos[i] = oldServos[i];
    }
    servos[ numServos-1 ] = new Servo( pin , percent/100*125 + 38 );
    pinMode( pin , OUTPUT );
    delete[] oldServos;
}

void ServoManager::remove( uint8_t pin ) {
    uint8_t pinIndex = 255;
    for ( uint8_t i=0 ; i<numServos ; ++i ) {
        if ( servos[i]->pin == pin ) {
            pinIndex = i;
            break;
        }
    }
    if ( pinIndex == 255 ) return;
    
    Servo **oldServos = servos;
    numServos -= 1;
    servos = new Servo*[numServos];
    for ( uint8_t i=0 ; i<numServos+1 ; ++i ) {
        if ( i < pinIndex ) {
            servos[i] = oldServos[i];
        } else if ( i > pinIndex ) {
            servos[i-1] = oldServos[i];
        } else {
            delete oldServos[i];
        }
    }
    delete[] oldServos;
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
