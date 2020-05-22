/********************************************************
 * I2C IR Remote
 * 
 * Receives I2C commands and maps them to IR remote 
 * commands.
 *******************************************************/

#include <Wire.h>
#include <IRremote.h>

#define cnt_of_array(x) (sizeof(x) / sizeof(0[x]))

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
    i2c_command_t i2c_command;
    const char *  ir_code;
  } command_map_t;

const command_map_t command_map[] = 
  {
    /* I2C Command                IR Code           */
    { I2C_COMMAND_POWER         , power_toggle_code },
    { I2C_COMMAND_CHANNEL_UP    , channel_up_code   },
    { I2C_COMMAND_CHANNEL_DOWN  , channel_down_code },
    { I2C_COMMAND_VOLUME_UP     , volume_up_code    },
    { I2C_COMMAND_VOLUME_DOWN   , volume_down_code  }

    /* Add new i2c -> ir command mappings here */
  };
  
IRsend          ir_send;
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
    int i;
    
    for (i = 0; i < cnt_of_array(command_map); i++)
    {
      if (command_map[i].i2c_command == recieved_command)
      { 
        static unsigned int code[256];
        static int          len;
        static int          freq;

        DBG_PRINT(F("Command received: "));
        DBG_PRINTLN(recieved_command);

        convertProntoProgmem(command_map[i].ir_code, code, &len, &freq);
        
        DBG_PRINT(F("Pronto Code: "));
        DBG_PRINTLN(command_map[i].ir_code);
        
        DBG_PRINT(F("Raw("));
        DBG_PRINT(len);
        DBG_PRINT(F("): "));
        for (int i = 0; i < len; i++)
        {
          DBG_PRINT(code[i]);
          DBG_PRINT(" ");
        }
        DBG_PRINTLN("")
        
        DBG_PRINT(F("Frequency: "));
        DBG_PRINTLN(freq);
        
        ir_send.sendRaw(code, len, freq);
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

/*
 * Function: convertProntoProgmem()
 * Description: Convert a pronto hex string, stored in FLASH, to code length frequency
 *              values required by the IRremote library
 */
void convertProntoProgmem(PGM_P i_pronto, unsigned int * o_code, int * o_length, int * o_frequency)
{
  int       i = 0;
  uint16_t  j = 0;
  uint16_t  arr[128];

  while (i < strlen_P(i_pronto))
  {
    char hexchararray[5];
    strncpy_P(hexchararray, i_pronto + i, 4);
    hexchararray[4] = '\0';

    unsigned int hexNumber = hexToInt(hexchararray);
    arr[j++] = hexNumber;
    i = i + 5;
  }

  float carrierFrequency = 1000000 / (arr[1] * 0.241246);

  int codeLength = arr[2];
  if (codeLength == 0) codeLength = arr[3];

  int repeatCodeLength = arr[3];
  *o_length = codeLength*2;

  int index = 0;
  
  for (int i = 4; i < j; i++ )
  {
    int convertedToMicrosec = (1000000 * (arr[i] / carrierFrequency) + 0.5);
    o_code[index++] = convertedToMicrosec;
  }

  *o_frequency = (int)(carrierFrequency / 1000);
}

/*
 * Function: hexToInt()
 * Description: Convert hex string in integer. Ported from:
 *               http://stackoverflow.com/questions/4951714/c-code-to-convert-hex-to-int
 */
unsigned int hexToInt(const char *hex)
{
  unsigned int result = 0;
  while (*hex)
  {
    if (*hex > 47 && *hex < 58)
      result += (*hex - 48);
    else if (*hex > 64 && *hex < 71)
      result += (*hex - 55);
    else if (*hex > 96 && *hex < 103)
      result += (*hex - 87);
    if (*++hex)
      result <<= 4;
  }
  return result;
}
