#include <SPI.h>
#include "mcp_can.h"

// Pins used by Can controller
//const int SPI_SCK_PIN = 13;
//const int SPI_MISO_PIN = 12;
//const int SPI_MOSI_PIN = 11;
const int SPI_CS_PIN = 9;
MCP_CAN CAN(SPI_CS_PIN);       // Set CS pin
//const int SPI_INT_PIN = 2;
const int InteruptPin = 2;

// Buffer for storing data
char  MsgBuff[50];
int indexBuff = 0;
// Buffer for storing incoming data from Can bus
unsigned char len = 0;
unsigned char buf[8];

const int analogPin = 0;

union Data{     // Used to convert char to float
  char data_buf[4];
  long mesurment;
};

int counter = 0;
    
void setup()
{
    Serial.begin(115200);

    while (CAN_OK != CAN.begin(CAN_1000KBPS))  // init can bus : baudrate = 1000k
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println("Init CAN BUS Shield again");
        delay(100);
    }
    Serial.println("CAN BUS Shield init ok!");
    
}


void loop()
{
    // Read Voltage over Potentiometer to send over Can Bus
    /*
    double volt = ((double) analogRead(analogPin))*5/1023;
    String  msg = String(volt,4);
    char sendmsg[msg.length()];
    msg.toCharArray(sendmsg,msg.length());
    CanWrite(sendmsg);
    delay(200);
    */
    
    // Check wether there is a message on CAN BUS and read it if there is
    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
      CanRead();
    }
}


// Write message to Can bus with CAN ID = 0x01
void CanWrite(char msg[]) {
  CAN.sendMsgBuf(0x01,0,sizeof(msg)+4, msg);
  
}


// Read message from CAN BUS and transmit it over XBee module
void CanRead() 
{
  CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf
  int canId = CAN.getCanId();
  String nameCanId = nameSender(canId);
  if( nameCanId != "NA$")                 // if the id is recognized
    {
      int nameLength = nameCanId.length();
      char Idname[nameLength+1];
      nameCanId.toCharArray(Idname, nameLength+1);
      // add message
      // if message is from Powermeter
      if(canId == 0x521 || canId == 0x524 || canId == 0x525 || canId == 0x526 || canId == 0x528)
      {
        union Data Mesurments;
        Mesurments.data_buf[0] = buf[5];
        Mesurments.data_buf[1] = buf[4];
        Mesurments.data_buf[2] = buf[3];
        Mesurments.data_buf[3] = buf[2];
      
        float current = Mesurments.mesurment;
        if(canId == 0x525)
        {
          current = current / 10;
        }
          
        if(current > 10000)   // if data is to large throw it away 
        {
          return;
        }
        // add message name
        for(int i = 0; i < nameLength; i++) 
        {
          MsgBuff[i+indexBuff] = Idname[i];
        }
        indexBuff = indexBuff + nameLength;
        // add message
        String tmp = String(current);
        for(int t = 0; t < tmp.length(); t++) 
        {
          MsgBuff[indexBuff+t] = tmp[t];
        }
        MsgBuff[indexBuff+tmp.length()] = '$';
        indexBuff = indexBuff + tmp.length() + 1;
      }
      else
      {
        // add message name
        for(int i = 0; i < nameLength; i++) {
          MsgBuff[i+indexBuff] = Idname[i];
        }
        indexBuff = indexBuff + nameLength;
        // add message
        for(int j = 0; j < len; j++)
        {
          MsgBuff[j+indexBuff] = (char) buf[j];
        }
        MsgBuff[indexBuff+len] = '$';
        indexBuff = indexBuff + len + 1;
      }
      xbeeSend(indexBuff);  // send recieved message
  }
        /*  Used for debugging writes all recieved CAN messages to Serial
        Serial.println("-----------------------------");
        Serial.println("get data from ID: ");
        Serial.println(canId);

        for(int i = 0; i<len; i++)    // print the data
        {
            Serial.print(buf[i]);
            Serial.print("\t");
        }
        Serial.println();
        */
}

// send message of length Msglength over Xbee
void xbeeSend(int Msglength) {
    //noInterrupts();
    /*
    for(int i = 0; i <= indexBuff; i++) {
      Serial.print(MsgBuff[i]);
    }
    indexBuff = 0;
    */
    // start message
    Serial.print("#");
    // send message of length Msglength over serial (Xbee)
    for(int i = 0; i < Msglength; i++) {
      Serial.print(MsgBuff[i]);
    }
    // Delete sent message from buffer
    for(int j = 0; j < Msglength && j < 50-Msglength; j++) {
      MsgBuff[j] = MsgBuff[j+Msglength];
    }
    indexBuff = indexBuff-Msglength;
    //Serial.println();
    //interrupts();
}

// Used to identify revieced CAN message to transmit over XBee
String nameSender(int canID) {
  switch(canID) {
    case 0x01:
      return "Volt$";
    case 0x02:
      return "Ohm$";
    case 0x03:
      return "Amp$";
    case 0x521:
      return "PCurrent$";
    case 0x524:
      return "PVolt$";
    case 0x525:
      return "PTemp$";
    case 0x526:
      return "PPower$";
    case 0x528:
      return "PTotEner$";
    default:
      return "NA$";
  }
}



