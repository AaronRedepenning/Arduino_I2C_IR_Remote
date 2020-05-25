/********************************************************
 * I2C IR Remote
 * 
 * Receives I2C commands and maps them to IR remote 
 * commands.
 *******************************************************/

#include <Wire.h>
#include <Pronto.h>
#include <IrSenderPwm.h>

#define cnt_of_array(x) (sizeof(x) / sizeof(0[x]))

/*
 * Debug macros
 */
#define DEBUG ( 1 )

#if DEBUG
  #define DBG_PRINT(...)    Serial.print(__VA_ARGS__)
  #define DBG_PRINTLN(...)  Serial.println(__VA_ARGS__)
#else 
  #define DBG_PRINT(...)
  #define DBG_PRINTLN(...)
#endif

/*
 * I2C Slave Address
 */
#define I2C_ADDRESS   ( 0x02 )

/*
 * List of IR Remote Codes
 */
const PROGMEM char power_toggle_code[] = "0000 006C 0000 0022 00AD 00AD 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0041 0016 0041 0016 0041 0016 0041 0016 0041 0016 0041 0016 06FB";
const PROGMEM char channel_up_code[]   = "0000 006C 0000 0022 00AD 00AD 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0041 0016 0041 0016 0016 0016 0041 0016 0041 0016 0041 0016 06FB";
const PROGMEM char channel_down_code[] = "0000 006C 0000 0022 00AD 00AD 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0041 0016 0016 0016 0041 0016 0041 0016 0041 0016 06FB";
const PROGMEM char volume_up_code[]    = "0000 006C 0000 0022 00AD 00AD 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0041 0016 0041 0016 06FB";
const PROGMEM char volume_down_code[]  = "0000 006C 0000 0022 00AD 00AD 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0041 0016 0041 0016 0041 0016 0041 0016 06FB";

/* Add new ir codes here */

/*
 * Mapping of I2C Commands to IR Remote Commands
 */
typedef struct
  {
    uint8_t             i2c_command;
    const char *        ir_code;
    const unsigned int  n_sends;
  } command_map_t;

const command_map_t command_map[] = 
  {
    /* I2C Command                IR Code             Number of times to send IR code */
    { 0x00                      , power_toggle_code,  1                               },
    { 0x01                      , channel_up_code,    1                               },
    { 0x02                      , channel_down_code,  1                               },
    { 0x03                      , volume_up_code,     3                               },
    { 0x04                      , volume_down_code,   3                               }

    /* Add new i2c -> ir command mappings here */
  };
  
uint8_t     command;
bool        recieved_command;  
IrSender *  irSender = IrSenderPwm::getInstance(true);           

/*
 * Function: setup()
 * Description: Initialize variables, setup peripherals
 */
void setup() 
{
  recieved_command = false;
  
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(receiveEvent);

  #if DEBUG
    Serial.begin(9600);
  #endif
  DBG_PRINTLN(F("Running..."));
}

/*
 * Function: loop()
 * Description: The app's main loop
 */
void loop() 
{
  if (recieved_command)
  {
    for (int i = 0; i < cnt_of_array(command_map); i++)
    {
      if (command_map[i].i2c_command == command)
      { 
        DBG_PRINT(F("Command received: "));
        DBG_PRINTLN(command);

        const IrSignal * irSignal = Pronto::parse_PF(command_map[i].ir_code);
        #if DEBUG
          irSignal->dump(Serial, true);
        #endif

        for (int n = 0; n < command_map[i].n_sends; n++)
        {
          irSender->sendIrSignal(*irSignal, 1);
          delay(100);
        }

        delete irSignal;
        
        break; 
      }
    }
    recieved_command = false;
  }
}

/*
 * Function: receiveEvent()
 * Description: Callback for when I2C bytes are received
 */
void receiveEvent(int bytesReceived)
{ 
  DBG_PRINT(F("receiveEvent( "));
  DBG_PRINT(bytesReceived);
  DBG_PRINTLN(" )");
  
  while (bytesReceived--)
  {
    if (!recieved_command)
    {
      command = Wire.read();
      recieved_command = true;
    }
    else
    {
      (void)Wire.read();
    }
  }
}
