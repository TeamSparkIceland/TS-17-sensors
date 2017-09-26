#include <SPI.h>
#include "mcp_can.h"


// --- CAN-addresses --------
// mesuements
#define CURRENT 0x521
#define VOLTAGE1 0x524 
#define TEMP 0x525
#define POWER 0x526
#define TOTAL_ENERGY 0x528

#define CAN_RESPONSE 0x511
#define CAN_COMMAND 0x411

//----- VAriable decleration
const int SPI_CS_PIN = 9; // SPI chip select pin
unsigned char RxTxBuf[8]; // data buffer
int canID;  // CAN ID of resiving mesage
union Data{     // Convert char to float
  char data_buf[4];
  long mesurment;
};
String mesage = ""; // Strign for serial mesage
float send_data[5] ={0, 0, 0, 0, 0};  // stores mesured data befor serial transmit
int power_Max = 0; // Stors the peek power mesured

MCP_CAN CAN(SPI_CS_PIN);    // Set CS pin


//----------- Function declerations---------------------
int resive_Mesege(char buf[8]);
float convert_data(char buf[8]);
void reset_buffer();
void power_config();


void setup()
{
    Serial.begin(9600);

    while (CAN_OK != CAN.begin(CAN_1000KBPS))              // init can bus : baudrate = 500k
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println(" Init CAN BUS Shield again");
        delay(100);
    }
    Serial.println("CAN BUS Shield init ok!");

    power_config();
}


void loop()
{ 
  mesage = "";  // Empry string for new transmition
  get_mesurments(); // Saves the mesurments in send_data arrry

  // Abs of current, voltage and power
  if(send_data[0] < 0){
    send_data[0]*=(-1);
  }
  if(send_data[1] < 0){
    send_data[1]*=(-1);
  }
  if(send_data[3] < 0){
    send_data[3]*=(-1);
  }
  
  // saves the peek power in power_Max
  if(send_data[3] > power_Max){
     power_Max = send_data[3];
  }
  
  mesage = "C:" + String(send_data[0]/1000) + " A\t\tV:" + String(send_data[1]/1000) + " V\t\tT:" + String(send_data[2]/10) + " °C\t\tP:" + String(send_data[3]) + " W\t\tPP:" + String(power_Max) + " W\t\tE:" + String(send_data[4])+" Wh";
  Serial.println(mesage); 
}


// wayst for eatch mesurment and saves it in the send_data array
void get_mesurments(){

  while(resive_Mesege(RxTxBuf) != CURRENT); // Wayts for current mesurment
      send_data[0] = convert_data(RxTxBuf);   // Saves mesurment to array
  

  while(resive_Mesege(RxTxBuf) != VOLTAGE1); 
      send_data[1] = convert_data(RxTxBuf);
  

  while(resive_Mesege(RxTxBuf) != TEMP);
      send_data[2] = convert_data(RxTxBuf);
  

  while(resive_Mesege(RxTxBuf) != POWER);
      send_data[3] = convert_data(RxTxBuf);
  

  while(resive_Mesege(RxTxBuf) != TOTAL_ENERGY);
      send_data[4] = convert_data(RxTxBuf);
  
}

// Stores the 8 byte CAN data in buf
// Returns the CAN-ID or -1 if no mesege resived
int resive_Mesege(unsigned char buf[8]){
  
  unsigned char len = 0; // Stores mesage length
  union Data Mesurments; 
  canID = -1;
    
  if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
  {
      CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf
      canID = CAN.getCanId();
      
  }
  return canID;
}

// Converts the char array to a float
float convert_data(unsigned char buf[]){
  
  union Data Mesurments;
  
  Mesurments.data_buf[0] = buf[5];
  Mesurments.data_buf[1] = buf[4];
  Mesurments.data_buf[2] = buf[3];
  Mesurments.data_buf[3] = buf[2];
  
  float current = Mesurments.mesurment;
  return current;
}


