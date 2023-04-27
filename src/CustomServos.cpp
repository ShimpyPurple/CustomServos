#include "CustomServos.h"

ServoManager::ServoManager( uint8_t timer ):
    servos( new Servo*[0] ) ,
    tcnt8ExtraByte( 0 ) ,
    ocra8ExtraByte( 0 ) ,
    ocrb8ExtraByte( 0 ) ,
    numServos( 0 )
{
    switch ( timer ) {
        case TIMER_1: this->timer = new GenericTimer( &Timer1 ); break;
        case TIMER_2: this->timer = new GenericTimer( &Timer2 ); break;
#if defined( __AVR_ATmega2560__ )
        case TIMER_3: this->timer = new GenericTimer( &Timer3 ); break;
        case TIMER_4: this->timer = new GenericTimer( &Timer4 ); break;
        case TIMER_5: this->timer = new GenericTimer( &Timer5 ); break;
#endif
        default: timerReserved = false; return;
    }
}

ServoManager::ServoManager( BaseTimer16 *timer16 ):
    servos( new Servo*[0] ) ,
    timer( new GenericTimer(timer16) ) ,
    tcnt8ExtraByte( 0 ) ,
    ocra8ExtraByte( 0 ) ,
    ocrb8ExtraByte( 0 ) ,
    numServos( 0 )
{}

ServoManager::ServoManager( BaseTimer8Async *timer8 ):
    servos( new Servo*[0] ) ,
    timer( new GenericTimer(timer8) ) ,
    tcnt8ExtraByte( 0 ) ,
    ocra8ExtraByte( 0 ) ,
    ocrb8ExtraByte( 0 ) ,
    numServos( 0 )
{}

ServoManager::ServoManager( GenericTimer *timer ):
    servos( new Servo*[0] ) ,
    timer( timer ) ,
    tcnt8ExtraByte( 0 ) ,
    ocra8ExtraByte( 0 ) ,
    ocrb8ExtraByte( 0 ) ,
    numServos( 0 )
{}

void ServoManager::begin() {
    if ( timer->isFree() ) {
        timer->reserve();
        timerReserved = true;
    } else {
        timerReserved = false;
        return;
    }
    
    switch( timer->getTimerType() ) {
        case TIMER_16_BIT:
            timer->setMode( CTC_OCA );
            timer->setClockSource( CLOCK_256 );
            timer->setOutputCompareA( 1250 );
            timer->setOutputCompareB( UINT16_MAX );
            timer->attachInterrupt( COMPARE_MATCH_A , timer16CompA , this );
            timer->attachInterrupt( COMPARE_MATCH_B , timer16CompB , this );
            break;
        case TIMER_8_BIT_ASYNC:
            timer->setMode( NORMAL );
            timer->setClockSource( CLOCK_256 );
            ocra8ExtraByte = ( 1250>>8 );
            timer->setOutputCompareA( 1250 & 0xFF );
            ocrb8ExtraByte = ( 0xFF );
            timer->setOutputCompareB( 0xFF );
            timer->attachInterrupt( COMPARE_MATCH_A , timer8CompA , this );
            timer->attachInterrupt( COMPARE_MATCH_B , timer8CompB , this );
            timer->attachInterrupt( OVERFLOW , timer8Overflow , this );
            break;
    }
}

void ServoManager::kill() {
    if ( timerReserved ) {
        timer->release();
    }
}

void ServoManager::write( uint8_t pin , float percent ) {
    if ( percent > 100 ) percent = 100;
    if ( percent < 0 ) percent = 0;
    
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

static void ServoManager::timer16CompA( void *object ) {
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

static void ServoManager::timer16CompB( void *object ) {
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

static void ServoManager::timer8CompA( void *object ) {
    ServoManager *sm = ( ServoManager* )( object );
    
    if ( sm->tcnt8ExtraByte != sm->ocra8ExtraByte ) return;
    
    uint16_t next = UINT16_MAX;
    for ( uint8_t i=0 ; i<sm->numServos ; ++i ) {
        digitalWrite( sm->servos[i]->pin , HIGH );
        if ( sm->servos[i]->ocrb < next ) {
            next = sm->servos[i]->ocrb;
        }
    }
    
    sm->ocrb8ExtraByte = ( next>>8 );
    sm->timer->setOutputCompareB( next & 0xFF );
    
    sm->tcnt8ExtraByte = 0;
    sm->timer->setCounter( 0 );
}

static void ServoManager::timer8CompB( void *object ) {
    ServoManager *sm = ( ServoManager* )( object );
    
    if ( sm->tcnt8ExtraByte != sm->ocrb8ExtraByte ) return;
    
    uint16_t tcnt = sm->timer->getCounter() | ( (uint16_t)(sm->tcnt8ExtraByte) << 8 );
    uint16_t next = UINT16_MAX;
    
    for ( uint8_t i=0 ; i<sm->numServos ; ++i ) {
        if ( tcnt - sm->servos[i]->ocrb < 20 ) {
            digitalWrite( sm->servos[i]->pin , LOW );
        }
        if ( (sm->servos[i]->ocrb > tcnt) && (sm->servos[i]->ocrb < next) ) {
            next = sm->servos[i]->ocrb;
        }
    }
    
    sm->ocrb8ExtraByte = ( next>>8 );
    sm->timer->setOutputCompareB( next );
}

static void ServoManager::timer8Overflow( void *object ) {
    ( (ServoManager*)(object) )->tcnt8ExtraByte += 1;
}
