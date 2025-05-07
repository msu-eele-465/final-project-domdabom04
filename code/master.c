#include "intrinsics.h"
#include "msp430fr2355.h"
#include <msp430.h>
#include <stdlib.h>
#include <stdbool.h>

#define SLAVE_ADDRESS 0x46
#define PRESS 0b11111111
#define REHEAT 0b11111000
#define DEFROST 0b11111001
#define POPCORN 0b11111010
#define POTATO 0b11111011
#define ENTER_STATE_1 0b10000000
#define ENTER_STATE_2 0b11000000

void send_bytes(unsigned char, unsigned char);
void send_data();
void send_press();
void send_enter_1();
void send_enter_2();
void send_reheat();
void send_defrost();
void send_popcorn();
void send_potato();

void master_setup();
void keypad_setup();
void sensor_setup();

void key_released(volatile unsigned char*, unsigned char);
void clear_data();
void clear_input();
void input_to_data();

char input_arr[4] = {0, 0, 0, 0};
unsigned char data_arr[2] = {0, 0};

int input_index = 0;
int i = 0;
int write_index = 0;

int state = -1;
bool first_press = false;

int main() {

    WDTCTL = WDTPW | WDTHOLD;

    master_setup();
    __enable_interrupt();
    sensor_setup();
    keypad_setup();
    while(true) {
        // poll row 1
        P6OUT &= ~BIT1;
        if((P3IN & BIT7) == 0) {
            input_arr[input_index++] = '1';
            key_released(&P3IN, BIT7);
            //if (state == 1 || state == 2) send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
        } else if((P2IN & BIT4) == 0) {
            input_arr[input_index++] = '2';
            key_released(&P2IN, BIT4);
            //if (state == 1 || state == 2) send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
        } else if((P3IN & BIT3) == 0) {
            input_arr[input_index++] = '3';
            key_released(&P3IN, BIT3);
            //if (state == 1 || state == 2) send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
        } else if((P3IN & BIT2) == 0) {
            key_released(&P3IN, BIT2);
            state = 3;
            send_reheat();
            clear_input();
        }
        P6OUT |= BIT1;
        // poll row 2
        P6OUT &= ~BIT2;
        if((P3IN & BIT7) == 0) {
            input_arr[input_index++] = '4';
            key_released(&P3IN, BIT7);
            //if (state == 1 || state == 2) send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
        } else if((P2IN & BIT4) == 0) {
            input_arr[input_index++] = '5';
            key_released(&P2IN, BIT4);
            //if (state == 1 || state == 2) send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
        } else if((P3IN & BIT3) == 0) {
            input_arr[input_index++] = '6';
            key_released(&P3IN, BIT3);
            //if (state == 1 || state == 2) send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
        } else if((P3IN & BIT2) == 0) {
            key_released(&P3IN, BIT2);
            state = 3;
            send_defrost();
            clear_input();
        }
        P6OUT |= BIT2;
        // poll row 3
        P6OUT &= ~BIT3;
        if((P3IN & BIT7) == 0) {
            input_arr[input_index++] = '7';
            key_released(&P3IN, BIT7);
            //if (state == 1 || state == 2) send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
        } else if((P2IN & BIT4) == 0) {
            input_arr[input_index++] = '8';
            key_released(&P2IN, BIT4);
            //if (state == 1 || state == 2) send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
        } else if((P3IN & BIT3) == 0) {
            input_arr[input_index++] = '9';
            key_released(&P3IN, BIT3);
            //if (state == 1 || state == 2) send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
        } else if((P3IN & BIT2) == 0) {
            key_released(&P3IN, BIT2);
            state = 3;
            send_popcorn();
            clear_input();
        }
        P6OUT |= BIT3;
        // poll row 4
        P6OUT &= ~BIT4;
        if((P3IN & BIT7) == 0) {
            key_released(&P3IN, BIT7);
            //state = 2;
            //send_enter_2();
            clear_input();
        } else if((P2IN & BIT4) == 0) {
            input_arr[input_index++] = '0';
            key_released(&P2IN, BIT4);
            //if (state == 1 || state == 2) send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
        } else if((P3IN & BIT3) == 0) {
            key_released(&P3IN, BIT3);
            state = 1;
            send_enter_1();
            clear_input();
        } else if((P3IN & BIT2) == 0) {
            key_released(&P3IN, BIT2);
            state = 3;
            send_potato();
            clear_input();
        }
        P6OUT |= BIT4;

        //if (state == 1 || state == 2) send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
        if (P1IN & BIT5) {
            send_press();
            __delay_cycles(262144);
            
            //if (state == 1) {
                input_to_data();
                send_data();
            //} else if (state == 2) {
                //send_bytes(input_arr[input_index - 1], input_arr[input_index - 1]);
            //}

            if (state == 1 || state == 2) state = 3;
            else if (state == 3) state = 0;
            else state = -1;
        }

        if (input_index > 4) {
            input_index = 0;
            clear_input();
        }
    }
}