// Sets the configeration of power meater
void power_config(){

  // Set mode stop
  reset_buffer();
  RxTxBuf[0] = 0x34;
  RxTxBuf[2] = 0x01;  
  CAN.sendMsgBuf(CAN_COMMAND, 0, 8, RxTxBuf);

  
  reset_buffer();
  while(resive_Mesege(RxTxBuf) != CAN_RESPONSE);
  if(RxTxBuf[0] == 0xB4 && RxTxBuf[1] == 0x00){
      Serial.println("Mode: STOP");
  }
  
  
  /* Dos not work. Config data is wrong
  // Voltage mesutment 2 off
  reset_buffer();
  RxTxBuf[0] = 0x22;
  RxTxBuf[1] = 0x00;
  CAN.sendMsgBuf(CAN_COMMAND, 0, 8, RxTxBuf);
  
  reset_buffer();
  while(resive_Mesege(RxTxBuf) != CAN_RESPONSE);
  if(RxTxBuf[0] == 0xA2 && RxTxBuf[1] == 0x00){
      Serial.println("VOLTAGE 2 OFF");
  }
  
  // Voltage mesutment 3 off
  reset_buffer();
  RxTxBuf[0] = 0x23;
  RxTxBuf[1] = 0x00;
  CAN.sendMsgBuf(CAN_COMMAND, 0, 8, RxTxBuf);
  
  reset_buffer();
  while(resive_Mesege(RxTxBuf) != CAN_RESPONSE);
  if(RxTxBuf[0] == 0xA3 && RxTxBuf[1] == 0x00){
      Serial.println("VOLTAGE 3 OFF");
  }
*/
  // Tempeture mesurment on
  reset_buffer();
  RxTxBuf[0] = 0x24;
  RxTxBuf[1] = 0x02;
  RxTxBuf[3] = 0x14;
  CAN.sendMsgBuf(CAN_COMMAND, 0, 8, RxTxBuf);

  reset_buffer();
  while(resive_Mesege(RxTxBuf) != CAN_RESPONSE);
  if(RxTxBuf[0] == 0xA4 && RxTxBuf[1] == 0x02){
      Serial.println("TEMPETURE ON");
  }
  
  // Power mesurment on
  reset_buffer();
  RxTxBuf[0] = 0x25;
  RxTxBuf[1] = 0x02;
  RxTxBuf[3] = 0x14;
  CAN.sendMsgBuf(CAN_COMMAND, 0, 8, RxTxBuf);

  reset_buffer();
  while(resive_Mesege(RxTxBuf) != CAN_RESPONSE);
  if(RxTxBuf[0] == 0xA5 && RxTxBuf[1] == 0x02){
      Serial.println("POWER ON");
  }

  
  // Energy mesurment on
  reset_buffer();
  RxTxBuf[0] = 0x27;
  RxTxBuf[1] = 0x02;
  RxTxBuf[3] = 0x14;
  CAN.sendMsgBuf(CAN_COMMAND, 0, 8, RxTxBuf);
  
  reset_buffer();
  while(resive_Mesege(RxTxBuf) != CAN_RESPONSE);
  if(RxTxBuf[0] == 0xA7 && RxTxBuf[1] == 0x02){
      Serial.println("ENERGY ON");
  }
  
  // Store configerations
  reset_buffer();
  RxTxBuf[0] = 0x32;
  CAN.sendMsgBuf(CAN_COMMAND, 0, 8, RxTxBuf);

  reset_buffer();
  while(resive_Mesege(RxTxBuf) != CAN_RESPONSE);
  if(RxTxBuf[0] == 0xB2 && RxTxBuf[1] == 0x02){
      Serial.println("CONFIG STRED");
  }
  
  // Set mode run
  reset_buffer();
  RxTxBuf[0] = 0x34;
  RxTxBuf[1] = 0x01;
  RxTxBuf[2] = 0x01;  
  CAN.sendMsgBuf(CAN_COMMAND, 0, 8, RxTxBuf);
  
  reset_buffer();
  while(resive_Mesege(RxTxBuf) != CAN_RESPONSE);
  if(RxTxBuf[0] == 0xB4 && RxTxBuf[1] == 0x01){
    Serial.println("Mode: RUN");
  }
  
}

// Clears the mesage buffer
void reset_buffer(){
  for(uint8_t caunt = 0; caunt < 8; caunt++){
    RxTxBuf[caunt] = 0x00;
  }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
