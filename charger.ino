#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg_recv;
struct can_frame canMsg_send_yes;
struct can_frame canMsg_send_no;

MCP2515 mcp2515(10);

//PIN DEFINES
#define SDC_FINAL 4
#define SDC_BEFORE_FINAL 5

//THRESOLD DEFINES
#define MAX_CURRENT 16
#define MAX_VOLTAGE 260 

//GLOBAL VARIABLES
double send_voltage = 250.0;
double send_current = 0.0  ;
bool dont_send = false ;
double voltage = 0.0 ;
double current = 0.0 ;
bool status_flags[8] = {false} ;
unsigned long t0 = millis() + 1000;
bool latest_message = false ;


 


void setup() {
  Serial.begin(115200);
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS);
  mcp2515.setNormalMode();

  pinMode(SDC_FINAL, INPUT);
pinMode(SDC_BEFORE_FINAL, INPUT);

    //CAN Send Yes VARIABLES
  canMsg_send_yes.can_id  = 0x9806E5F4;
  canMsg_send_yes.can_dlc = 8;
  canMsg_send_yes.data[0] = static_cast<int>((send_voltage)*10) >> 8;
  canMsg_send_yes.data[1] = static_cast<int>((send_voltage)*10) & 255;
  canMsg_send_yes.data[2] = static_cast<int>((send_current)*10) >> 8;
  canMsg_send_yes.data[3] = static_cast<int>((send_current)*10) & 255;
  canMsg_send_yes.data[4] = dont_send ;

  //CAN Send No VARIABLES
  canMsg_send_no.can_id  = 0x9806E5F4;
  canMsg_send_no.can_dlc = 8;
  canMsg_send_no.data[0] = 0;
  canMsg_send_no.data[1] = 0;
  canMsg_send_no.data[2] = 0;
  canMsg_send_no.data[3] = 0;
  canMsg_send_no.data[4] = false ;

  
  Serial.println("------- Charger --------");
}

void loop() {

  if (((digitalRead(SDC_BEFORE_FINAL))&&(digitalRead(SDC_FINAL))) && (t0 - millis() > 1000)) {
    if (send_voltage < MAX_VOLTAGE && send_current < MAX_CURRENT) {
      if (mcp2515.sendMessage(&canMsg_send_yes) == 0) {
          Serial.println("message sent successfully");
        }
      else {
          Serial.println("CAN ERROR message NOT sent") ;
        }
      }
      else {
       if (mcp2515.sendMessage(&canMsg_send_no) == 0) {
        Serial.println("CC/CV Thresolds NOT correct");
        }
       else {
        Serial.println("CAN ERROR message NOT sent") ;
        }

      }
    }

  else {
      if (mcp2515.sendMessage(&canMsg_send_no) == 0) {
          Serial.println("Not charging as SDC is open");
      }
      else{
        Serial.println("CAN ERROR message NOT sent also SDC OPEN") ;
      }

    }

    t0 = millis() ;

  while(millis() - t0 < 1050){
    if (mcp2515.readMessage(&canMsg_recv) == 0) {
      if(canMsg_recv.can_id == 0X98FF50E5 ){  //Check if its the Charger Broadcast message
        Serial.print(canMsg_recv.can_id, HEX); // print ID
        Serial.println(" "); 
        // Extract and assign charger status information
        voltage = (canMsg_recv.data[0] * 256 + canMsg_recv.data[1]) / 10.0;
        current = (canMsg_recv.data[2] * 256 + canMsg_recv.data[3]) / 10.0;
        status_flags[0] = canMsg_recv.data[4] & 0x01;
        status_flags[1] = (canMsg_recv.data[4] >> 1) & 0x01;
        status_flags[2] = (canMsg_recv.data[4] >> 2) & 0x01;
        status_flags[3] = (canMsg_recv.data[4] >> 3) & 0x01;
        status_flags[4] = (canMsg_recv.data[4] >> 4) & 0x01;


        Serial.println("SUCESSFULLY RECEIVED MESSAGE");
        latest_message = true ;
        delay(50) ;
        break ;

        }
    }
    else {
        latest_message = false ;
      }
  }

    
    // PRINT SECTION
    if(latest_message){
    Serial.println("----------Charger Status----------") ;  
    Serial.println(digitalRead(SDC_BEFORE_FINAL))  ;
    Serial.println(digitalRead(SDC_FINAL))         ;


    Serial.print("The voltage is ");
    Serial.println(voltage);

    Serial.print("The current is ");
    Serial.println(current);

    if (status_flags[0] == false)
    Serial.println("Hardware is OK");
    else
    Serial.println("Hardware is NOT OK");

    if (status_flags[1] == false)
    Serial.println("Temperature is OK");
    else
    Serial.println("Temperature is NOT OK");

    if (status_flags[2] == false)
    Serial.println("Input voltage is Normal");
    else
    Serial.println("Input Voltage is Wrong");

    if (status_flags[3] == false)
    Serial.println("Orientation proper");
    else
    Serial.println("Orientation NOT proper");

    if (status_flags[4] == false)
    Serial.println("Communication normal");
    else
    Serial.println("Communication timed-out")   ;

    Serial.println("-----------------------------------") ;

    latest_message = false ;
    }
    

}