void send_bytes(unsigned char byte1, unsigned char byte2) {
    data_arr[0] = byte1;
    data_arr[1] = byte2;
    write_index = 0;

    UCB0IFG &= ~UCTXIFG0;           // Clear flag
    UCB0IE |= UCTXIE0;              // Enable TX interrupt
    UCB0CTLW0 |= UCTR | UCTXSTT;    // Start TX mode + send start

    while (!(UCB0IFG & UCSTPIFG)) ; // Wait for STOP condition

    UCB0IFG &= ~UCSTPIFG;           // Clear STOP
    UCB0IE &= ~UCTXIE0;             // Disable TX interrupt
    UCB0CTLW0 &= ~UCTR;             // Clear TX mode

    clear_data();
}

void send_data() {
    send_bytes(data_arr[0], data_arr[1]);
}

void send_press() {
    send_bytes(PRESS, PRESS);
}

void send_enter_1() {
    send_bytes(ENTER_STATE_1, ENTER_STATE_1);
}

void send_enter_2() {
    send_bytes(ENTER_STATE_2, ENTER_STATE_2);
}

void send_reheat() {
    send_bytes(REHEAT, REHEAT);
}

void send_defrost() {
    send_bytes(DEFROST, DEFROST);
}

void send_popcorn() {
    send_bytes(POPCORN, POPCORN);
}

void send_potato() {
    send_bytes(POTATO, POTATO);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Setup functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void master_setup() {
    UCB0CTLW0 |= UCSWRST;               // Software Reset

    UCB0CTLW0 |= UCSSEL__SMCLK;         // SMCLK
    UCB0BRW = 10;                       // Set prescalar to 10

    UCB0CTLW0 |= UCMODE_3;              // Put into i2c mode
    UCB0CTLW0 |= UCMST;                 // Set MSP430FR2355 as master

    UCB0I2CSA = SLAVE_ADDRESS;          // Set slave address

    UCB0CTLW1 |= UCASTP_2;
    UCB0TBCNT = 2;

    P1SEL1 &= ~BIT3;                    // SCL setup
    P1SEL0 |= BIT3;

    P1SEL1 &= ~BIT2;                    // SDA setup
    P1SEL0 |= BIT2;

    PM5CTL0 &= ~LOCKLPM5;
    UCB0CTLW0 &= ~UCSWRST;
}

void keypad_setup() {
    P6DIR |= BIT1;          // all rows set as outputs
    P6DIR |= BIT2;
    P6DIR |= BIT3;
    P6DIR |= BIT4;

    P6OUT |= BIT1;          // initialize row outputs HI
    P6OUT |= BIT2;
    P6OUT |= BIT3;
    P6OUT |= BIT4;

    P3DIR &= ~BIT7;         // all columns set as inputs
    P2DIR &= ~BIT4;
    P3DIR &= ~BIT3;
    P3DIR &= ~BIT2;

    P3REN |= BIT7;          // enable resistors for columns
    P2REN |= BIT4;
    P3REN |= BIT3;
    P3REN |= BIT2;
    P3OUT |= BIT7;          // set column resistors as pull-ups
    P2OUT |= BIT4;
    P3OUT |= BIT3;
    P3OUT |= BIT2;
}

void sensor_setup() {
    P1DIR &= ~BIT5;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void key_released(volatile unsigned char* pin, unsigned char bit) {
    while((*pin & bit) == 0) {}
    return;
}

void clear_data() {
    data_arr[0] = 0;
    data_arr[1] = 0;
}

void clear_input() {
    input_arr[0] = 0;
    input_arr[1] = 0;
}

void input_to_data() {
    int mins = 0;
    int secs = 0;
    int digits = 4;

    int j;
    for (j = 0; j < 4; j++) {
        if (input_arr[j] == 0) {
            digits = j;
            break;
        }
        if (input_arr[j] < '0' || input_arr[j] > '9') return;
    }

    switch (digits) {
        case 1:
            secs = input_arr[0] - '0';
            break;
        case 2:
            secs = (input_arr[0] - '0') * 10 + (input_arr[1] - '0');
            break;
        case 3:
            mins = input_arr[0] - '0';
            secs = (input_arr[1] - '0') * 10 + (input_arr[2] - '0');
            break;
        case 4:
            mins = (input_arr[0] - '0') * 10 + (input_arr[1] - '0');
            secs = (input_arr[2] - '0') * 10 + (input_arr[3] - '0');
            break;
        default:
            return;
    }

    data_arr[0] = (unsigned char) mins;
    data_arr[0] |= 0b10100000;
    data_arr[1] = (unsigned char) secs;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ISRs
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    switch(__even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG)) {
        case USCI_I2C_UCTXIFG0:
            UCB0TXBUF = data_arr[write_index++];
            if (write_index == 2) {
                UCB0CTLW0 |= UCTXSTP; // Send stop
                write_index = 0;
            }
            break;
        case USCI_I2C_UCNACKIFG:
            UCB0CTLW0 |= UCTXSTT;    // Retry
            break;
        case USCI_I2C_UCSTPIFG:
            UCB0IFG &=~UCSTPIFG;
            break;
        default: break;
    }
}
