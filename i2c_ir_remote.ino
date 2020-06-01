/********************************************************
 * I2C IR Remote
 *
 * Receives I2C commands and maps them to IR remote
 * commands.
 *******************************************************/

#include <avr/wdt.h>

#include <Wire.h>
#include <Pronto.h>
#include <IrSenderPwm.h>

#define cnt_of_array(x) (sizeof(x) / sizeof(0[x]))

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
 * IR Remote Codes
 */
const PROGMEM char power_toggle_code[] = "0000 006C 0000 0022 00AD 00AD 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0041 0016 0041 0016 0041 0016 0041 0016 0041 0016 0041 0016 06FB";
const PROGMEM char channel_up_code[]   = "0000 006C 0000 0022 00AD 00AD 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0041 0016 0041 0016 0016 0016 0041 0016 0041 0016 0041 0016 06FB";
const PROGMEM char channel_down_code[] = "0000 006C 0000 0022 00AD 00AD 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0041 0016 0016 0016 0041 0016 0041 0016 0041 0016 06FB";
const PROGMEM char volume_up_code[]    = "0000 006C 0000 0022 00AD 00AD 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0041 0016 0041 0016 06FB";
const PROGMEM char volume_down_code[]  = "0000 006C 0000 0022 00AD 00AD 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0041 0016 0041 0016 0041 0016 0041 0016 06FB";
const PROGMEM char channel_0_code[]    = "0000 006C 0000 0022 00AD 00AD 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0041 0016 0016 0016 0016 0016 0016 0016 0016 0016 0041 0016 0041 0016 0041 0016 0016 0016 0041 0016 0041 0016 0041 0016 06FB";
const PROGMEM char channel_5_code[]    = "0000 006D 0000 0022 00ac 00ac 0016 0040 0016 0040 0016 0040 0016 0015 0016 0015 0016 0015 0016 0015 0016 0015 0016 0040 0016 0040 0016 0040 0016 0015 0016 0015 0016 0015 0016 0015 0016 0015 0016 0040 0016 0015 0016 0015 0016 0040 0016 0015 0016 0015 0016 0015 0016 0015 0016 0015 0016 0040 0016 0040 0016 0015 0016 0040 0016 0040 0016 0040 0016 0040 0016 071c";

/* Add new IR codes here */

/*
 * IR Remote Code Sequences
 */
const char * volume_up_3_sequence[]    = { volume_up_code,   volume_up_code,   volume_up_code    };
const char * volume_down_3_sequence[]  = { volume_down_code, volume_down_code, volume_down_code  };
const char * channel_50_sequence[]     = { channel_0_code, channel_5_code };

/* Add new IR code sequences here */

/*
 * Mapping of I2C Commands to IR Remote Commands
 */

/* Adds a single IR command */
#define ADD_IR_CODE(_i2c_command, _ir_code) \
          { _i2c_command, (char*[]){_ir_code}, 1 }

/* Adds a IR command sequence */
#define ADD_IR_SEQUENCE(_i2c_command, _ir_sequence) \
          { _i2c_command , _ir_sequence, cnt_of_array(_ir_sequence) }

typedef struct
  {
    uint8_t             i2c_command;
    const char **       ir_code_list;
    const unsigned int  ir_code_list_length;
  } command_map_t;

const command_map_t command_map[] =
  {
    /*                I2C Command     IR Code / IR Code Sequence  */
    ADD_IR_CODE     ( 0x00          , power_toggle_code           ),
    ADD_IR_CODE     ( 0x01          , channel_up_code             ),
    ADD_IR_CODE     ( 0x02          , channel_down_code           ),
    ADD_IR_SEQUENCE ( 0x03          , volume_up_3_sequence        ),
    ADD_IR_SEQUENCE ( 0x04          , volume_down_3_sequence      ),
    ADD_IR_SEQUENCE ( 0x05          , channel_50_sequence         ),

    /* Add new i2c -> IR code/sequence mappings here */
  };

uint8_t     command;
bool        recieved_command;
IrSender *  irSender = IrSenderPwm::getInstance(true);

/*
 * Function: setup()
 * Description: Initialize variables, setup peripherals.
 */
void setup()
{
  wdt_disable();

  recieved_command = false;

  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(receiveEvent);

  #if DEBUG
    Serial.begin(9600);
  #endif
  DBG_PRINTLN(F("Running...\n"));
}

/*
 * Function: loop()
 * Description: The app's main loop.
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

        for (int n = 0; n < command_map[i].ir_code_list_length; n++)
        {
          const IrSignal * irSignal = Pronto::parse_PF(command_map[i].ir_code_list[n]);

          #if DEBUG
            irSignal->dump(Serial, true);
          #endif

          irSender->sendIrSignal(*irSignal, 1);
          delay(100);

          delete irSignal;
        }

        DBG_PRINTLN(F("SUCCESS!\n"));

        /* After a few I2C commands the arduino seems to stop responding.
           Adding a software reset as a temporary workaround until the root cause is found. */
        softwareReset();

        break;
      }
    }
    recieved_command = false;
  }
}

/*
 * Function: receiveEvent()
 * Description: Callback for when I2C bytes are received.
 */
void receiveEvent(int bytesReceived)
{
  DBG_PRINT(F("receiveEvent( "));
  DBG_PRINT(bytesReceived);
  DBG_PRINTLN(F(" )"));

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

/*
 * Function: softwareReset()
 * Description: Software reset MCU by enabling the watchdog
 *              and waiting for it to timeout. Resets in 14 ms.
 * Note: DOES NOT RETURN!
 */
void softwareReset()
{
  wdt_enable(WDTO_60MS);
  while(1) { /* wait for watchdog to timeout */ };
}
