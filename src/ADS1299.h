//
//  ADS1299.h
//  
//  Created by Conor Russomanno on 6/17/13.
//  Modified by UOBionics 2021

#ifndef ADS1299_h
#define ADS1299_h

#include <stdio.h>
#include <Arduino.h>
#include <avr/pgmspace.h>
#include "Definitions.h"


class ADS1299 {
public:
    
    void setup(int _DRDY, int _CS, int RESET_pin);
    
    //ADS1299 SPI Command Definitions (Datasheet, Pg. 35)
    //System Commands
    void WAKEUP();
    void STANDBY();
    void RESET();
    void START();
    void STOP();
    
    //Data Read Commands
    void RDATAC();
    void SDATAC();
    void RDATA();
    
    //Register Read/Write Commands
    void getDeviceID();
    void RREG(byte _address);
    void RREG(byte _address, byte _numRegistersMinusOne); //to read multiple consecutive registers (Datasheet, pg. 38)
    
    void printRegisterName(byte _address);
    
    void WREG(byte _address, byte _value); //
    void WREG(byte _address, byte _value, byte _numRegistersMinusOne); //
    
    void updateData(); // RDATAC
    void RDATA_update(); // RDATA
    void STARTUP(); // Startup youtine
    void init_ADS_4();// initialize configs for 4 channel version
    void init_ADS_8();// initialize configs for 8 channel version
    void init_ADS_4_test();
    //SPI Arduino Library Stuff
    byte transfer(byte _data);
    
    float tCLK;
    int DRDY, CS; //pin numbers for "Data Ready" (DRDY) and "Chip Select" CS (Datasheet, pg. 26)
    int RESET_Pin;
    float VREF;
    int outputCount;
    
    
};

#endif