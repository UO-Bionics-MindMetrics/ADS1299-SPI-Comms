// Basic ADS1299 Test with Arduino Uno

#include "ADS1299.h"

ADS1299 ADS;

//Arduino Uno - Pin Assignments; Need to use ICSP for later AVR boards
// SCK = 13
// MISO [DOUT] = 12
// MOSI [DIN] = 11
// CS = 10; 
// DRDY = 9;

//  0x## -> Arduino Hexadecimal Format
//  0b## -> Arduino Binary Format

boolean deviceIDReturned = false;
boolean startedLogging = false;


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("ADS1299-bridge has started!");
  
  ADS.setup(9, 10, 8); // (DRDY pin, CS pin, RESET pin);
  delay(10);  //delay to ensure connection
  ADS.STARTUP();
  ADS.init_ADS_4();

}

void loop(){
  
  if(deviceIDReturned == false){
    
    ADS.getDeviceID(); //Funciton to return Device ID
    
    //prints dashed line to separate serial print sections
    Serial.println("----------------------------------------------");

    //PRINT ALL REGISTERS 
    ADS.RREG(0x00, 0x17);
    Serial.println("----------------------------------------------");
    delay(2000);
    //Start data conversions command
    ADS.START(); //must start before reading data continuous
    delay(1);
    deviceIDReturned = true;
    ADS.SDATAC();
  }
  
  //print data to the serial console for only the 1st 10seconds of 
  while(1){
    if(startedLogging == false){
      Serial.print("Millis: "); //this is to see at what time the data starts printing to check for timing accuracy (default sample rate is 250 sample/second)
      Serial.println(millis());
      startedLogging = true;
    }
    
    //Print Read Data Continuous (RDATAC) to Ardiuno serial monitor... 
    //The timing of this method is not perfect yet. Some data is getting lost 
    //and I believe its due to the serial monitor taking too much time to print data and not being ready to recieve to packets
    //ADS.updateData();  
    ADS.RDATA_update();
  }
  
  
}
