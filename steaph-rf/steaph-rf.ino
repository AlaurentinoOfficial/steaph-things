#include <SPI.h>
#include <IRremote.h>
#include "RF24.h"

byte addresses[] = {"steaph/environments/[environments id]"};

/****************** Pin Config ***************************/

const unsigned short relay[] = { 2, 3, 4, 5 };

const unsigned short irSender = 6;

IRsend irsend; // PIN ~3

unsigned int irPowerOn[] = {9000, 4500, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 39416, 9000, 2210, 560}; //AnalysIR Batch Export (IRremote) - RAW
unsigned int irPowerOff[] = {9000, 4500, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 560, 560, 560, 560, 560, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 560, 560, 1690, 560, 1690, 560, 1690, 560, 1690, 560, 39416, 9000, 2210, 560}; //AnalysIR Batch Export (IRremote) - RAW

/**********************************************************/


/****************** RF Config ***************************/

RF24 radio(9,10);

/**********************************************************/


struct EnvironmentStatus
{
  int status;
};

EnvironmentStatus myData;

void setup() 
{

  Serial.begin(115200);
  Serial.println(F("RF24/examples/GettingStarted_HandlingData"));
  Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));
  
  radio.begin();

  radio.setPALevel(RF24_PA_LOW);
  
  radio.openWritingPipe(addresses);
  radio.openReadingPipe(1,addresses);
  
  myData.status = false;
  radio.startListening();

  for(int i = 0; i < sizeof(relay); i++)
    pinMode(relay[i], OUTPUT);
}




void loop() 
{
  if( radio.available())
  {
    
      while (radio.available()) 
      {
        radio.read( &myData, sizeof(myData) );
      }
     
      radio.stopListening();
      radio.write(&myData, true);      
      radio.startListening();
         
      Serial.print("Sent response: ");
      Serial.println(myData.status);

      setStatus(myData.status);
   }
} // Loop


void setStatus(bool status)
{
  if(status)
  {
    for(int i = 0; i < sizeof(relay); i++)
      digitalWrite(relay[i], LOW);

    irsend.sendRaw(irPowerOn, sizeof(irPowerOn) / sizeof(irPowerOn[0]), 38);
    return;
  }

  for(int i = 0; i < sizeof(relay); i++)
      digitalWrite(relay[i], HIGH);
      
  irsend.sendRaw(irPowerOff, sizeof(irPowerOff) / sizeof(irPowerOff[0]), 38);
}
