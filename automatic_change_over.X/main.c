/*
 * File:   main.c
 * Author: peTron_Quantinity
 *
 * Created on December 12, 2020, 10:37 PM
 * three phase automatic change over system with instantaneous power consumption LCD display
 */

/*
 * change the following lines 
 * line 105 to __delay_ms(1000);
 * line 109 to __delay_ms(500);
 * line 130 to __delay_ms(500);
 * line 132 to __delay_ms(500);
 * line 320 to __delay_ms(200);
 */

// CONFIG
#pragma config FOSC = HS     // Oscillator Selection bits (RC oscillator)
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

//INPUTS
int aVOLTAGE,bVOLTAGE,cVOLTAGE; /*aVOLTAGE on RA0
                                 *bVOLTAGE on RA1
                                 *cVOLTAGE on RA2 
                                */
#define max_power RB6
#define power_saving RB7
#define acsReset RB2

//OUTPUTS
#define aCTRL RB3
#define bCTRL RB4
#define cCTRL RB5

//VIA
short int aDANGER;
short int bDANGER;
short int cDANGER;
int danger;

//PARAMETERS
unsigned int voltage, current, power;
short int cLoop=0;

//STATES
enum state {iS,hL,pS3,pS2,pS1,mP3,mP2,mP1,aP,bP,cP,off,cM,pC,lcd,BLE}nextState;
enum state nextState=iS;
void initialState();
void hotList();
void powerSaving3();
void powerSaving2();
void powerSaving1();
void maximumPower3();
void maximumPower2();
void maximumPower1();
void aON();
void bON();
void cON();
void OFF();
void currentMeasurement();
void powerCalculation();
void LCD();
void ble();

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
    TRISB=0xC0;
    PORTB=0x00;
    TRISE0=0;
    RE0=0;
    TRISD=0x00;
    PORTD=0x00;
    Lcd_Init();
    Lcd_Set_Cursor(1,5);
    Lcd_Write_String("Aeon-Atk");
    Lcd_Set_Cursor(2,1);
    Lcd_Write_String("auto phase sysm");
    __delay_ms(100);
    Lcd_Clear();
    Lcd_Set_Cursor(1,5);
    Lcd_Write_String("loading");
    __delay_ms(100);
    Lcd_Clear();
    nextState=hL;
    
    while(1){
        if(nextState==hL)hotList();
        if(nextState==pS3)powerSaving3();
        else if(nextState==pS2)powerSaving2();
        else if(nextState==pS1)powerSaving1();
        else if(nextState==mP3)maximumPower3();
        else if(nextState==mP2)maximumPower2();
        else if(nextState==mP1)maximumPower1();
        else if(nextState==aP)aON();
        else if(nextState==bP)bON();
        else if(nextState==cP)cON();
        else if(nextState==off)OFF();
        if(nextState==cM)currentMeasurement();
        if(nextState==pC)nextState=lcd;//call power Calculation function and make nextState hotList
        if(nextState==lcd)LCD();
        if(nextState==BLE)ble();
        acsReset=0;
        RE0=1;
        __delay_ms(10);
        RE0=0;
        __delay_ms(10);
    }   
    return;
}
void hotList(){
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
    if((cVOLTAGE>=614)||(cVOLTAGE<=368))cDANGER=1; 
    else cDANGER=0;
    __delay_us(20);
    danger=0;
    danger=(aDANGER<<2)|(bDANGER<<1)|(cDANGER);
    Lcd_Clear();
    if(danger==7)nextState=off;
    else{
       if(power_saving==1){
           Lcd_Set_Cursor(1,1);
           Lcd_Write_String("pS");
           if(danger==0)nextState=pS3;
           else if((danger==1)||(danger==2)||(danger==4))nextState=pS2;
           else if((danger==3)||(danger==6)||(danger==5))nextState=pS1;
       }else if(max_power==1){
           Lcd_Set_Cursor(1,1);
           Lcd_Write_String("mP");
           if(danger==0)nextState=mP3;
           else if((danger==1)||(danger==2)||(danger==4))nextState=mP2;
           else if((danger==3)||(danger==6)||(danger==5))nextState=mP1;
       }else{
           Lcd_Set_Cursor(1,1);
           Lcd_Write_String("p ");
           if(aDANGER==0)nextState=aP;
           else if(aDANGER==0)nextState=bP;
           else if(cDANGER==0)nextState=cP;
       }
    }
}
void powerSaving3(){
    if(aVOLTAGE==bVOLTAGE){
        if(cVOLTAGE<aVOLTAGE)cON();
        else aON();
    }else if(bVOLTAGE==cVOLTAGE){
        if(aVOLTAGE<bVOLTAGE)aON();
        else bON();
    }else if(aVOLTAGE==cVOLTAGE){
        if(bVOLTAGE<aVOLTAGE)bON();
        else aON();
    }else{
        if((aVOLTAGE<bVOLTAGE)&&(aVOLTAGE<cVOLTAGE))aON;
        else if((bVOLTAGE<cVOLTAGE)&&(bVOLTAGE<aVOLTAGE))bON();
        else if((cVOLTAGE<bVOLTAGE)&&(cVOLTAGE<aVOLTAGE))cON();
    }
    nextState=cM;
}
void powerSaving2(){
    switch(danger){
        case 1:
            if(aVOLTAGE<=bVOLTAGE)aON();
            else bON();
            break;
        case 2:
            if(aVOLTAGE<=cVOLTAGE)aON();
            else cON();
            break;
        case 4:
            if(bVOLTAGE<=cVOLTAGE)bON();
            else cON();
            break;
    }
    nextState=cM;
}
void powerSaving1(){
    switch(danger){
        case 3:
            aON();
            break;
        case 6:
            cON();
            break;
        case 5:
            bON();
            break;
    }
    nextState=cM;
}
void maximumPower3(){
    if(aVOLTAGE==bVOLTAGE){
        if(cVOLTAGE>aVOLTAGE)cON();
        else aON();
    }else if(bVOLTAGE==cVOLTAGE){
        if(aVOLTAGE>bVOLTAGE)aON();
        else bON();
    }else if(aVOLTAGE==cVOLTAGE){
        if(bVOLTAGE>aVOLTAGE)bON();
        else aON();
    }else{
        if((aVOLTAGE>bVOLTAGE)&&(aVOLTAGE>cVOLTAGE))aON;
        else if((bVOLTAGE>cVOLTAGE)&&(bVOLTAGE>aVOLTAGE))bON();
        else if((cVOLTAGE>bVOLTAGE)&&(cVOLTAGE>aVOLTAGE))cON();
    }
    nextState=cM;
}
void maximumPower2(){
    switch(danger){
        case 1:
            if(aVOLTAGE>=bVOLTAGE)aON();
            else bON();
            break;
        case 2:
            if(aVOLTAGE>=cVOLTAGE)aON();
            else cON();
            break;
        case 4:
            if(bVOLTAGE>=cVOLTAGE)bON();
            else cON();
            break;
    }
    nextState=cM;
}
void maximumPower1(){
    switch(danger){
        case 3:
            aON();
            break;
        case 6:
            cON();
            break;
        case 5:
            bON();
            break;
    }
    nextState=cM;
}
void aON(){
    aCTRL=1;
    bCTRL=0;
    cCTRL=0;
    voltage=aVOLTAGE;
    nextState=cM;
}
void bON(){
    aCTRL=0;
    bCTRL=1;
    cCTRL=0;
    voltage=bVOLTAGE;
    nextState=cM;
}
void cON(){
    aCTRL=0;
    bCTRL=0;
    cCTRL=1;
    voltage=cVOLTAGE;
    nextState=cM;
}
void OFF(){
    aCTRL=0;
    bCTRL=0;
    cCTRL=0;
    voltage=0;
    Lcd_Set_Cursor(1,16);
    Lcd_Write_Char('-');
    nextState=cM;
}
void currentMeasurement(){
    acsReset=1;
    ADCON0bits.CHS=3;
    ADCON0bits.GO_nDONE=1;
    while(ADCON0bits.GO_nDONE==1);
    current=(ADRESH<<8)+ADRESL;
    if(current>=512)current=((0.005*current)-2.5)/0.066;
    else current=(2.5-(0.005*current))/0.066;
    nextState=pC;
    __delay_us(200);
    acsReset=0;
}
void LCD(){
    Lcd_Set_Cursor(2,1);
    Lcd_Write_Int(0.5*voltage);
    Lcd_Write_String("V");
    Lcd_Set_Cursor(2,7);
    Lcd_Write_Int(current);
    Lcd_Write_String("A");
    Lcd_Set_Cursor(2,11);
    power=current*voltage;
    Lcd_Write_Int(power);
    Lcd_Write_String("W");
    nextState=BLE;
}
void ble(){
    nextState=hL;
}