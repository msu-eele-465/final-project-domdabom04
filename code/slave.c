#include "intrinsics.h"
#include "msp430fr2355.h"
#include <msp430.h>
#include <stdbool.h>

#define SLAVE_ADDRESS 0x46
#define PRESS 0b11111111
#define REHEAT 0b11111000
#define DEFROST 0b11111001
#define POPCORN 0b11111010
#define POTATO 0b11111011
#define ENTER_STATE_1 0b10000000
#define ENTER_STATE_2 0b11000000

int i = 0;
int j = 0;
unsigned char index = 0;
int time_remaining = 30000;
char time_input[4] = {-1, -1, -1, -1};
unsigned char cook_power = 10;
volatile unsigned char data[2];
int state = -1; // -1 = not started, 0 = done, 1 = enter time, 2 = enter power, 3 = running

bool time_changed = false;
bool full_transmission = false;
bool input_received = false;
bool enter_state_1 = false;
bool enter_state_2 = false;
bool started = false;
bool stopped = false;
bool silenced = false;
bool reheat = false;
bool defrost = false;
bool popcorn = false;
bool potato = false;

int mins = 0;
int secs = 10;

void slave_setup();
void LED_setup();
void LED_bar_setup();
void piezo_setup();
void LCD_setup();
void timer_setup();

void check_presets();
void start(int, int);
void stop();
void LED_on();
void LED_off();
void set_cook_power(unsigned char);
void beep();

void set_RS(unsigned char);
void set_RW(unsigned char);
void enable_HTL();
void set_DB(unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char);
void read_data();
void return_home();
void clear_display();
void print_cook();
void print_cook_time();
void print_cook_power();
void print_done();
void print_cook_time_input(unsigned char);
unsigned char print_cook_power_input();
void print_time_remaining(int);

void load_0();
void load_1();
void load_2();
void load_3();
void load_4();
void load_5();
void load_6();
void load_7();
void load_8();
void load_9();

void shift_insert(int);
void clear_data();

void test_LED_on() {
    if (!(P4OUT & BIT1)) P4OUT |= BIT1;
}

void test_LED_off() {
    P4OUT &= ~BIT1;
}

int main() {
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer

    slave_setup();
    __enable_interrupt();  // Enable global interrupts

    LED_setup();
    LED_bar_setup();
    piezo_setup();
    LCD_setup();

    P4DIR |= BIT1;
    P4OUT &= ~BIT1;

    while (1) {
        if ((state != -1) && time_remaining <= 0) stop();

        check_presets();

        if (enter_state_1) {
            state = 1;
            print_cook_time();
            clear_data();
            enter_state_1 = false;
        } else if (enter_state_2) {
            state = 2;
            print_cook_power();
            clear_data();
            enter_state_2 = false;
        }

        if (state == 1) {
            while (!started) {
                check_presets();
            }
            started = false;
            clear_data();
            full_transmission = false;
            test_LED_on();
            while (!full_transmission) {}
            full_transmission = false;
            //time_remaining = (60 * data[0]) + data[1];
            time_remaining = 60 * mins + secs;
            start(time_remaining, cook_power);
            test_LED_off();
        } else if (state == 2) {
            while (!started) {
                check_presets();
                //cook_power = print_cook_power_input();
                __delay_cycles(65536); /////////////////////////////////////////////////////
            }
            started = false;
            state = 1;
        } else if (state == 3) {
            while (!time_changed) {}
            print_time_remaining(time_remaining);
            time_changed = false;
            if (stopped) {
                stop();
                stopped = false;
            }
        }

        while (state == 0) {
            beep();
            for (i = 0; i < 15000; i++) {
                __delay_cycles(2096); // -1?
                if (silenced) {
                    clear_data();
                    clear_display();
                    return_home();
                    state = -1;
                    silenced = false;
                    break;
                }
            }
        }
    }
}

void slave_setup() {
    UCB0CTLW0 |= UCSWRST;

    P1SEL1 &= ~BIT3;
    P1SEL0 |= BIT3;

    P1SEL1 &= ~BIT2;
    P1SEL0 |= BIT2;

    UCB0CTLW0 &= ~UCTR;
    UCB0CTLW0 &= ~UCMST;
    UCB0CTLW0 |= UCMODE_3 | UCSYNC;

    UCB0I2COA0 = SLAVE_ADDRESS | UCOAEN;
    UCB0I2COA0 |= UCGCEN;

    UCB0CTLW0 &= ~UCSWRST;

    PM5CTL0 &= ~LOCKLPM5;

    UCB0IE |= UCRXIE0;
}

