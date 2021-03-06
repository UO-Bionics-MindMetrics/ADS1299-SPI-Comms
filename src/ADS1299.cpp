//
//  ADS1299.cpp
//  
//  Created by Conor Russomanno on 6/17/13.
//  Modified by UOBionics 2021


#include "pins_arduino.h"
#include "ADS1299.h"

void ADS1299::setup(int _DRDY, int _CS, int RESET_pin){
    
    // **** ----- SPI Setup ----- **** //
    
    // Set direction register for SCK and MOSI pin.
    // MISO pin automatically overrides to INPUT.
    // When the SS pin is set as OUTPUT, it can be used as
    // a general purpose output port (it doesn't influence
    // SPI operations).
    
    pinMode(SCK, OUTPUT);
    pinMode(MOSI, OUTPUT);
    pinMode(SS, OUTPUT);
    
    digitalWrite(SCK, LOW);
    digitalWrite(MOSI, LOW);
    digitalWrite(SS, HIGH);
    
    // Warning: if the CS pin ever becomes a LOW INPUT then SPI
    // automatically switches to Slave, so the data direction of
    // the CS pin MUST be kept as OUTPUT.
    SPCR |= _BV(MSTR);
    SPCR |= _BV(SPE);
    
    //set clock divider
    SPCR = (SPCR & ~SPI_CLOCK_MASK) | (SPI_CLOCK_DIV16 & SPI_CLOCK_MASK);  // Divides 16MHz clock by 16 to set CLK speed to 1MHz
    SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((SPI_CLOCK_DIV16 >> 2) & SPI_2XCLOCK_MASK); // Divides 16MHz clock by 16 to set CLK speed to 1MHz
    
    //set data mode
    SPCR = (SPCR & ~SPI_MODE_MASK) | SPI_DATA_MODE; //clock polarity = 0; clock phase = 1 (pg. 8)
    
    //set bit order
    SPCR &= ~(_BV(DORD)); ////SPI data format is MSB (pg. 25)
    
    // **** ----- End of SPI Setup ----- **** //
    
    // initalize the  data ready and chip select pins:
    DRDY = _DRDY;
    CS = _CS;
    RESET_Pin = RESET_pin;
    pinMode(DRDY, INPUT);
    pinMode(CS, OUTPUT);
    pinMode(RESET_Pin, OUTPUT);
    
    tCLK = 0.000666; //666 ns (Datasheet, pg. 8)
    outputCount = 0;
}

//System Commands
void ADS1299::WAKEUP() {
    digitalWrite(CS, LOW); //Low to communicate
    transfer(_WAKEUP);
    digitalWrite(CS, HIGH); //High to end communication
    delay(4.0*tCLK);  //must way at least 4 tCLK cycles before sending another command (Datasheet, pg. 35)
}
void ADS1299::STANDBY() {
    digitalWrite(CS, LOW);
    transfer(_STANDBY);
    digitalWrite(CS, HIGH);
}
void ADS1299::RESET() {
    digitalWrite(CS, LOW);
    transfer(_RESET);
    delay(10);
//    delay(18.0*tCLK); //must wait 18 tCLK cycles to execute this command (Datasheet, pg. 35)
    digitalWrite(CS, HIGH);
}
void ADS1299::START() {
    digitalWrite(CS, LOW);
    transfer(_START);
    digitalWrite(CS, HIGH);
}
void ADS1299::STOP() {
    digitalWrite(CS, LOW);
    transfer(_STOP);
    digitalWrite(CS, HIGH);
}
//Data Read Commands
void ADS1299::RDATAC() {
    digitalWrite(CS, LOW);
    transfer(_RDATAC);
    digitalWrite(CS, HIGH);
}
void ADS1299::SDATAC() {
    digitalWrite(CS, LOW);
    transfer(_SDATAC);
    digitalWrite(CS, HIGH);
}
void ADS1299::RDATA() {
    digitalWrite(CS, LOW);
    transfer(_RDATA);
    digitalWrite(CS, HIGH);
}

//Register Read/Write Commands
void ADS1299::getDeviceID() {
    digitalWrite(CS, LOW); //Low to communicated
    transfer(_SDATAC); //SDATAC
    transfer(_RREG); //RREG
    transfer(0x00); //Asking for 1 byte
    byte data = transfer(0x00); // byte to read (hopefully 0b???11110)
    transfer(_RDATAC); //turn read data continuous back on
    digitalWrite(CS, HIGH); //Low to communicated
    Serial.println(data, BIN);
}

void ADS1299::RREG(byte _address) {
    byte opcode1 = _RREG + _address; //001rrrrr; _RREG = 00100000 and _address = rrrrr
    digitalWrite(CS, LOW); //Low to communicated
    transfer(_SDATAC); //SDATAC
    transfer(opcode1); //RREG
    transfer(0x00); //opcode2
    byte data = transfer(0x00); // returned byte should match default of register map unless edited manually (Datasheet, pg.39)
    printRegisterName(_address);
    Serial.print("0x");
    if(_address<16) Serial.print("0");
    Serial.print(_address, HEX);
    Serial.print(", ");
    Serial.print("0x");
    if(data<16) Serial.print("0");
    Serial.print(data, HEX);
    Serial.print(", ");
    for(byte j = 0; j<8; j++){
        Serial.print(bitRead(data, 7-j), BIN);
        if(j!=7) Serial.print(", ");
    }
    transfer(_RDATAC); //turn read data continuous back on
    digitalWrite(CS, HIGH); //High to end communication
    Serial.println();
}

