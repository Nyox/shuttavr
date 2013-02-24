#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define BUTTON_HOLD_THRESH 3 * 4 /* qsec */

static enum {
    STATE_IDLE = 0,
    STATE_HOLDING,
    STATE_RECORDING,
    STATE_PLAYING,
    STATE_PULSING,
} State;

static long qsec_counter;

static long trigger_counter;
static char trigger_mode;
static char pulsing;

int main(void)
{
    DDRB |= _BV(PB2); // Switch, INT0 (still works as output)
    DDRB |= _BV(PB4);
    DDRB |= _BV(PB0);

    // Switch interrupt
    //GIMSK |= _BV(INT0); // ext interrupts
    //MCUCR |= _BV(ISC00); // rising and falling
    //MCUCR |= _BV(ISC01) | _BV(ISC00); // interrupt on rising edge

    // Counter, counter interrupt
    //TCCR1 |= _BV(CTC1); // clear timer on compare match
    //TIMSK |= _BV(OCIE1A); // Enable interrups on ORC1A compare match
    //OCR1A  = 245; // Roughly 0.25 seconds @ 1MHz w/ 1024 prescalar

    #if 0
    // ir comm
    OCR0A   = 20; // ~38.461 KHz @ 1MHz clk, no prescaling
    TCCR0A |= _BV(WGM00); // pwm mode, top == ocra
    TCCR0B |= _BV(WGM02);
    TCCR0A |= _BV(COM0A1) | _BV(COM0A0); // high cntup, low cntdwn
    TCCR0B |= _BV(CS00); // no prescaling
    #else
    OCR0A   = 12; // ~38.461 kHZ @ 1MHz clk, no prescaling
    TCCR0A |= _BV(COM0A0); // toggle oc0a on match
    TCCR0A |= _BV(WGM01); // ctc mode
    TCCR0B |= _BV(CS00); // no prescaling
    #endif

    #define ON  TCCR0A |=  _BV(COM0A0)
    #define OFF TCCR0A &= ~_BV(COM0A0)

    #if 1
    OFF;
    //TCCR1 |= _BV(CTC1); // ctc enable
    TCCR1 |= _BV(CS12) | _BV(CS10); // 16 prescaler
    OCR1B  = 0; // should interrupt immediately
    TIMSK |= _BV(OCIE1B); // interrupt on ocrb match
    sei(); // enable interupts

    //set_sleep_mode(SLEEP_MODE_IDLE);
    //sei();
    for (;;) {
        //sleep_mode();
    }
    #else
    // send this twice
    ON;
    _delay_us(16*125); // 2000
    OFF;
    _delay_us(1024*28); // 27830
    ON;
    _delay_us(16*25); // 400
    OFF;
    _delay_us(16*98); // 1580
    ON;
    _delay_us(16*25); // 400
    OFF;
    _delay_us(16*223); // 3580
    ON;
    _delay_us(16*25); // 400
    OFF;
    //_delay_ms(64);
    #endif
    for(;;) {
    }
    for(;;) {
        flicker_on(77);
        //_delay_us(2000);
        PORTB &= ~_BV(PB0);
        _delay_ms(28);
        flicker_on(15);
        //_delay_us(400);
        PORTB &= ~_BV(PB0);
        _delay_us(1580);
        flicker_on(15);
        //_delay_us(400);
        PORTB &= ~_BV(PB0);
        _delay_us(3580);
        flicker_on(15);
        //_delay_us(400);
        PORTB &= ~_BV(PB0);

        _delay_ms(64);
    }
    set_sleep_mode(SLEEP_MODE_IDLE);
    sei();
    for (;;) {
        sleep_mode();
    }
}

