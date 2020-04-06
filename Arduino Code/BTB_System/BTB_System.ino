#define PIN_SHUTDOWN 7
#define PIN_UC_SIGNAL A0

// demo: CAN-BUS Shield, receive data with check mode
// send data coming to fast, such as less than 10ms, you can use this way
// loovee, 2014-6-13


#include <SPI.h>
#include "mcp_can.h"

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;
MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin


char readCmd[255];
int readCmdIndex = 0;
unsigned long signalTimer = 0;


float packVoltage = 0;
float packCurrent = 0;
float maxCellTemp = 0;
float minCellTemp = 0;
float minCellVoltage = 0;
float maxCellVoltage = 0;
float packCapacity = 0;
int packStatus = 0;

unsigned long sampleTimer = millis();
unsigned long samplePeriod = 20; //20 milliseconds.

void setup()
{
	initSerial();
	pinMode(PIN_SHUTDOWN, OUTPUT);
	digitalWrite(PIN_SHUTDOWN, LOW);
	postStatus();
}

void loop()
{
	//Serial.println(analogRead(PIN_UC_SIGNAL));
	serialHandler();
	//checkSystem();  //Code for the undervoltage check system
	checkCAN();
	postStatus();
}

void postStatus()
{
  if((millis()-samplePeriod)>=sampleTimer){
    sampleTimer = millis(); //reset the timer
    Serial1.print("$BTBS,");
    Serial1.print(packStatus);Serial1.print(",");
    Serial1.print(packVoltage, 3);Serial1.print(",");
    Serial1.print(packCurrent, 3);Serial1.print(",");
    Serial1.print(packCapacity, 3);Serial1.print(",");
    Serial1.print(maxCellTemp, 3);Serial1.print(",");
    Serial1.print(minCellTemp, 3);Serial1.print(",");
    Serial1.print(maxCellVoltage, 3);Serial1.print(",");
    Serial1.print(minCellVoltage, 3);Serial1.println();
  }
}
void checkSystem() 
{
  if (analogRead(PIN_UC_SIGNAL) < 200) {
    Serial1.println("$BTBF,UC_DETECT_1");
    //UC signal detected, wait 30 seconds and check again.
    delay(30000);
    if (analogRead(PIN_UC_SIGNAL) < 200) {
      Serial1.println("$BTBF,UC_DETECT_2");
      //UC signal detected, wait 30 seconds and check again.
      delay(30000);
      if (analogRead(PIN_UC_SIGNAL) < 200) {
        shutDown();
      }
    }
  }
}

void serialHandler()
{
  if (Serial1.available() > 0) 
  {
    // get incoming byte:
    readCmd[readCmdIndex] = Serial1.read();

    if (readCmd[readCmdIndex] == '\n') { //carriage return found meaning that is the end of our command.
      readCmdIndex = 0;

      processCmd(readCmd);  //send the readCmd to be processed

      memset(readCmd, 0, 255); //rest the readCmd char array to all nulls
    } else
      readCmdIndex++;
  }
}

void processCmd(char incomingCmd[]) 
{
  if (strcmp(incomingCmd, "$BTBC,SHUTDOWN\n") == 0) {

    shutDown();
  }
}

void shutDown() 
{
  Serial1.println("$BTBC,SHUTTING_DOWN");
  int timerShutdown = 60;
  while(timerShutdown != 0){
	Serial1.print("Shutting down in ");
	Serial1.print(timerShutdown);
	Serial1.println("seconds.");
	delay(1000);
	timerShutdown--;
  }
  digitalWrite(PIN_SHUTDOWN, HIGH);
  delay(5000); //this is in the case the arduino remains powered during a shutdown bench test
  digitalWrite(PIN_SHUTDOWN, LOW);
}

void initSerial() 
{
  //Initialize serial and wait for port to open:
  Serial1.begin(115200);


  // prints title with ending line break
  Serial1.println("==============================================");
  Serial1.println("==== SAMMUT TECH L.L.C =======================");
  Serial1.println("========================== © June,2016 =======");
  Serial1.println("==============================================");
  Serial1.println("BTB Battery Computer System Online!");

  while (CAN_OK != CAN.begin(CAN_500KBPS))              // init can bus : baudrate = 500k
  {
    Serial1.println("CAN BUS Shield init fail");
    Serial1.println(" Init CAN BUS Shield again");
    delay(100);
  }
  Serial1.println("CAN BUS Shield init ok!");
}

