#define F_CPU 8000000UL /** Lfuse (ATtiny85) must be set on 0xE2 */
#define DELAY 1000 /** Delay fof hold in run state, bypass for disconnecting from rails */
#define EXTERNALINT /** Use externall interrupt for change state or button */

/**
* White LED is connected to PB5 (Externall reset pin)
* For white LED enable, you need to disable externall reset
* in HVSP mode (set Fuse High Byte)
* RSTDISBL = 0 (programmed)
*
* avrdude -P usb -p t85 -c dragon_hvsp -t
* w hfuse 0 0x5f
**/

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

volatile uint16_t sample;
volatile unsigned int count0;
volatile unsigned int count2;
volatile unsigned int count3;
volatile unsigned int servoPos;
volatile unsigned int run;

unsigned int servoMin = 568; // 570 = 1ms
unsigned int servoMax = 605; // 600 = 2ms


unsigned int debounce(volatile uint8_t *port, uint8_t pin) {
    if (!(*port & (1 << pin))) {
        _delay_ms(80);
        if ( *port & (1 << pin) ) {
            _delay_ms(80);
            return 1;
        }
    }
    return 0;
}

/**
* Set start attributes
*/
void start() {
    TCCR0A = (1 << COM0A1) | (1 << COM0A0) | (1 << WGM01) | (1 << WGM00);
    TCCR0B = (1 < WGM02) | (1 << CS00);

    TCCR1 |= (1 << CS10);
    PORTB &= ~(1 << PB3);
    PORTB |= (1 << PB4);
    sample = 0;
    run = 1;
}

/**
* Set stop attributes
*/
void stop() {
    TCCR0A = 0;
    TCCR0B = (1 < WGM02) | (1 << CS00);// | (1 << CS02);

    TCCR1 |= (1 << CS10);
    PORTB |= (1 << PB3);
    PORTB |= (1 << PB4);
    OCR0A = 0;
    run = 0;
}

/**
* Chip init attributes
*/
void cpuInit(void) {
    DDRB = 0x00;
    /** output pins */
    DDRB |= (1 << PB0) | (1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5);
    /** input pins */
    DDRB &= ~(1 << PB1); /* pin PB1 input */

    /** timer 0 and pwm for playing sound */
    TCCR0A = (1 << COM0A1) | (1 << COM0A0) | (1 << WGM01) | (1 << WGM00);
    //TCCR0B = (1 < WGM02) | (1 << CS00);
    TCCR0B = (1 < WGM02) | (1 << CS00) | (1 << CS02);
    OCR0A = 0;
    TIMSK |= (1 << TOIE0);

    /** timer 1 servo controler */
    TCCR1 |= (1 << CTC1);// | (1 << CS10);
    OCR1A = 4;
    TCNT1 = 0;
    TIMSK |= (1 << OCIE1A);
    //TCCR1 &= ~(1 << CS10);

#ifdef EXTERNALINT
    /** externall interupt change from INT0 on PC1 (PB1)
     * for better circuit and servo connection */
    MCUCR |= (1 << ISC01);
    GIMSK |= (1 << PCIE);
    PCMSK |= (1 << PCINT1);
#endif

    count0 = 0;
    count2 = 0;
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
* Timer 1 overflow, servo driver
*/
ISR(TIMER1_COMPA_vect) {
}

/**
* Timer 0 overflow for playing sound
*/
ISR(TIMER0_OVF_vect) {

    count0++;
    if (run) {
        if (count0 == 4) {

            count0 = 0;

            if(sample > pcm_length) {
                sample = 0;
                PORTB ^= 1 << PB3;
                PORTB ^= 1 << PB4;
            }
            if (sample == pcm_length / 2) {
                PORTB ^= 1 << PB3;
                PORTB ^= 1 << PB4;
            }
        }
    } else {
        if (count0 == 15000) {
            PORTB &= ~(1 << PB5);
        } else
        if (count0 == 30000) {
            PORTB |= 1 << PB5;
            count0 = 0;
        }
    }
}

int main(void) {
    cpuInit();

    PORTB |= (1 << PB3);
    PORTB |= (1 << PB4);
    PORTB |= (1 << PB5);

    while(1) {

#ifdef EXTERNALINT

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
#else
        if (debounce(&PINB, PB1)) {
            if (!run) {
                start();
            } else {
                stop();
            }
        }
#endif
    }
}
