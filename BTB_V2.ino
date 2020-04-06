// demo: CAN-BUS Shield, receive data with check mode
// send data coming to fast, such as less than 10ms, you can use this way
// loovee, 2014-6-13


#include <SPI.h>
#include "mcp_can.h"


// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;



MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

void setup()
{
    Serial.begin(115200);

    while (CAN_OK != CAN.begin(CAN_500KBPS))              // init can bus : baudrate = 500k
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println(" Init CAN BUS Shield again");
        delay(100);
    }
    Serial.println("CAN BUS Shield init ok!");
}


void loop()
{
    unsigned char len = 0;
    unsigned char buf[8];

    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

        unsigned char canId = CAN.getCanId();
        
        //Serial.println("-----------------------------");
        //Serial.print("Get data from ID: ");
       // Serial.println(canId, HEX);
        //Serial.println(buf[2], HEX);

        if(buf[2] == 0x25){// we have a percent sign
          Serial.print(buf[0]);
          Serial.println(buf[1]);
          if(buf[0] == 0x31324 && buf[1] == 0x31){
            //Serial.print(buf[3]);
            Serial.print((char)buf[3]);
            Serial.print((char)buf[4]);
            Serial.print((char)buf[5]);
            Serial.print((char)buf[6]);
            Serial.println((char)buf[7]);
          }
        }


        for(int i = 0; i<len; i++)    // print the data
        {
            //Serial.print(buf[i], HEX);
            //Serial.print("\t");
        }
        //Serial.println();
    }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
