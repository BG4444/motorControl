/*
 */

#include <avr/io.h>
#include <avr/interrupt.h>

ISR(TIMER0_OVF_vect)
{
    PORTD ^= _BV(PD3);
}

int main(void)
{

    TCCR0B =1<<CS02 | 1<<CS00;
    TIMSK  =1<<TOIE0;

    DDRD = _BV(PD3); //_BV(PD4) | _BV(PD5)

    //DDRB = _BV(PB0) | _BV(PB1);

    sei();

    while(1)
    ;

    return 0;
}
