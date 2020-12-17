/*
 * File:   main.c
 * Author: peTron_Quantinity
 *
 * Created on December 12, 2020, 10:37 PM
 * three phase automatic change over system with instantaneous power consumption LCD display
 */


// CONFIG
#pragma config FOSC = EXTRC     // Oscillator Selection bits (RC oscillator)
#pragma config WDTE = OFF        // Watchdog Timer Enable bit (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable bit (BOR enabled)
#pragma config LVP = ON         // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3/PGM pin has PGM function; low-voltage programming enabled)
#pragma config CPD = ON         // Data EEPROM Memory Code Protection bit (Data EEPROM code-protected)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = ON          // Flash Program Memory Code Protection bit (All program memory code-protected)

#include <xc.h>
#define _XTAL_FREQ 20000000
#define RS RD1
#define RW RD2
#define EN RD3
#define D4 RD4
#define D5 RD5
#define D6 RD6
#define D7 RD7
#include "lcd.h"

int aVOLTAGE,bVOLTAGE,cVOLTAGE; /*aVOLTAGE on RA0
                                 *bVOLTAGE on RA1
                                 *cVOLTAGE on RA2 
                                */
#define aCTRL RB3
#define bCTRL RB4
#define cCTRL RB5
#define aDANGER RB0
#define bDANGER RB1
#define cDANGER RB2
int voltage, current, power;
int max_current=0;
int min_current=0;
#define mid_thresh 512
#define max_power RB6
#define power_saving RB7

void aON();
void bON();
void cON();
void OFF();
void update_current(int *current, int *max_current, int *min_current);

void main(void) {
    ADCON1bits.ADFM=1;
    ADCON1bits.ADCS2=0;
    ADCON0bits.ADCS1=1;
    ADCON0bits.ADCS0=0;
    ADCON1bits.PCFG3=0;
    ADCON1bits.PCFG2=0;
    ADCON1bits.PCFG1=1;
    ADCON1bits.PCFG0=0;
    TRISA=0x0F;
    ADCON0bits.ADON=1;
    TRISB=0xc0;
    PORTB=0x00;
    TRISE0=0;
    RE0=0;
    TRISD=0x00;
    PORTD=0x00;
    Lcd_Init();
    Lcd_Set_Cursor(1,5);
    Lcd_Write_String("Aeon-Atk");
    Lcd_Set_Cursor(2,1);
    __delay_ms(1000);
    Lcd_Write_String("auto phase sysm");
    __delay_ms(2000);
    
    while(1){
        //hotlist high or low voltage
        __delay_us(20);
        ADCON0bits.CHS=0;
        ADCON0bits.GO_nDONE=1;
        while(ADCON0bits.GO_nDONE);
        aVOLTAGE=(ADRESH<<8)+ADRESL;
        if((aVOLTAGE>=614)||(aVOLTAGE<=368))aDANGER=1;
        else aDANGER=0;
        __delay_us(20);
        ADCON0bits.CHS=1;
        ADCON0bits.GO_nDONE=1;
        while(ADCON0bits.GO_nDONE);
        bVOLTAGE=(ADRESH<<8)+ADRESL;
        if((bVOLTAGE>=614)||(bVOLTAGE<=368))bDANGER=1;
        else bDANGER=0;
        __delay_us(20);
        ADCON0bits.CHS=2;
        ADCON0bits.GO_nDONE=1;
        while(ADCON0bits.GO_nDONE);
        cVOLTAGE=(ADRESH<<8)+ADRESL;
        if((cVOLTAGE>=720)||(cVOLTAGE<=368))cDANGER=1; 
        else cDANGER=0;
        __delay_us(20);

        //enter maximum voltage or power saving mode or priority mode
        if(max_power){
            if((aVOLTAGE>=bVOLTAGE)&&(aDANGER==0)){
                if(aVOLTAGE>cVOLTAGE)aON();
                else if(cDANGER==0)cON();
                else OFF();
            }else if((bVOLTAGE>=aVOLTAGE)&&(bDANGER==0)){
                if(bVOLTAGE>cVOLTAGE)bON();
                else if(cDANGER==0)cON();
                else OFF;
            }
        }else if(power_saving){
            if((aVOLTAGE<=bVOLTAGE)&&(aDANGER==0)){
                if(aVOLTAGE<cVOLTAGE)aON();
                else if(cDANGER==0)cON();
                else OFF();
            }else if((bVOLTAGE<=aVOLTAGE)&&(bDANGER==0)){
                if(bVOLTAGE<cVOLTAGE)bON();
                else if(cDANGER==0)cON();
                else OFF;
            }
        }else{
            if(aDANGER==0)aON();
            else if(bDANGER==0)bON();
            else if(cDANGER==0)cON();
            else OFF();
        }
        
        //sample acs712 output to get maximum current and calculate power consumption from that
        ADCON0bits.CHS=3;
        for(int i=0;i<100;i++){
            ADCON0bits.GO_nDONE=1;
            while(ADCON0bits.GO_nDONE);
            current=(ADRESH<<8)+ADRESL;
            if(current>max_current)max_current=current;
            if(current<min_current)min_current=current;
            __delay_us(190);
        }
        current=(max_current-min_current)/2;
        if(aCTRL==1){
            voltage=(500/1023)*aVOLTAGE;
        }else if(bCTRL==1){
            voltage=(500/1023)*bVOLTAGE;
        }else if(cCTRL==1){
            voltage=(500/1023)*cVOLTAGE;
        }else{
            voltage=0;
        }
        power=voltage*current;
        
        Lcd_Clear();
        Lcd_Set_Cursor(1,1);
        Lcd_Write_String("volts:");

        
        RE0=1;
        __delay_ms(10);
        RE0=0;
        __delay_ms(10);
    }   
    return;
}

void aON(){
    aCTRL=1;
    bCTRL=0;
    cCTRL=0;
}
void bON(){
    aCTRL=0;
    bCTRL=1;
    cCTRL=0;
}
void cON(){
    aCTRL=0;
    bCTRL=0;
    cCTRL=1;
}
void OFF(){
    aCTRL=0;
    bCTRL=0;
    cCTRL=0;
}