// ir pulser
#if 0
ISR(TIM1_COMPB_vect)
{
    static char stage;
    switch (stage) {
        case 0:
            ON;
            OCR1B = 125; // 2000us
            break;

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 6:
            OFF;
            OCR1B = 255; // 27830us
            break;

        case 7:
            OCR1B = 209; // 27830us
            break;

        case 8:
            ON;
            OCR1B = 25; // 400us
            break;

        case 9:
            OFF;
            OCR1B = 98; // 1580us
            break;

        case 10:
            ON;
            OCR1B = 25; // 400us
            break;

        case 11:
            OFF;
            OCR1B = 224; // 3580us
            break;

        case 12:
            ON;
            OCR1B = 25; // 400us
            break;

        case 13:
            OFF;
            TCCR1 &= ~(_BV(CS12) | _BV(CS10)); // turn off timer
            break;
    }
    stage++;
    TCNT1 = 0;
}
#else 
// irpulser prescale change
ISR(TIM1_COMPB_vect)
{
    static char stage;
    switch (stage) {
        case 0:
            ON;
            OCR1B = 125; // 2000us
            break;

        case 1:
            OFF;
            TCCR1 &= ~(_BV(CS12) | _BV(CS10)); // 16 prescaler off
            TCCR1 |= _BV(CS13) | _BV(CS11) | _BV(CS10); // 1024 prescaler
            OCR1B = 28; // 27830us
            break;

        case 2:
            ON;
            TCCR1 &= ~(_BV(CS13) | _BV(CS11) | _BV(CS10)); // 1024 prescaler off
            TCCR1 |= _BV(CS12) | _BV(CS10); // 16 prescaler
            OCR1B = 25; // 400us
            break;

        case 3:
            OFF;
            OCR1B = 98; // 1580us
            break;

        case 4:
            ON;
            OCR1B = 25; // 400us
            break;

        case 5:
            OFF;
            OCR1B = 224; // 3580us
            break;

        case 6:
            ON;
            OCR1B = 25; // 400us
            break;

        #if 1
        case 7:
            OFF;
            TCCR1 &= ~(_BV(CS12) | _BV(CS10)); // turn off timer
            break;
        #else
        case 7:
            OFF;
            TCCR1 &= ~(_BV(CS12) | _BV(CS10)); // 16 prescaler off
            TCCR1 |= _BV(CS13) | _BV(CS11) | _BV(CS10); // 1024 prescaler
            OCR1B = 62; // 64 ms
            break;

        case 8:
            TCCR1 &= ~(_BV(CS13) | _BV(CS11) | _BV(CS10)); // 1024 prescaler off
            TCCR1 |= _BV(CS12) | _BV(CS10); // 16 prescaler
            stage = -1;
        #endif
    }
    stage++;
    TCNT1 = 0;
    //TIFR |= _BV(OCF1B);
}
#endif

// switch interrupt handler
ISR(INT0_vect)
{
    static char prev;
    char cur;
    cur = PINB & _BV(PB2);

    // Rising edge
    if (!prev && cur) {
        switch (State) {
            case STATE_PULSING:
                PORTB &= ~_BV(PB2);
            default:
                State = STATE_HOLDING;

                // measuring hold time w/ qsec
                qsec_counter = 0;
                // enable timer w/ prescalar of 1024
                TCCR1 |= _BV(CS13) | _BV(CS11) | _BV(CS10);
                break;

            case STATE_RECORDING:
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
                if (qsec_counter < BUTTON_HOLD_THRESH) {
                    State = STATE_IDLE;
                    TCCR1 = 0; // disable counter
                    break;
                }
                State = STATE_RECORDING;

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
        case STATE_PULSING:
            State = STATE_PLAYING;
            PORTB &= ~_BV(PB2);
            GIMSK |= _BV(INT0); // ext interrupts
            break;

        case STATE_PLAYING:
            if (qsec_counter < trigger_counter) break;
            State = STATE_PULSING;
            GIMSK &= ~_BV(INT0); // ext interrupts

            // Pulse the switch to trigger the shutter!
            qsec_counter = 0;
            PORTB |= _BV(PB2);
            break;
    }
}