void checkCAN()
{
  unsigned char len = 0;
  unsigned char buf[8];

  if (CAN_MSGAVAIL == CAN.checkReceive())           // check if data coming
  {
    CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

    unsigned char canId = CAN.getCanId();

    //Serial1.println("-----------------------------");
    //Serial1.print("Get data from ID: ");
    // Serial1.println(canId, HEX);
    //Serial1.println(buf[2], HEX);

    if (buf[2] == 0x25) { // we have a percent sign
      //Serial1.print(buf[0], HEX);
      //Serial1.println(buf[1], HEX);

      //pack voltage
      if (buf[0] == 0x30 && buf[1] == 0x42) {
        processPackVoltage(buf[3], buf[4], buf[5], buf[6], buf[7]);
      }

      //pack current
      else if (buf[0] == 0x30 && buf[1] == 0x43) {
        processPackCurrent(buf[3], buf[4], buf[5], buf[6], buf[7]);
      }

      //pack capacity
      else if (buf[0] == 0x30 && buf[1] == 0x44) {
        processPackCapacity(buf[3], buf[4], buf[5], buf[6], buf[7]);
      }

      //min cell temp
      else if (buf[0] == 0x31 && buf[1] == 0x31) {
        processMinCellTemp(buf[3], buf[4], buf[5], buf[6], buf[7]);
      }


      //max cell temp
      else if (buf[0] == 0x30 && buf[1] == 0x35) {
        processMaxCellTemp(buf[3], buf[4], buf[5], buf[6], buf[7]);
      }

      //min cell voltage
      else if (buf[0] == 0x30 && buf[1] == 0x39) {
        processMinCellVoltage(buf[3], buf[4], buf[5], buf[6], buf[7]);
      }

      //max cell voltage
      else if (buf[0] == 0x30 && buf[1] == 0x37) {
        processMaxCellVoltage(buf[3], buf[4], buf[5], buf[6], buf[7]);
      }
      
      //pack status THER IS A TYPO IN THE DOC. I can guarantee you that it is 0-10 and not 1-11
      else if (buf[0] == 0x30 && buf[1] == 0x45) {
        processPackStatus(buf[3], buf[4], buf[5], buf[6], buf[7]);
      }
    }
  }
}

void processPackVoltage(char buff0, char buff1, char buff2, char buff3, char buff4) 
{
  char hexstring[] = {buff0, buff1, buff2, buff3, buff4};
  int number = (int)strtol(hexstring, NULL, 16);
  packVoltage = (float)number / 1000;
  //Serial1.print(packVoltage, 3);
  //Serial1.println();
}

void processPackCurrent(char buff0, char buff1, char buff2, char buff3, char buff4)
{
  char hexstring[] = {buff0, buff1, buff2, buff3, buff4};
  int number = (int)strtol(hexstring, NULL, 16);
  packCurrent = (float)number / 10;
  //Serial1.println(packCurrent, 3);
  //Serial1.println();
}

void processPackCapacity(char buff0, char buff1, char buff2, char buff3, char buff4)
{
  char hexstring[] = {buff0, buff1, buff2, buff3, buff4};
  int number = (int)strtol(hexstring, NULL, 16);
  packCapacity = (float)number;
  //Serial1.println(packCapacity, 3);
  //Serial1.println();
}

void processMinCellTemp(char buff0, char buff1, char buff2, char buff3, char buff4)
{
  char hexstring[] = {buff0, buff1, buff2, buff3, buff4};
  int number = (int)strtol(hexstring, NULL, 16);
  minCellTemp = (float)number;
  //Serial1.println(minCellTemp, 3);
  //Serial1.println();
}

void processMaxCellTemp(char buff0, char buff1, char buff2, char buff3, char buff4)
{
  char hexstring[] = {buff0, buff1, buff2, buff3, buff4};
  int number = (int)strtol(hexstring, NULL, 16);
  maxCellTemp = (float)number;
  //Serial1.println(maxCellTemp, 3);
  //Serial1.println();
}

void processMinCellVoltage(char buff0, char buff1, char buff2, char buff3, char buff4) 
{
  char hexstring[] = {buff0, buff1, buff2, buff3, buff4};
  int number = (int)strtol(hexstring, NULL, 16);
  minCellVoltage = (float)number / 1000;
  //Serial1.println(minCellVoltage, 3);
  //Serial1.println();
}

void processMaxCellVoltage(char buff0, char buff1, char buff2, char buff3, char buff4) 
{
  char hexstring[] = {buff0, buff1, buff2, buff3, buff4};
  int number = (int)strtol(hexstring, NULL, 16);
  maxCellVoltage = (float)number / 1000;
  //Serial1.println(maxCellVoltage, 3);
  //Serial1.println();
}

void processPackStatus(char buff0, char buff1, char buff2, char buff3, char buff4) 
{
  char hexstring[] = {buff0, buff1, buff2, buff3, buff4};
  int number = (int)strtol(hexstring, NULL, 16);
  packStatus = number;
  //Serial1.println(packStatus);
  //Serial1.println();
}



