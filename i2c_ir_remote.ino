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
 * Pin to use for PWM
 * IrSenderPwm only supports pin 6 on Arduino Nano Every
 */
#define PIN ( 6 )

/*
 * Debug macros
 */
#define DEBUG ( 0 )

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
 * Supported I2C Commands
 */
typedef uint8_t i2c_command_t; enum
  { 
    I2C_COMMAND_POWER         = 0,
    I2C_COMMAND_CHANNEL_UP    = 1,
    I2C_COMMAND_CHANNEL_DOWN  = 2,
    I2C_COMMAND_VOLUME_UP     = 3,
    I2C_COMMAND_VOLUME_DOWN   = 4,
    
    /* Add new i2c commands here */
    
    I2C_COMMAND_COUNT,
    I2C_COMMAND_INVALID = I2C_COMMAND_COUNT
  };
  
#define I2C_COMMAND_VALID(cmd)  ((cmd != I2C_COMMAND_INVALID) && (cmd < I2C_COMMAND_COUNT))

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
    i2c_command_t       i2c_command;
    const char *        ir_code;
    const unsigned int  n_sends;
  } command_map_t;

const command_map_t command_map[] = 
  {
    /* I2C Command                IR Code             Number of times to send IR code */
    { I2C_COMMAND_POWER         , power_toggle_code,  1                               },
    { I2C_COMMAND_CHANNEL_UP    , channel_up_code,    1                               },
    { I2C_COMMAND_CHANNEL_DOWN  , channel_down_code,  1                               },
    { I2C_COMMAND_VOLUME_UP     , volume_up_code,     3                               },
    { I2C_COMMAND_VOLUME_DOWN   , volume_down_code,   3                               }

    /* Add new i2c -> ir command mappings here */
  };
  
i2c_command_t   recieved_command;

/*
 * Function: setup()
 * Description: Initialize variables, setup peripherals
 */
void setup() 
{
  recieved_command = I2C_COMMAND_INVALID;
  
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(receiveEvent);

  #if DEBUG
    Serial.begin(9600);
  #endif
}

/*
 * Function: loop()
 * Description: The app's main loop
 */
void loop() 
{
  if (I2C_COMMAND_VALID(recieved_command))
  {
    for (int i = 0; i < cnt_of_array(command_map); i++)
    {
      if (command_map[i].i2c_command == recieved_command)
      { 
        DBG_PRINT(F("Command received: "));
        DBG_PRINTLN(recieved_command);

        const IrSignal * irSignal = Pronto::parse_PF(command_map[i].ir_code);
        #if DEBUG
          irSignal->dump(Serial, true);
        #endif

        IrSender * irSender = IrSenderPwm::getInstance(true, PIN);
        for (int n = 0; n < command_map[i].n_sends; n++)
        {
          irSender->sendIrSignal(*irSignal, 1);
          delay(100);
        }
        break; 
      }
    }
    recieved_command = I2C_COMMAND_INVALID;
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
    if (!I2C_COMMAND_VALID(recieved_command))
    {
      recieved_command = Wire.read();
    }
    else
    {
      (void)Wire.read();
    }
  }
}
