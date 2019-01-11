/*
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <avr/sleep.h>

typedef enum _Mode
{
    Armed,
    ButtonWait2Press,
    Control,
    Disarmed,
    Fired,
    LowPower
} Mode;

const uint16_t waitControl=128;
const uint16_t waitArmed=2048;
const uint16_t waitDisarmed=1900;

volatile Mode mode;
volatile uint16_t counter=0;
volatile uint8_t motorCounter=0;

void switchLed()
{
    PORTD ^= _BV(PD3);
}

bool isLedOn()
{
    return PORTD & _BV(PD3);
}

void oNLed()
{
    PORTD |= _BV(PD3);
}

void oFFLed()
{
    PORTD &= ~_BV(PD3);
}


bool buttonPressed()
{
    return ! ( PIND & _BV(PD2) );
}

void turnLeft()
{
        PORTD|= _BV(PD4);
        motorCounter=6;
}

void turnRight()
{
        PORTD|= _BV(PD4);
        PORTD|= _BV(PD5);
        motorCounter=6;
}

void stop()
{
        PORTD&= ~(_BV(PD4) | _BV(PD5));
}

void blink(bool state)
{
    static uint8_t ledCounter=0;
    if(state)
    {
        if(++ledCounter==5)
        {
            switchLed();
            ledCounter=0;
        }
    }
    else
    {
        switchLed();
    }
}

ISR(TIMER0_OVF_vect)
{
    static uint8_t rate=1;
    switch(mode)
    {
        case LowPower:
        {
            static uint8_t ledCnt=0;
            if(++ledCnt<=6)
            {
                switchLed();
            }
            if(ledCnt==12)
            {
                ledCnt=0;
            }
            break;
        }
        case Disarmed:
        {
            blink(!isLedOn());
            break;
        }
        case Armed:
        {
            switchLed();
            if (buttonPressed())
            {
                counter=0;
                mode=ButtonWait2Press;
            }
            break;
        }
        case ButtonWait2Press:
        {
            switchLed();
            rate += !buttonPressed() == ( (counter >> 2) !=0);

            if(counter==16)
            {
                if(rate>=12)
                {
                     mode = Control;
                     oNLed();
                }
                else
                {
                    mode=Armed;
                }
                rate=1;
                counter=0;
            }
            break;
        }
        case Control:
        {
            blink(isLedOn());
            if(motorCounter)
            {
                break;
            }
            if(buttonPressed())
            {
                    counter=0;
                    static bool isMoveUp=false;
                    if(isMoveUp)
                    {
                        turnLeft();
                    }
                    else
                    {
                        turnRight();
                    }
                    isMoveUp=!isMoveUp;
                    break;
            }
            if(counter>waitControl)
            {
                mode=Armed;
            }
            break;
        }
    }
    if(motorCounter && --motorCounter==0)
    {
        stop();
    }

    if(++counter==waitArmed)
    {
        TIMSK  = 0;
        TCCR0B = 0;
        oFFLed();

        GIMSK=_BV(INT0);
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();
    }
}


void init()
{
    stop();
    DDRD   = _BV(PD3)   | _BV(PD4) | _BV(PD5);

    TIMSK  =_BV(TOIE0);
    TCCR0B =_BV(CS02)   | _BV(CS00);
    GIMSK=0;
}

bool isTriggered()
{
    return PINB & _BV(PB2);
}


void arm()
{
        if(isTriggered())
        {
            counter=waitDisarmed;
            mode=Disarmed;
        }
        else
        {
            mode=Armed;
            counter=0;
        }
}

ISR(INT0_vect)
{
    init();
    arm();
}


int main(void)
{
    init();
    if(MCUSR & _BV(BORF))
    {
        MCUSR ^= _BV(BORF);
        counter=waitDisarmed;
        mode=LowPower;
    }
    else
    {
        arm();
    }

    sei();

    while(1)
    {
        if(isTriggered() && mode==Armed)
        {
            mode=Fired;
            counter=waitDisarmed;
            oNLed();
            turnLeft();
        }
    }
    return 0;
}
