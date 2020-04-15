/* 
 * File:   main.c
 * Author: dannimakes
 *
 * Created on August 22, 2019.
 */
// PIC12F1822 Configuration Bit Settings

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable (PWRT enabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = ON      // PLL Enable (4x PLL disabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

// Includes and definitions

#define _XTAL_FREQ 32000000
#define time_debounce 450
#define time_loop 50
#define MAX_ANIM 9

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

// Global variables declation

volatile union _Matrix {

    struct _data {
        unsigned char EN;
        unsigned char L1;
        unsigned char L2;
        unsigned char L3;
        unsigned char L4;
        unsigned char L5;
        unsigned char L6;
        unsigned char L7;
        unsigned char L8;
    } data;
    unsigned char byte[];
} Matrix;

volatile unsigned char position;
volatile unsigned char animation;
volatile unsigned char steps;
volatile unsigned char debounce;

// Functions

void Display() //refresh and maintains the LED's with POV. Only one LED lits at a time.
{
    if (Matrix.byte[position] == 1) {
        switch (position) { //light up the corresponding LED
            case 1:
                TRISA = 0b00101011;
                LATA = 0b00000100;
                break;
            case 2:
                TRISA = 0b00001111;
                LATA = 0b00010000;
                break;
            case 3:
                TRISA = 0b00101011;
                LATA = 0b00010000;
                break;
            case 4:
                TRISA = 0b00001111;
                LATA = 0b00100000;
                break;
            case 5:
                TRISA = 0b00011011;
                LATA = 0b00000100;
                break;
            case 6:
                TRISA = 0b00011011;
                LATA = 0b00100000;
                break;
            case 7:
                TRISA = 0b00111100;
                LATA = 0b00000001;
                break;
            case 8:
                TRISA = 0b00111100;
                LATA = 0b00000010;
                break;
        }
    } else {
        TRISA = 0b00111111; //turn off all the LED's
    }
}

void initializeVars() {
    position = 1;
    animation = 2;
    steps = 0;
    debounce = 0;
    Matrix.data.EN = 0;
    Matrix.data.L1 = 0;
    Matrix.data.L2 = 0;
    Matrix.data.L3 = 0;
    Matrix.data.L4 = 0;
    Matrix.data.L5 = 0;
    Matrix.data.L6 = 0;
    Matrix.data.L7 = 0;
    Matrix.data.L8 = 0;
}

void badgeLeds(unsigned char leds) {
    Matrix.data.L8 = (unsigned char) (0x01 & (leds >> 0));
    Matrix.data.L7 = (unsigned char) (0x01 & (leds >> 1));
    Matrix.data.L6 = (unsigned char) (0x01 & (leds >> 2));
    Matrix.data.L5 = (unsigned char) (0x01 & (leds >> 3));
    Matrix.data.L4 = (unsigned char) (0x01 & (leds >> 4));
    Matrix.data.L3 = (unsigned char) (0x01 & (leds >> 5));
    Matrix.data.L2 = (unsigned char) (0x01 & (leds >> 6));
    Matrix.data.L1 = (unsigned char) (0x01 & (leds >> 7));
}

// Interrupts

void __interrupt isr() {

    if (INTCONbits.IOCIF == 1 && IOCAFbits.IOCAF3 == 1) //interrupt on change on RA3 triggered
    {
        INTCONbits.IOCIE = 0; //disable on change interrupts
        if (debounce == 0) {
            debounce = 1;
            animation++; //move to the next badge LED animation
            if (animation > MAX_ANIM) { //start again with the first animation
                animation = 1;
            }
        }
        IOCAFbits.IOCAF3 = 0; //clear interrupt flag
        INTCONbits.IOCIE = 1; //enable on change interrupts
    }

    if (PIR1bits.TMR1IF == 1) //timer1 interrupt, called every 8.1918 ms. This creates the POV illusion for the badge
    {
        T1CONbits.TMR1ON = 0; //stop timer1
        position++; //increment the LED to position show
        if (position > 8) { //start again with the first LED
            position = 1;
        }
        Display(); //show up the LED's on the badge
        PIR1bits.TMR1IF = 0; //clear interrutp flag
        T1CONbits.TMR1ON = 1; //start timer1
    }

}

// Main program

void main() {
    OSCCON = 0b11110000; //configure internal oscilator for 8Mhz
    TRISA = 0b00001000; //configure IO
    ANSELA = 0b00000000; //analog functions of pins disabled
    WPUA = 0b00001000; //configure weak pull-ups on input pins
    OPTION_REGbits.nWPUEN = 0; //enable weak pull-ups
    initializeVars();
    INTCONbits.PEIE = 1; // Enable peripheral interrupts
    IOCANbits.IOCAN3 = 1; //configure interrupt on falling edge for button
    INTCON = 0b01001000; //enables interrupts
    T1CON = 0b01000100; //configure timer1 to run at 8 MHz
    PIE1bits.TMR1IE = 1; //enable timer1 interrupt
    T1CONbits.TMR1ON = 1; //start timer1
    INTCONbits.GIE = 1; //run interrupts

    while (1) {
        if (debounce == 1) { //if a button interrupt was called we debounce it here    
            badgeLeds(0);
            steps = 0;
            __delay_ms(time_debounce);
            debounce = 0;
        } else {
            switch (animation) {
                case 1: //shut down 
                    T1CONbits.TMR1ON = 0; //stop timer 1
                    TRISA = 0b00111111; //turn off all the LED's
                    asm("sleep"); //sleep the micro until a button press
                    break;
                case 2: //animation 1
                    T1CONbits.TMR1ON = 1; //start timer1
                    badgeLeds(0b11111111);
                    break;
                case 3: //animation 2
                    T1CONbits.TMR1ON = 1; //start timer1
                    badgeLeds(0b11111100);
                    break;
                case 4: //animation 3
                    T1CONbits.TMR1ON = 1; //start timer1
                    badgeLeds(0b00000011);
                    break;
                case 5: //animation 4
                    T1CONbits.TMR1ON = 1; //start timer1
                    if (steps >= 0 && steps <= 4) {
                        badgeLeds(0b10000010);
                    }
                    if (steps >= 5 && steps <= 9) {
                        badgeLeds(0b01000001);
                    }
                    if (steps >= 10 && steps <= 14) {
                        badgeLeds(0b00100010);
                    }
                    if (steps >= 15 && steps <= 19) {
                        badgeLeds(0b00010001);
                    }
                    if (steps >= 20 && steps <= 24) {
                        badgeLeds(0b00001010);
                    }
                    if (steps >= 25 && steps <= 29) {
                        badgeLeds(0b00000101);
                    }
                    steps++;
                    if (steps >= 30) {
                        steps = 0;
                    }
                    break;
                case 6: //animation 5
                    T1CONbits.TMR1ON = 1; //start timer1
                    if (steps >= 0 && steps <= 4) {
                        badgeLeds(0b00000101);
                    }
                    if (steps >= 5 && steps <= 9) {
                        badgeLeds(0b00001010);
                    }
                    if (steps >= 10 && steps <= 14) {
                        badgeLeds(0b00010001);
                    }
                    if (steps >= 15 && steps <= 19) {
                        badgeLeds(0b00100010);
                    }
                    if (steps >= 20 && steps <= 24) {
                        badgeLeds(0b01000001);
                    }
                    if (steps >= 25 && steps <= 29) {
                        badgeLeds(0b10000010);
                    }
                    steps++;
                    if (steps >= 30) {
                        steps = 0;
                    }
                    break;
                case 7: //animation 6
                    T1CONbits.TMR1ON = 1; //start timer1
                    if (steps >= 0 && steps <= 4) {
                        badgeLeds(0b00000111);
                    }
                    if (steps >= 5 && steps <= 9) {
                        badgeLeds(0b00001011);
                    }
                    if (steps >= 10 && steps <= 14) {
                        badgeLeds(0b00010011);
                    }
                    if (steps >= 15 && steps <= 19) {
                        badgeLeds(0b00100011);
                    }
                    if (steps >= 20 && steps <= 24) {
                        badgeLeds(0b01000011);
                    }
                    if (steps >= 25 && steps <= 29) {
                        badgeLeds(0b10000000);
                    }
                    if (steps >= 30 && steps <= 34) {
                        badgeLeds(0b10000011);
                    }
                    if (steps >= 35 && steps <= 39) {
                        badgeLeds(0b01000011);
                    }
                    if (steps >= 40 && steps <= 44) {
                        badgeLeds(0b00100011);
                    }
                    if (steps >= 45 && steps <= 49) {
                        badgeLeds(0b00010011);
                    }
                    if (steps >= 50 && steps <= 54) {
                        badgeLeds(0b00001011);
                    }
                    if (steps >= 55 && steps <= 59) {
                        badgeLeds(0b00000100);
                    }
                    steps = steps + 2;
                    if (steps >= 60) {
                        steps = 0;
                    }
                    break;
                case 8: //animation 7
                    T1CONbits.TMR1ON = 1; //start timer1
                    if (steps >= 0 && steps <= 3) {
                        badgeLeds(0b00110011);
                    }
                    if (steps >= 4 && steps <= 7) {
                        badgeLeds(0b01001000);
                    }
                    if (steps >= 8 && steps <= 11) {
                        badgeLeds(0b10000110);
                    }
                    if (steps >= 12 && steps <= 15) {
                        badgeLeds(0b10000101);
                    }
                    if (steps >= 16 && steps <= 19) {
                        badgeLeds(0b01001011);
                    }
                    if (steps >= 20 && steps <= 23) {
                        badgeLeds(0b00110000);
                    }
                    steps++;
                    if (steps >= 24) {
                        steps = 0;
                    }
                    break;
                case 9: //animation 8
                    T1CONbits.TMR1ON = 1; //start timer1
                    badgeLeds((unsigned char) rand());
                    break;
            }
        }
        __delay_ms(time_loop);
    }
}
