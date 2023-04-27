#include "CustomServos.h"

ServoManager::ServoManager( uint8_t timer ):
    tcnt8ExtraByte( 0 ) ,
    ocra8ExtraByte( 0 ) ,
    ocrb8ExtraByte( 0 ) ,
    numServos( 0 )
{
    Serial.print( "Starting Servo Manager @" );
    Serial.print( (uint16_t)this , HEX );
    Serial.println( "..." );
    
    servos = new Servo*[0];
    
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
    
    if ( this->timer->isFree() ) {
        this->timer->reserve();
        timerReserved = true;
    } else {
        timerReserved = false;
        return;
    }
    
    switch( this->timer->getTimerType() ) {
        case TIMER_16_BIT:
            this->timer->setMode( CTC_OCA );
            this->timer->setClockSource( CLOCK_1024 );
            this->timer->setOutputCompareA( 40000 );
            this->timer->setOutputCompareB( UINT16_MAX );
            this->timer->attachInterrupt( COMPARE_MATCH_A , timer16CompA , this );
            this->timer->attachInterrupt( COMPARE_MATCH_B , timer16CompB , this );
            break;
        case TIMER_8_BIT_ASYNC:
            this->timer->setMode( NORMAL );
            this->timer->setClockSource( CLOCK_8 );
            ocra8ExtraByte = ( 40000>>8 );
            // this->timer->setOutputCompareA( 40000 & 0xFF );
            break;
    }
    
    Serial.println( "Servo Manager initialization done" );
}

void ServoManager::write( uint8_t pin , float percent ) {
    Serial.print( "Writing " );
    Serial.print( percent );
    Serial.print( "% to pin " );
    Serial.println( pin );
    
    for ( uint8_t i=0 ; i<numServos ; ++i ) {
        if ( servos[i]->pin == pin ) {
            servos[i]->ocrb = percent/100*2000 + 2000;
            return;
        }
    }
    
    Serial.println( "Pin not found. Adding..." );
    
    if ( numServos == 254 ) return; // Thats too many. stopit.
    Servo **oldServos = servos;
    numServos += 1;
    servos = new Servo*[numServos];
    for ( uint8_t i=0 ; i<numServos-1 ; ++i ) {
        servos[i] = oldServos[i];
    }
    servos[ numServos-1 ] = new Servo( pin , percent/100*2000 + 2000 );
    pinMode( pin , OUTPUT );
    delete[] oldServos;
    
    Serial.print( "Number of servos increased to " );
    Serial.println( numServos );
    for ( uint8_t i=0 ; i<numServos ; ++i ) {
        Serial.print( "  Pin: " );
        Serial.print( servos[i]->pin );
        Serial.print( "  OCRB: " );
        Serial.println( servos[i]->ocrb );
    }
    
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
    
    digitalWrite( sm->servos[0]->pin , LOW );
    
    for ( uint8_t i=0 ; i<sm->numServos ; ++i ) {
        digitalWrite( sm->servos[i]->pin , HIGH );
    }
    
    uint16_t next = UINT16_MAX;
    for ( uint8_t i=0 ; i<sm->numServos ; ++i ) {
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
        if ( sm->servos[i]->ocrb == tcnt ) {
            digitalWrite( sm->servos[i]->pin , LOW );
        }
        if ( (sm->servos[i]->ocrb > tcnt) && (sm->servos[i]->ocrb < next) ) {
            next = sm->servos[i]->ocrb;
        }
    }
    sm->timer->setOutputCompareB( next );
}