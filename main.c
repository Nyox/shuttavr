/*
 * Open source, AVR based, Nikon intervalometer
 * Kevin Nygaard <http://www.mutablecode.com>
 * Feb 28, 2013
 *
 * Default configuration for AVR's ATtiny25
 */

// Change to the speed of target uc
#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// How long you need to hold the button to enter time record mode
#define BUTTON_HOLD_THRESH 3 * 4 /* qsec */
//#define DEBUG_LEDS /* Uncomment to enable debug LEDS */

static enum {
    STATE_IDLE = 0,
    STATE_HOLDING,
    STATE_RECORDING,
    STATE_PLAYING,
    STATE_PULSING,
    STATE_PULSING_SINGLE,
} State;

static long qsec_counter;

static long trigger_counter;
static char trigger_mode;
static char pulsing;

// convienience for changing prescalers nicely
#define TCCR1_CS1X_SET(vector) do {\
    TCCR1 = TCCR1 & ~(_BV(CS13) | _BV(CS12) | _BV(CS11) | _BV(CS10))\
          | (vector); \
    } while (0)

// ir led timer. must turn on for irled to work
// no prescaler
#define IR_TIMER_ON  do {TCCR0B |=  _BV(CS00);} while (0)
#define IR_TIMER_OFF do {TCCR0B &= ~_BV(CS00);} while (0)

// ir led control (on == ~38khz flicker)
#define IR_ON  do {IR_TIMER_ON;  TCCR0A |=  _BV(COM0A0);} while (0)
#define IR_OFF do {IR_TIMER_OFF; TCCR0A &= ~_BV(COM0A0);} while (0)

#define TCCR1_BUTTON_HOLD_CFG do {\
    /* Enable interrups on ORC1A compare match*/\
    TIMSK &= ~_BV(OCIE1B);\
    TIMSK |=  _BV(OCIE1A);\
    /* enable timer w/ prescalar of 1024 */\
    TCCR1_CS1X_SET(_BV(CS13) | _BV(CS11) | _BV(CS10));\
    TCNT1  = 0;\
    } while (0)

#define TCCR1_IRLED_DELAY_CFG do {\
    /* Enable interrups on ORC1B compare match*/\
    TIMSK &= ~_BV(OCIE1A);\
    TIMSK |=  _BV(OCIE1B);\
    /* enable timer w/ prescalar of 16 */\
    TCCR1_CS1X_SET(_BV(CS12) | _BV(CS10));\
    TCNT1  = 0;\
    OCR1B  = 0;/* immediate interrupt */\
    } while (0)

int main(void)
{
    // Declare outputs
    DDRB |= _BV(PB0); // led pin used by pwm
    #ifdef DEBUG_LEDS
    DDRB |= _BV(PB4);
    DDRB |= _BV(PB3);
    DDRB |= _BV(PB1);
    #endif /* DEBUG_LEDS */

    // Switch interrupt
    GIMSK |= _BV(INT0); // enable int0
    MCUCR |= _BV(ISC00); // int on any logic change

    // Button hold counter
    TCCR1 |= _BV(CTC1); // clear timer on compare match
    OCR1A  = 245; // Roughly 0.25 seconds @ 1MHz w/ 1024 prescalar

    // IR led timer
    OCR0A   = 12; // ~38.461 kHZ @ 1MHz clk, no prescaling
    TCCR0A |= _BV(COM0A0); // toggle oc0a on match
    TCCR0A |= _BV(WGM01); // ctc mode

    // Conserve power until we need to do stuff
    set_sleep_mode(SLEEP_MODE_IDLE);
    sei();
    for (;;) sleep_mode();
}

