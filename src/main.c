#define F_CPU 1200000UL
#define DELAY 1000 /** Delay for hold in run state, bypass for disconnecting from rails */

/**
 * avrdude -P usb -p t13 -c dragon_hvsp -t
 **/

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

volatile unsigned int count0;
volatile unsigned int count3;
volatile unsigned int run;

/**
* Set start attributes
*/
void start() {
    TCCR0B = (1 < CS00) | (1 << CS02);

    PORTB |= (1 << PB3);
    PORTB |= (1 << PB4);
    run = 1;
}

/**
* Set stop attributes
*/
void stop() {
    TCCR0B = 0;

    PORTB &= ~(1 << PB3);
    PORTB |= (1 << PB4);
    run = 0;
}

/**
* Chip init attributes
*/
void cpuInit(void) {
    DDRB = 0x00;
    /** output pins */
    DDRB |= (1 << PB3) | (1 << PB4);
    /** input pins */
    DDRB &= ~(1 << PB1); /* pin PB1 input */

    /** Nastaveni citace pro blikani */
    TCCR0B |= (1 << CS00) | (1 << CS02);
    TIMSK0 = 0;
    TIMSK0 = (1 << TOIE0);

    /** externall interupt change from INT0 on PC1 (PB1)
     * for better circuit and servo connection */
    MCUCR |= (1 << ISC01);
    GIMSK |= (1 << PCIE);
    PCMSK |= (1 << PCINT1);

    count0 = 0;
    count3 = DELAY;
    run = 0;

    /** enable all interrupts */
    sei();
}

/**
 * Externall interrupt on PCINT0
 */
ISR(PCINT0_vect) {
    // simply set this counter
    count3 = DELAY;
}

/**
 * Timer 0 overflow for blinking led
 */
ISR(TIM0_OVF_vect) {
    count0++;
    if (run) {
        if (count0 == 5) {
            PORTB ^= 1 << PB4;
        } else
        if (count0 == 10) {
            PORTB ^= 1 << PB4;
            count0 = 0;
        }
    }
}

int main(void) {
    cpuInit();

    PORTB |= (1 << PB3);
    PORTB |= (1 << PB4);

    while(1) {
        // wait and decreese the counter
        _delay_ms(1);
        count3--;

        if (count3 < 1) {
            count3 = 1;
            if (run) {
                stop();
            }
        } else {
            if (!run) {
                start();
            }
        }
    }
}