void LED_setup() {
    P1DIR |= BIT4;
    P1OUT &= ~BIT4;
}

void LED_bar_setup() {
    P3DIR |= BIT0;              //LED1
    P3DIR |= BIT1;              //LED2
    P3DIR |= BIT2;              //LED3
    P3DIR |= BIT3;              //LED4
    P5DIR |= BIT0;              //LED5
    P5DIR |= BIT1;              //LED6
    P5DIR |= BIT2;              //LED7
    P5DIR |= BIT3;              //LED8
    P5DIR |= BIT4;              //LED9
    P3DIR |= BIT4;              //LED10

    P3OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4);
    P5OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4);
}

void piezo_setup() {
    P2DIR |= BIT0;
    P2OUT &= ~BIT0;
}

void LCD_setup() {
    P4DIR |= BIT4 |BIT5 | BIT6 | BIT7;
    P6DIR |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6;

    // Set 8-bit mode
    set_RS(0);
    set_RW(0);
    enable_HTL();
    set_DB(0, 0, 0, 1, 1, 1, 0, 0);
    __delay_cycles(2000);

    // Turn on display, cursor off, not blinking
    set_RS(0);
    set_RW(0);
    enable_HTL();
    set_DB(0, 0, 1, 1, 0, 0, 0, 0);
    __delay_cycles(2000);

    // Clear all characters and reset DDRAM address to 0
    set_RS(0);
    set_RW(0);
    enable_HTL();
    set_DB(1, 0, 0, 0, 0, 0, 0, 0);
    __delay_cycles(2000);

    // Set cursor to move right, no display shift
    set_RS(0);
    set_RW(0);
    enable_HTL();
    set_DB(0, 1, 1, 0, 0, 0, 0, 0);
    __delay_cycles(2000);
}

void timer_setup() {
    TB0CTL = TBSSEL__ACLK | MC__UP | TBCLR;
    TB0CCTL0 |= CCIE;
    TB0CCR0 = 32767;
    __enable_interrupt();
    TB0CCTL0 &= ~CCIFG;
}

void check_presets() {
    if (reheat) {
        start(105, 6);
        reheat = false;
    } else if (defrost) {
        start(300, 3);
        defrost = false;
    } else if (popcorn) {
        start(150, 8);
        popcorn = false;
    } else if (potato) {
        start(240, 9);
        potato = false;
    }
}

void start(int cook_time, int cook_power) {
    clear_data();
    clear_display();
    time_remaining = cook_time;
    timer_setup();
    set_cook_power(cook_power);
    LED_on();
    state = 3;
}

void stop() {
    clear_data();
    print_done();
    time_remaining = 0;
    cook_power = 10;
    set_cook_power(0);
    LED_off();
    state = 0;
}

void LED_on() {
    P1OUT |= BIT4;
}

void LED_off() {
    P1OUT &= ~BIT4;
}

void set_cook_power(unsigned char power) {
    if (power < 0 || power > 10) return;
    switch (power) {
        case 0:
            P3OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4);
            P5OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4);
            break;
        case 1:
            P3OUT |= BIT0;
            P3OUT &= ~(BIT1 | BIT2 | BIT3 | BIT4);
            P5OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4);
            break;
        case 2:
            P3OUT |= BIT0 | BIT1;
            P3OUT &= ~(BIT2 | BIT3 | BIT4);
            P5OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4);
            break;
        case 3:
            P3OUT |= BIT0 | BIT1 | BIT2;
            P3OUT &= ~(BIT3 | BIT4);
            P5OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4);
            break;
        case 4:
            P3OUT |= BIT0 | BIT1 | BIT2 | BIT3;
            P3OUT &= ~BIT4;
            P5OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4);
            break;
        case 5:
            P3OUT |= BIT0 | BIT1 | BIT2 | BIT3;
            P3OUT &= ~BIT4;
            P5OUT |= BIT0;
            P5OUT &= ~(BIT1 | BIT2 | BIT3 | BIT4);
            break;
        case 6:
            P3OUT |= BIT0 | BIT1 | BIT2 | BIT3;
            P3OUT &= ~BIT4;
            P5OUT |= BIT0 | BIT1;
            P5OUT &= ~(BIT2 | BIT3 | BIT4);
            break;
        case 7:
            P3OUT |= BIT0 | BIT1 | BIT2 | BIT3;
            P3OUT &= ~BIT4;
            P5OUT |= BIT0 | BIT1 | BIT2;
            P5OUT &= ~(BIT3 | BIT4);
            break;
        case 8:
            P3OUT |= BIT0 | BIT1 | BIT2 | BIT3;
            P3OUT &= ~BIT4;
            P5OUT |= BIT0 | BIT1 | BIT2 | BIT3;
            P5OUT &= ~BIT4;
            break;
        case 9:
            P3OUT |= BIT0 | BIT1 | BIT2 | BIT3;
            P3OUT &= ~BIT4;
            P5OUT |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4;
            break;
        case 10:
            P3OUT |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4;
            P5OUT |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4;
            break;
    }
}