void ADS1299::RREG(byte _address, byte _numRegistersMinusOne) {
    byte opcode1 = _RREG + _address; //001rrrrr; _RREG = 00100000 and _address = rrrrr
    digitalWrite(CS, LOW); //Low to communicated
    transfer(_SDATAC); //SDATAC
    transfer(opcode1); //RREG
    transfer(_numRegistersMinusOne); //opcode2
    for(byte i = 0; i <= _numRegistersMinusOne; i++){
        byte data = transfer(0x00); // returned byte should match default of register map unless previously edited manually (Datasheet, pg.39)
        printRegisterName(i);
        Serial.print("0x");
        if(i<16) Serial.print("0"); //lead with 0 if value is between 0x00-0x0F to ensure 2 digit format
        Serial.print(i, HEX);
        Serial.print(", ");
        Serial.print("0x");
        if(data<16) Serial.print("0"); //lead with 0 if value is between 0x00-0x0F to ensure 2 digit format
        Serial.print(data, HEX);
        Serial.print(", ");
        for(byte j = 0; j<8; j++){
            Serial.print(bitRead(data, 7-j), BIN);
            if(j!=7) Serial.print(", ");
        }
        Serial.println();
    }
    transfer(_RDATAC); //turn read data continuous back on
    digitalWrite(CS, HIGH); //High to end communication
}

void ADS1299::WREG(byte _address, byte _value) {
    byte opcode1 = _WREG + _address; //001rrrrr; _RREG = 00100000 and _address = rrrrr
    digitalWrite(CS, LOW); //Low to communicated
    transfer(_SDATAC); //SDATAC
    transfer(opcode1);
    transfer(0x00);
    transfer(_value);
    transfer(_RDATAC);
    digitalWrite(CS, HIGH); //Low to communicated
    Serial.print("Register 0x");
    Serial.print(_address, HEX);
    Serial.println(" modified.");
}

void ADS1299::updateData(){
    if(digitalRead(DRDY) == LOW){
        digitalWrite(CS, LOW);
//        long output[100][9];
        long output[9];
        long dataPacket;
        for(int i = 0; i<9; i++){
            for(int j = 0; j<3; j++){
                byte dataByte = transfer(0x00);
                dataPacket = (dataPacket<<8) | dataByte;
            }
//            output[outputCount][i] = dataPacket;
            output[i] = dataPacket;
            dataPacket = 0;
        }
        digitalWrite(CS, HIGH);
        // conversions to microvolts 
        double outputvolts[9];
        for(int i = 0; i < 9; i++){
            outputvolts[i] = double(output[i])*4.5/12/(pow(2,23)-1);
        }

        //Serial.print(outputCount);
        //Serial.print("\t");
        // changed to get data from 4 channels of ads1299-4
        // i=0;i<9 was original
        for (int i=1;i<5; i++) {
            Serial.print(outputvolts[i]);
            //Serial.print(output[i], HEX);
            if(i!=4) Serial.print("\t");
            
        }
        Serial.println();
    }
}