// IR pulser
// Sends properly timed signals to activate shutter
ISR(TIM1_COMPB_vect)
{
    static char stage;
    switch (stage) {
        default:
            stage = 0;
        case 0:
            IR_ON;
            OCR1B = 125; // 2000us
            break;

        case 1:
            IR_OFF;
            // 1024 prescaler
            TCCR1_CS1X_SET(_BV(CS13) | _BV(CS11) | _BV(CS10));
            OCR1B = 28; // 27830us
            break;

        case 2:
            IR_ON;
            // 16 prescaler
            TCCR1_CS1X_SET(_BV(CS12) | _BV(CS10));
            OCR1B = 25; // 400us
            break;

        case 3:
            IR_OFF;
            OCR1B = 98; // 1580us
            break;

        case 4:
            IR_ON;
            OCR1B = 25; // 400us
            break;

        case 5:
            IR_OFF;
            OCR1B = 224; // 3580us
            break;

        case 6:
            IR_ON;
            OCR1B = 25; // 400us
            break;

        case 7:
            IR_OFF; // ir pulse timer turned off already
            stage = 0;

            switch (State) {
                default: // single shot
                    State = STATE_IDLE;
                    TCCR1_CS1X_SET(0); // turn off ir delay timer
                    break;

                // we are in playback mode
                case STATE_PULSING:
                    State = STATE_PLAYING;
                    TCCR1_BUTTON_HOLD_CFG;
                    break;
            }

            #ifdef DEBUG_LEDS
            PORTB &= ~_BV(PB4);
            #endif /* DEBUG_LEDS */

            return; // We don't want to do the cleanup stuff
    }
    stage++;
    TCNT1 = 0;
}

// switch interrupt handler
ISR(INT0_vect)
{
    static char prev;
    char cur;
    cur = PINB & _BV(PB2); // switch

    // Rising edge
    if (!prev && cur) {
        switch (State) {
            case STATE_PLAYING:
            case STATE_PULSING:
            case STATE_PULSING_SINGLE:
                IR_OFF;
                TCCR1_CS1X_SET(0); // turn off ir delay timer
            default:
                #ifdef DEBUG_LEDS
                PORTB &= ~_BV(PB3);
                PORTB &= ~_BV(PB4);
                #endif /* DEBUG_LEDS */
                State = STATE_HOLDING;

                // measuring hold time w/ qsec
                qsec_counter = 0;
                // Configure TCCR1 to be button hold timer
                TCCR1_BUTTON_HOLD_CFG;
                break;

            case STATE_RECORDING:
                #ifdef DEBUG_LEDS
                PORTB &= ~_BV(PB4);
                PORTB |=  _BV(PB3);
                #endif /* DEBUG_LEDS */
                State = STATE_PLAYING;

                // Latch recorded time
                trigger_counter = qsec_counter;
                // zero these for playback
                TCNT1 = 0;
                qsec_counter = 0;
                break;
        }

    // Falling edge
    } else if (prev && !cur) {
        switch (State) {
            case STATE_PLAYING:
            case STATE_PULSING:
                break;

            default:
                // Threshold not met. Do single shot
                if (qsec_counter < BUTTON_HOLD_THRESH) {
                    State = STATE_PULSING_SINGLE;

                    #ifdef DEBUG_LEDS
                    PORTB ^= _BV(PB1);
                    #endif /* DEBUG_LEDS */

                    TCCR1_IRLED_DELAY_CFG;
                    break;
                }
                State = STATE_RECORDING;
                #ifdef DEBUG_LEDS
                PORTB |= _BV(PB4);
                #endif /* DEBUG_LEDS */

                TCNT1 = 0; // zero counter
                qsec_counter = 0; // recording time in qsec
            break;
        }
    }

    prev = cur;
}

// counter compare match interrupt handler
ISR(TIM1_COMPA_vect)
{
    qsec_counter++;
    switch (State) {
        default:
            break;

        case STATE_PLAYING:
            if (qsec_counter < trigger_counter) break;
            #ifdef DEBUG_LEDS
            PORTB |= _BV(PB4);
            #endif /* DEBUG_LEDS */
            State = STATE_PULSING;

            // Switch the timer to pulse the led
            TCCR1_IRLED_DELAY_CFG;
            qsec_counter = 0;
            break;
    }
}