void beep() {
    for (i = 0; i < 3; i++) {
        P2OUT |= BIT0;
        for (j = 0; j < 100; j++) {
            __delay_cycles(5000);
        }
        P2OUT &= ~BIT0;
        for (j = 0; j < 100; j++) {
            __delay_cycles(5000);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LCD STUFF
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void set_RS(unsigned char bit) {
    if (bit) P4OUT |= BIT7;
    else P4OUT &= ~BIT7;
}

void set_RW(unsigned char bit) {
    if (bit) P4OUT |= BIT6;
    else P4OUT &= ~BIT6;
}

void enable_HTL() {
    P4OUT |= BIT5;
    __delay_cycles(5);
    P4OUT &= ~BIT5;
}

void set_DB(unsigned char bit0, unsigned char bit1, unsigned char bit2, unsigned char bit3, unsigned char bit4, unsigned char bit5, unsigned char bit6, unsigned char bit7) {
    if (bit0) P4OUT |= BIT4;
    else P4OUT &= ~BIT4;

    if (bit1) P6OUT |= BIT6;
    else P6OUT &= ~BIT6;

    if (bit2) P6OUT |= BIT5;
    else P6OUT &= ~BIT5;

    if (bit3) P6OUT |= BIT4;
    else P6OUT &= ~BIT4;

    if (bit4) P6OUT |= BIT3;
    else P6OUT &= ~BIT3;

    if (bit5) P6OUT |= BIT2;
    else P6OUT &= ~BIT2;

    if (bit6) P6OUT |= BIT1;
    else P6OUT &= ~BIT1;

    if (bit7) P6OUT |= BIT0;
    else P6OUT &= ~BIT0;
}

void read_data() {
    set_RS(1);
    set_RW(0);
    enable_HTL();
    set_DB(0, 0, 0, 0, 1, 1, 0, 0);
    __delay_cycles(2000);
    set_RS(0);
}

void return_home() {
    set_RS(0);
    set_RW(0);
    enable_HTL();
    set_DB(0, 1, 0, 0, 0, 0, 0, 0);
    __delay_cycles(2000);
}

void clear_display() {
    set_RS(0);
    set_RW(0);
    enable_HTL();
    set_DB(1, 0, 0, 0, 0, 0, 0, 0);
    __delay_cycles(2000);
}

void print_cook() {
    clear_display();
    return_home();
    set_RS(0);
    set_RW(0);

    enable_HTL();
    set_DB(1, 1, 0, 0, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(1, 1, 1, 1, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(1, 1, 1, 1, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(1, 1, 0, 1, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(0, 0, 0, 0, 0, 1, 0, 0);
    __delay_cycles(2000);
    read_data();
}

void print_cook_time() {
    print_cook();

    enable_HTL();
    set_DB(0, 0, 1, 0, 1, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(1, 0, 0, 1, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(1, 0, 1, 1, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(1, 0, 1, 0, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(0, 1, 0, 1, 1, 1, 0, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(0, 0, 0, 0, 0, 1, 0, 0);
    __delay_cycles(2000);
    read_data();
}

void print_cook_power() {
    print_cook();

    enable_HTL();
    set_DB(0, 0, 0, 0, 1, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(1, 1, 1, 1, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(1, 1, 1, 0, 1, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(1, 0, 1, 0, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(0, 1, 0, 0, 1, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(0, 1, 0, 1, 1, 1, 0, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(0, 0, 0, 0, 0, 1, 0, 0);
    __delay_cycles(2000);
    read_data();
}

void print_done() {
    clear_display();
    return_home();
    set_RS(0);
    set_RW(0);

    enable_HTL();
    set_DB(0, 0, 1, 0, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(1, 1, 1, 1, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(0, 1, 1, 1, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();

    enable_HTL();
    set_DB(1, 0, 1, 0, 0, 0, 1, 0);
    __delay_cycles(2000);
    read_data();
}

void print_cook_time_input(volatile unsigned char digit) {
    shift_insert(digit);
    for (i = 0; i < 2; i++) {
        switch (time_input[i]) {
            case -1:
                enable_HTL();
                set_DB(0, 0, 0, 0, 0, 1, 0, 0);
                __delay_cycles(2000);
                read_data();
                break;
            case '0':
                load_0();
                read_data();
                break;
            case '1':
                load_1();
                read_data();
                break;
            case '2':
                load_2();
                read_data();
                break;
            case '3':
                load_3();
                read_data();
                break;
            case '4':
                load_4();
                read_data();
                break;
            case '5':
                load_5();
                read_data();
                break;
            case '6':
                load_6();
                read_data();
                break;
            case '7':
                load_7();
                read_data();
                break;
            case '8':
                load_8();
                read_data();
                break;
            case '9':
                load_9();
                read_data();
                break;
            default:
                load_0();
                read_data();
                break;
        }
    }
    enable_HTL();
    set_DB(0, 1, 0, 1, 1, 1, 0, 0);
    __delay_cycles(2000);
    read_data();
    for (i = 0; i < 2; i++) {
        switch (time_input[i]) {
            case -1:
                enable_HTL();
                set_DB(0, 0, 0, 0, 0, 1, 0, 0);
                __delay_cycles(2000);
                read_data();
                break;
            case '0':
                load_0();
                read_data();
                break;
            case '1':
                load_1();
                read_data();
                break;
            case '2':
                load_2();
                read_data();
                break;
            case '3':
                load_3();
                read_data();
                break;
            case '4':
                load_4();
                read_data();
                break;
            case '5':
                load_5();
                read_data();
                break;
            case '6':
                load_6();
                read_data();
                break;
            case '7':
                load_7();
                read_data();
                break;
            case '8':
                load_8();
                read_data();
                break;
            case '9':
                load_9();
                read_data();
                break;
            default:
                load_0();
                read_data();
                break;
        }
    }
}

unsigned char print_cook_power_input() {
    clear_display();
    return_home();

    switch (data[0]) {
        case '0':
            load_0();
            read_data();
            return 0;
        case '1':
            load_1();
            read_data();
            return 1;
        case '2':
            load_2();
            read_data();
            return 2;
        case '3':
            load_3();
            read_data();
            return 3;
        case '4':
            load_4();
            read_data();
            return 4;
        case '5':
            load_5();
            read_data();
            return 5;
        case '6':
            load_6();
            read_data();
            return 6;
        case '7':
            load_7();
            read_data();
            return 7;
        case '8':
            load_8();
            read_data();
            return 8;
        case '9':
            load_9();
            read_data();
            return 9;
        default:
            load_0();
            read_data();
            return 0;
    }
}

void print_time_remaining(int time) {
    int minutes_10s = (time / 60) / 10;
    int minutes_1s = (time / 60) % 10;
    int seconds_10s = (time % 60) / 10;
    int seconds_1s = (time % 60) % 10;

    clear_display();
    return_home();
    set_RS(0);
    set_RW(0);
    switch (minutes_10s) {
        case 0:
            load_0();
            read_data();
            break;
        case 1:
            load_1();
            read_data();
            break;
        case 2:
            load_2();
            read_data();
            break;
        case 3:
            load_3();
            read_data();
            break;
        case 4:
            load_4();
            read_data();
            break;
        case 5:
            load_5();
            read_data();
            break;
        case 6:
            load_6();
            read_data();
            break;
        case 7:
            load_7();
            read_data();
            break;
        case 8:
            load_8();
            read_data();
            break;
        case 9:
            load_9();
            read_data();
            break;
        default:
            load_0();
            read_data();
            break;
    }
    switch (minutes_1s) {
        case 0:
            load_0();
            read_data();
            break;
        case 1:
            load_1();
            read_data();
            break;
        case 2:
            load_2();
            read_data();
            break;
        case 3:
            load_3();
            read_data();
            break;
        case 4:
            load_4();
            read_data();
            break;
        case 5:
            load_5();
            read_data();
            break;
        case 6:
            load_6();
            read_data();
            break;
        case 7:
            load_7();
            read_data();
            break;
        case 8:
            load_8();
            read_data();
            break;
        case 9:
            load_9();
            read_data();
            break;
        default:
            load_0();
            read_data();
            break;
    }
    enable_HTL();
    set_DB(0, 1, 0, 1, 1, 1, 0, 0);
    __delay_cycles(2000);
    read_data();
    switch (seconds_10s) {
        case 0:
            load_0();
            read_data();
            break;
        case 1:
            load_1();
            read_data();
            break;
        case 2:
            load_2();
            read_data();
            break;
        case 3:
            load_3();
            read_data();
            break;
        case 4:
            load_4();
            read_data();
            break;
        case 5:
            load_5();
            read_data();
            break;
        case 6:
            load_6();
            read_data();
            break;
        case 7:
            load_7();
            read_data();
            break;
        case 8:
            load_8();
            read_data();
            break;
        case 9:
            load_9();
            read_data();
            break;
        default:
            load_0();
            read_data();
            break;
    }
    switch (seconds_1s) {
        case 0:
            load_0();
            read_data();
            break;
        case 1:
            load_1();
            read_data();
            break;
        case 2:
            load_2();
            read_data();
            break;
        case 3:
            load_3();
            read_data();
            break;
        case 4:
            load_4();
            read_data();
            break;
        case 5:
            load_5();
            read_data();
            break;
        case 6:
            load_6();
            read_data();
            break;
        case 7:
            load_7();
            read_data();
            break;
        case 8:
            load_8();
            read_data();
            break;
        case 9:
            load_9();
            read_data();
            break;
        default:
            load_0();
            read_data();
            break;
    }
}

void load_0() {
    enable_HTL();
    set_DB(0, 0, 0, 0, 1, 1, 0, 0);
    __delay_cycles(2000);
}

void load_1() {
    enable_HTL();
    set_DB(1, 0, 0, 0, 1, 1, 0, 0);
    __delay_cycles(2000);
}

void load_2() {
    enable_HTL();
    set_DB(0, 1, 0, 0, 1, 1, 0, 0);
    __delay_cycles(2000);
}

void load_3() {
    enable_HTL();
    set_DB(1, 1, 0, 0, 1, 1, 0, 0);
    __delay_cycles(2000);
}

void load_4() {
    enable_HTL();
    set_DB(0, 0, 1, 0, 1, 1, 0, 0);
    __delay_cycles(2000);
}

void load_5() {
    enable_HTL();
    set_DB(1, 0, 1, 0, 1, 1, 0, 0);
    __delay_cycles(2000);
}

void load_6() {
    enable_HTL();
    set_DB(0, 1, 1, 0, 1, 1, 0, 0);
    __delay_cycles(2000);
}

void load_7() {
    enable_HTL();
    set_DB(1, 1, 1, 0, 1, 1, 0, 0);
    __delay_cycles(2000);
}

void load_8() {
    enable_HTL();
    set_DB(0, 0, 0, 1, 1, 1, 0, 0);
    __delay_cycles(2000);
}

void load_9() {
    enable_HTL();
    set_DB(1, 0, 0, 1, 1, 1, 0, 0);
    __delay_cycles(2000);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void shift_insert(int new_digit) {
    // Check if there's room for another digit
    int full = 1;
    for (i = 0; i < 4; i++) {
        if (time_input[i] == -1) {
            full = 0;
            break;
        }
    }
    if (full) return;

    // Shift existing digits left
    for (i = 0; i < 3; i++) {
        time_input[i] = time_input[i + 1];
    }
    // Insert new digit at the end
    time_input[3] = new_digit;
}

void clear_data() {
    data[0] = 0b00000000;
    data[1] = 0b00000000;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ISRs
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void) {
    if (time_remaining > 0) time_remaining--;
    time_changed = true;
    TB0CCTL0 &= ~CCIFG;
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_ISR(void) {
    if (state == 1) {
        input_received = true;
        if (((data[0] & 0b10100000) == 0b10100000) && ((data[0] & 0b01000000) != 0b01000000)) {
            full_transmission = true;
            mins = data[0] & 0b00011111;
            secs = data[1];
        }
    }
    if (UCB0RXBUF == ENTER_STATE_1 && state != 3) enter_state_1 = true;
    else if (UCB0RXBUF == ENTER_STATE_2 && state != 3) enter_state_2 = true;
    else if (UCB0RXBUF == PRESS && (state == 1 || state == 2)) started = true;
    else if (UCB0RXBUF == PRESS && state == 3) stopped = true;
    else if (UCB0RXBUF == PRESS && state == 0) silenced = true;
    else if (UCB0RXBUF == REHEAT) reheat = true;
    else if (UCB0RXBUF == DEFROST) defrost = true;
    else if (UCB0RXBUF == POPCORN) popcorn = true;
    else if (UCB0RXBUF == POTATO) potato = true;
    else data[index++] = UCB0RXBUF;
    if (index == 2) index = 0;
    
    UCB0IFG &= ~UCRXIFG;
}