void ADS1299::RDATA_update(){
    if(digitalRead(DRDY) == LOW){
        //DRDY debug
        //Serial.println("LOW");
        digitalWrite(CS, LOW);
        // RDATA command - ONLY DIFFERENCE BETWEEN THIS FUNC AND THE updateDATA()
        transfer(_RDATA);
//        long output[100][9];
        signed long output[9];
        uint32_t dataPacket;
        for(int i = 0; i<9; i++){
            for(int j = 0; j<3; j++){
                byte dataByte = transfer(0x00);
                dataPacket = (dataPacket<<8) | dataByte; // constructing the 24 bit binary
            }
            // ------------------ Testing twos complement --------------- 
            // not in use right now, not necessary for now
            /*
            // 2 complement
            bool negative = (dataPacket & (1 << 23)) != 0;
            if (negative)
                output[i] = dataPacket| ~((1 << 24) - 1);
            else
                output[i] = dataPacket;
            */

            output[i] = dataPacket;
            dataPacket = 0;
        }
        
        digitalWrite(CS, HIGH);
        // conversions to microvolts 
        double outputvolts[9];
        for(int i = 0; i < 9; i++){
            outputvolts[i] = double(output[i])*4.5/24/(pow(2,23)-1);
        }

        //Serial.print(outputCount);
        //Serial.print("\t");
        // changed to get data from 4 channels of ads1299-4
        // i=0;i<9 was original
        for (int i=1;i<5; i++) {
            Serial.print(outputvolts[i]);
            //Serial.print(output[i]);
            if(i!=4) Serial.print("\t");
            
        }
        Serial.println();
        //outputCount++;
    }
}
// Working startup routine
void ADS1299::STARTUP(){
    // using the RESET pin to pull high or low instead of the command (not sure if the command works, still needs to test)
    // power up
    delay(10);
    digitalWrite(RESET_Pin,HIGH);
    delay(250);

    // reset pulse
    digitalWrite(RESET_Pin,LOW);
    delayMicroseconds(15);
    digitalWrite(RESET_Pin, HIGH);
    delay(250);

    SDATAC();
    WREG(CONFIG3, 0xE0);
    WREG(CONFIG1, 0x96);
    WREG(CONFIG2, 0xC0);
    WREG(CH1SET, 0x01);
    WREG(CH2SET, 0x01);
    WREG(CH3SET, 0x01);
    WREG(CH4SET, 0x01);
    delayMicroseconds(15);
    RDATAC();
    SDATAC();
    WREG(CONFIG3, 0b11100000);
    delay(10);
    WREG(CONFIG1, 0x96);
    WREG(CONFIG2, 0xC0);
    WREG(CH1SET, 0x01);
    WREG(CH2SET, 0x01);
    WREG(CH3SET, 0x01);
    WREG(CH4SET, 0x01);
    RDATAC();
}
void ADS1299::init_ADS_4(){
    WREG(CONFIG1, 0b11010110);
    WREG(CONFIG2, 0b11000000);
    WREG(CONFIG3, 0b11100100);
    // 0b01100101 - test signal
    // 0b01100000 - normal operation
    WREG(CH1SET, 0b01100000);
    WREG(CH2SET, 0b01100000);
    WREG(CH3SET, 0b01100000);
    WREG(CH4SET, 0b01100000);
    WREG(BIAS_SENSP, 0b00000000);
    WREG(BIAS_SENSN, 0b00000000);
    WREG(GPIO, 0b00000000);
    WREG(MISC1, 0b00100000);

}
// Square sine wave test
void ADS1299::init_ADS_4_test(){
    WREG(CONFIG1, 0b11010110);
    WREG(CONFIG2, 0b11010000);
    WREG(CONFIG3, 0b11101000);
    // 0b01100101 - test signal
    // 0b01100000 - normal operation
    WREG(CH1SET, 0b01100101);
    WREG(CH2SET, 0b01100101);
    WREG(CH3SET, 0b01100101);
    WREG(CH4SET, 0b01100101);
    WREG(BIAS_SENSP, 0b00001111);
    WREG(BIAS_SENSN, 0b00001111);
    WREG(GPIO, 0b00000000);
}

void ADS1299::init_ADS_8(){

}

// String-Byte converters for RREG and WREG
void ADS1299::printRegisterName(byte _address) {
    if(_address == ID){
        Serial.print("ID, ");
    }
    else if(_address == CONFIG1){
        Serial.print("CONFIG1, ");
    }
    else if(_address == CONFIG2){
        Serial.print("CONFIG2, ");
    }
    else if(_address == CONFIG3){
        Serial.print("CONFIG3, ");
    }
    else if(_address == LOFF){
        Serial.print("LOFF, ");
    }
    else if(_address == CH1SET){
        Serial.print("CH1SET, ");
    }
    else if(_address == CH2SET){
        Serial.print("CH2SET, ");
    }
    else if(_address == CH3SET){
        Serial.print("CH3SET, ");
    }
    else if(_address == CH4SET){
        Serial.print("CH4SET, ");
    }
    else if(_address == CH5SET){
        Serial.print("CH5SET, ");
    }
    else if(_address == CH6SET){
        Serial.print("CH6SET, ");
    }
    else if(_address == CH7SET){
        Serial.print("CH7SET, ");
    }
    else if(_address == CH8SET){
        Serial.print("CH8SET, ");
    }
    else if(_address == BIAS_SENSP){
        Serial.print("BIAS_SENSP, ");
    }
    else if(_address == BIAS_SENSN){
        Serial.print("BIAS_SENSN, ");
    }
    else if(_address == LOFF_SENSP){
        Serial.print("LOFF_SENSP, ");
    }
    else if(_address == LOFF_SENSN){
        Serial.print("LOFF_SENSN, ");
    }
    else if(_address == LOFF_FLIP){
        Serial.print("LOFF_FLIP, ");
    }
    else if(_address == LOFF_STATP){
        Serial.print("LOFF_STATP, ");
    }
    else if(_address == LOFF_STATN){
        Serial.print("LOFF_STATN, ");
    }
    else if(_address == GPIO){
        Serial.print("GPIO, ");
    }
    else if(_address == MISC1){
        Serial.print("MISC1, ");
    }
    else if(_address == MISC2){
        Serial.print("MISC2, ");
    }
    else if(_address == CONFIG4){
        Serial.print("CONFIG4, ");
    }
}

//SPI communication methods
byte ADS1299::transfer(byte _data) {
    SPDR = _data;
    while (!(SPSR & _BV(SPIF)))
        ;
    return SPDR;
}
