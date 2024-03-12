// ======================================================================================
// 
//          ESP32 Based Corsa-e Battery SOC Gauge And More Created By C. Curtis
// https://github.com/JackelLab/CorsaOBDGauge
//
// Copyright (c) 2024 C.Curtis/JackelLab
//
// I offer these files for you to use, for personal use, free of charge. You may copy
// these files, modify and reupload these files providing you are not using them for
// financial gain and must include the header and details of where the original files
// maybe downloaded.
//
// Anyone wishing to use these files for financial gain, must request permission to do so.
//
// The libraries used within the software are copyright by their relevant owners and
// separate permission will have to be gained through them.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ======================================================================================


// ======================================================================================
// Version
// ======================================================================================
// beta - First release to github

// ======================================================================================
// Possible Future Features
// ======================================================================================
// Select between SOC and SOC calibrated
// Journey based range estimate
// Reset delta without having to turn power off
// Software selection of bluetooth MAC address instead of hard coding or automatic search

// ======================================================================================
// Options
// ======================================================================================

#define BackLightDim 127    // PWM level when illumination input is high
#define BackLightBright 255 // PWM level when illumination input is low
#define SOHDelay 1200       // Delay time to allow SOH to be seen
#define RefreshPeriod 1000  // Time in mS for main loop

#define BTMODULE 1          // 0 - No BT module, 1 - BT module
#define DEMOMODE 0          // 0 - Normal mode, 1 - Demo mode

// When demomode = 1, use the following values at the start
#define DemoSOH 87.6;
#define DemoSOC 91.01;
#define DemoSOCDelta 100.0;
#define DemoTemperature 12;
#define DemoVoltage 13.8;
#define DemoSpeed 0.0;

#define LCDOrientation 1    // 1 is newer PCB, 3 is older (aplha) PCB

#define SpeedCorrection 208

// --------------------------------------------------------------------------------------
// Colour Options
// --------------------------------------------------------------------------------------

#define SOCGood TFT_WHITE
#define SOCOk TFT_YELLOW
#define SOCBad TFT_RED
#define SOCOkLevel 10
#define SOCBadLevel 2

#define TempHot TFT_RED
#define TempCaution 0xFFE0              // Yellow
#define TempOk TFT_WHITE
#define TempCold 0x03FF                 // Cyanish blue
#define TempHotLevel 50
#define TempCautionLevel 40
#define TempColdLevel 2

#define DeltaSOCPlus TFT_GREEN
#define DeltaSOCNegative 0xFC10         // Whiter red

#define SOHGood TFT_WHITE
#define SOHOk TFT_WHITE
#define SOHBad TFT_YELLOW
#define SOHFlagPole TFT_WHITE
#define SOHFlagColour TFT_BLUE
#define SOHOkLevel 80
#define SOHBadLevel 50

#define VoltageHigh TFT_RED
#define VoltageCautionHigh TFT_YELLOW
#define VoltageGood TFT_GREEN
#define VoltageOk TFT_WHITE
#define VoltageLow TFT_YELLOW
#define VoltageLowBad TFT_RED
#define VoltageHighLevel 15.0
#define VoltageCautionHighLevel 14.6
#define VoltageOkLevel 13.8
#define VoltageLowLevel 12.5
#define VoltageLowBadLevel 12.0

#define InvalidDataColour TFT_RED

// Colours are a 16 bit number in the format of 5-6-5. 5 red bits, 6 green and 5 blue.
// Red is 0xF800, green is 0x07E0, blue is 0x001F

// ======================================================================================
// Includes
// ======================================================================================

#include <Arduino.h>
#include "SPI.h"
#include "TFT_eSPI.h"
#include "Free_Fonts.h"

#include "Icons.h"

// EEPROM include and definitions
#include <EEPROM.h>
#define EEPROM_SIZE 64

// ======================================================================================
// Definitions
// ======================================================================================

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite page = TFT_eSprite(&tft);

// Bluetooth setup
#if BTMODULE == 1
// Bluetooth includes
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
uint8_t OBDAddress[6]  = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};  // OBD MAC address
#endif

// Test positions
#define ROW1L 36
#define ROW2L 76
#define ROW2S 73

#define NoOfModes 9
// Modes:
// 0 - TL = SOC, BL = Temperature and SOC Delta
// 1 - TL = SOC, BL = Range 1 and SOC Delta
// 2 - TL = SOC, BL = Range 1 and Speed
// 3 - TL = SOC, BL = Temperature and Speed
// 4 - TL = SOC, BL = Range 1 and Range 5
// 5 - TL = SOC, BL = Temperature and 12V Battery
// 6 - TL = SOC, BL = 12V Battery and SOC Delta
// 7 - TL = SOC, BL = 12V Battery and Speed
// 8 - TL = SOC, BL = Speed

// Distance covered by traveling at 1mph for 1 second
#define SpeedFactor 0.000277777

// ======================================================================================
// Icon Location Definitions (Splash Screen)
// ======================================================================================

#define BTx 68
#define BTy 20
#define CPUx 0
#define CPUy 24
#define CARx 128
#define CARy 24

// ======================================================================================
// IO defintions
// ======================================================================================

#define BackLight 12      // LCD Backlight output pin
#define Button 13         // Reed switch input
#define Illumination 14   // Illumination control input

// ======================================================================================
// Comms Details
// ======================================================================================

#define CommsTimeOut 3000                   // Time in mS (General comms timeout)
#define SearchingTimeOut 10000              // Time in mS (timeout after issuing a reset)

// ======================================================================================
// Variables
// ======================================================================================

#if DEMOMODE == 1
// Demo mode variables with preset values
float SOC = DemoSOC;
int SOCDemoMode = 0;
float StartSOC = DemoSOCDelta;
float SOH = DemoSOH;
bool SOHFlag = true;
uint16_t Speed;
float SpeedF = DemoSpeed;
int Temperature = DemoTemperature;
float BatteryVoltage = DemoVoltage;
bool SpeedDirection = false;
#else
float SOC;
float StartSOC;
float SOH;
bool SOHFlag = false;
uint16_t Speed;
float SpeedF;
int Temperature;
float BatteryVoltage;
#endif

char ReceiveBuff[48];     // Buffer has to be big enough to hold
                          //  SEARCHING... dUNABLE TO CONNECT dd>
uint8_t ReceiveBuffCount = 0;

float dSOC;
bool LastBacklight;
bool IlluminationInput;
bool Stuck;
uint16_t tp;
bool Timeout;
bool ValidData;
unsigned long LastUpdate;

bool ModeChange = false;
bool ModeChangeInput;
uint8_t Mode = 0;
bool UseEEPROM;

float BufferD[300];
float BufferP[300];
uint16_t Pointer1 = 0;
uint16_t Pointer5 = 0;
uint16_t PointerMax = 0;
float Distance1 = 0.0;
float Distance5 = 0.0;
float Percent1 = 0.0;
float Percent5 = 0.0;
float Range1;
float Range5;
int Range;
float t;
uint16_t n;

// ======================================================================================
// Main Routine
// ======================================================================================

void setup()
{
  // Use serial for debugging and logging
  Serial.begin(115200);
  
  // Setup IO
  //pinMode(Button, INPUT);
  pinMode(Button, INPUT_PULLUP);
  pinMode(Illumination, INPUT);
  
  // Setup LCD using TFT eSPI
  tft.init();
  tft.setRotation(LCDOrientation);  // 3 is older design, 1 is newer design
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);

  // Setup backlight using PWM
  ledcSetup(1, 1000, 8);        // Setup PWM channel
  ledcAttachPin(BackLight, 1);  // Assign backlight to channel
  // Check Illumination input and set backlight as approprate
  if(digitalRead(Illumination) == LOW)
  {
    ledcWrite(1, BackLightDim);
    LastBacklight = true;
  }
  else
  {
    ledcWrite(1, BackLightBright);
    LastBacklight = false;
  }

  // Load last mode from EEPROM 
  if(!EEPROM.begin(EEPROM_SIZE))
  {
    // Starting EEPROM failed so don't use it
    UseEEPROM = false;
  }
  else
  {
    UseEEPROM = true;
    // Is the EEPROM already setup?
    if((uint8_t) EEPROM.read(0) != 'M')
    {
      // EEPROM is not setup
      EEPROM.write(0, 'M');
      EEPROM.write(1, Mode);
      EEPROM.commit();
    }
    Mode = EEPROM.read(1);
  }
  
  // Setup page for quick drawing on the LCD
  //page.setAttribute(PSRAM_ENABLE, true);
  page.createSprite(160, 80);
  
  // Draw icon's and a bar showing communication point
  //tft.fillScreen(TFT_BLACK);
  tft.pushImage(BTx, BTy, 25, 39, BT_G, TFT_BLACK);
  tft.pushImage(CPUx, CPUy, 32, 32, CPU, TFT_BLACK);
  tft.pushImage(CARx, CARy, 32, 32, CAR, TFT_BLACK);
  tft.fillRect(CPUx + 34, 38, BTx - CPUx - 36, 4, TFT_WHITE);
  
#if BTMODULE == 1
  // Bluetooth serial and ODB stuff
  SerialBT.setPin("1234");
  SerialBT.begin("Corsa-e", true);
  
  // Try and connect to OBD via bluetooth serial
  Stuck = true;
  while(Stuck)
  {
    if(!SerialBT.connect(OBDAddress))
    {
      // Can't connect to OBDII device, show message.
      tft.pushImage(BTx, BTy, 25, 39, BT_R, TFT_BLACK);
      tft.fillRect(CPUx + 34, 38, BTx - CPUx - 36, 4, TFT_RED);
      delay(1000);
    }
    else
    {
      // Connected, exit the loop
      Stuck = false;
    }
  }
  // Show that chip to BT module is successful
  tft.fillRect(CPUx + 34, 38, BTx - CPUx - 36, 4, TFT_GREEN);
  Serial.println("BT Connected");
  // BT icon to blue
  tft.pushImage(BTx, BTy, 25, 39, BT_B, TFT_BLACK);
  // Show new communication point, BT to car
  tft.fillRect(BTx + 27, 38, CARx - BTx - 29, 4, TFT_WHITE);
  
  // Communicate with OBD II, start with reset command
  Stuck = true;
  while(Stuck)
  {
    Timeout = SendOBD("ATZ", SearchingTimeOut);
    if(ReceiveBuffCount == 0 || Timeout)
    {
      // Couldn't talk to ELM327
      // Show red bluetooth icon?
      //tft.pushImage(BTx, BTy, 25, 39, BT_R, TFT_BLACK);
      // Show red line
      tft.fillRect(BTx + 27, 38, CARx - BTx - 29, 4, TFT_RED);
      delay(2000);
    }
    else
    {
      Stuck = false;
    }
  }
  // Clear the line to show communication steps
  tft.fillRect(BTx + 27, 38, CARx - BTx - 29, 4, TFT_BLACK);
  
  // Setup and start OBD
  Timeout = SendOBD("ATE0", CommsTimeOut);    // Echo off
  tft.fillRect(BTx + 27, 38, 2, 4, TFT_GREEN);
  
  Timeout = SendOBD("ATH1", CommsTimeOut);    // Headers on
  tft.fillRect(BTx + 30, 38, 2, 4, TFT_GREEN);
  
  Timeout = SendOBD("ATS0", CommsTimeOut);    // Spaces off
  tft.fillRect(BTx + 33, 38, 2, 4, TFT_GREEN);
  
  Timeout = SendOBD("ATM0", CommsTimeOut);    // Memory off
  tft.fillRect(BTx + 36, 38, 2, 4, TFT_GREEN);
  
  Timeout = SendOBD("ATSPA6", CommsTimeOut);   // Protocol automatic 6
  tft.fillRect(BTx + 39, 38, 2, 4, TFT_GREEN);
  
  Timeout = SendOBD("ATAT1", CommsTimeOut);    // Adaptive timing auto1
  tft.fillRect(BTx + 42, 38, 2, 4, TFT_GREEN);
  
  Timeout = SendOBD("ATSH6B4", CommsTimeOut);    // Set header 6B4 (this has SOC and SOH)
  tft.fillRect(BTx + 45, 38, 2, 4, TFT_GREEN);
  
  Timeout = SendOBD("ATFCSH6B4", CommsTimeOut);    // Flow control set header 6B4
  tft.fillRect(BTx + 48, 38, 2, 4, TFT_GREEN);
  
  Timeout = SendOBD("ATFCSD300000", CommsTimeOut);    // Set data to 300000
  tft.fillRect(BTx + 51, 38, 2, 4, TFT_GREEN);
  
  Timeout = SendOBD("ATFCSM1", CommsTimeOut);    // Flow control mode 1
  tft.fillRect(BTx + 54, 38, 2, 4, TFT_GREEN);
  
  Timeout = SendOBD("ATCRA694", CommsTimeOut);    // Receive address 694
#endif
#if DEMOMODE == 0
  // Get SOC
  Timeout = SendOBD("22D410", CommsTimeOut);    // Request SOC
  // Reply example 6940562D410ABA8dd>
  ValidData = true;
  // Check reply is valid
  if(ReceiveBuff[8] == 0x34 && ReceiveBuff[9] == 0x31 && ReceiveBuff[10] == 0x30 && Timeout == false)
  {
    // Valid data
    tp = Hex2Dec(ReceiveBuff[11], ReceiveBuff[12]);
    tp = (tp * 256) + Hex2Dec(ReceiveBuff[13], ReceiveBuff[14]);
    SOC = ((float) tp) / 512.0;
  }
  else
  {
    // Invalid data
    SOC = 0.0;
    ValidData = false;
  }
  // Copy SOC for delta SOC
  StartSOC = SOC;
  
  // Get SOH (we will only show this once)
  Timeout = SendOBD("22D860", CommsTimeOut);    // Request SOH
  // Reply example 6940562D8600105EDdd>
  // Check reply is valid
  if(ReceiveBuff[8] == 0x38 && ReceiveBuff[9] == 0x36 && ReceiveBuff[10] == 0x30 && Timeout == false)
  {
    // Valid data
    tp = Hex2Dec(ReceiveBuff[13], ReceiveBuff[14]);
    tp = (tp * 256) + Hex2Dec(ReceiveBuff[15], ReceiveBuff[16]);
    SOH = ((float) tp) / 16.0;
    if(ReceiveBuff[12] != 0x30)
    {
      SOHFlag = true;
      // Not sure what this flag is for
    }
  }
  else
  {
    // Invalid data
    SOH = 0.0;
    ValidData = false;
  }

  // Switch to 6A2/682 for battery temperature
  Timeout = SendOBD("ATSH6A2", CommsTimeOut);    // Set header 6A2
  Timeout = SendOBD("ATFCSH6A2", CommsTimeOut);    // Flow control set header 6A2
  Timeout = SendOBD("ATCRA682", CommsTimeOut);    // Receive address 682
  
  // Get battery temperature
  Timeout = SendOBD("22D8EF", CommsTimeOut);    // Request Temperature
  // Check if reply is valid
  if(ReceiveBuff[8] == 0x38 && ReceiveBuff[9] == 0x45 && ReceiveBuff[10] == 0x46 && Timeout == false)
  {
    // Valid data
    Temperature = (int) (Hex2Dec(ReceiveBuff[11], ReceiveBuff[12]));
    // If bit 7 is set (128-255), it's negative
    if(Temperature > 127)
    {
      // Make it negative
      Temperature = Temperature - 256;
    }
  }
  else
  {
    // Invalid data
    Temperature = 0;
    ValidData = false;
  }
#else
  tft.fillRect(BTx + 27, 38, CARx - BTx - 29, 4, TFT_GREEN);
  delay(500);
  ValidData = true;
#endif
    
  // If we have valid data, show the SOH
  if(ValidData == true)
  {
    DisplaySOH();
  }
  else
  {
    DisplayBadData();
  }
  // Delay for a moment so SOH can be read
  delay(SOHDelay);
}

// ======================================================================================
// Main Loop
// ======================================================================================

void loop()
{
  // Update loop delay
  LastUpdate = millis();
  ValidData = true;
  
#if DEMOMODE == 0
  // Switch to 6B4/694
  Timeout = SendOBD("ATSH6B4", CommsTimeOut);    // Set header 6B4
  Timeout = SendOBD("ATFCSH6B4", CommsTimeOut);    // Flow control set header 6B4
  Timeout = SendOBD("ATCRA694", CommsTimeOut);    // Receive address 694
  // Set SOC
  Timeout = SendOBD("22D410", CommsTimeOut);    // Request SOC
  if(ReceiveBuff[8] == 0x34 && ReceiveBuff[9] == 0x31 && ReceiveBuff[10] == 0x30 && Timeout == false)
  {
    // Valid data
    tp = Hex2Dec(ReceiveBuff[11], ReceiveBuff[12]);
    tp = (tp * 256) + Hex2Dec(ReceiveBuff[13], ReceiveBuff[14]);
    SOC = ((float) tp) / 512.0;
  }
  else
  {
    // Invalid data
    SOC = 0.0;
    ValidData = false;
  }
  
  // Switch to 6A2/682
  Timeout = SendOBD("ATSH6A2", CommsTimeOut);    // Set header 6A2
  Timeout = SendOBD("ATFCSH6A2", CommsTimeOut);    // Flow control set header 6A2
  Timeout = SendOBD("ATCRA682", CommsTimeOut);    // Receive address 682
  // Get battery temperature
  Timeout = SendOBD("22D8EF", CommsTimeOut);    // Request Temperature
  if(ReceiveBuff[8] == 0x38 && ReceiveBuff[9] == 0x45 && ReceiveBuff[10] == 0x46 && Timeout == false)
  {
    // Valid data
    Temperature = (int) (Hex2Dec(ReceiveBuff[11], ReceiveBuff[12]));
    if(Temperature > 127)
    {
      // Make it negative
      Temperature = Temperature - 256;
    }
  }
  else
  {
    // Invalid data
    Temperature = 0;
    ValidData = false;
  }
  
  // Speed
  Timeout = SendOBD("22D402", CommsTimeOut);    // Request Temperature
  if(ReceiveBuff[8] == 0x34 && ReceiveBuff[9] == 0x30 && ReceiveBuff[10] == 0x32 && Timeout == false)
  {
    // Valid data
    Speed = ((uint16_t) (Hex2Dec(ReceiveBuff[11], ReceiveBuff[12]))) << 8;
    Speed = Speed + (uint16_t) (Hex2Dec(ReceiveBuff[13], ReceiveBuff[14]));
    SpeedF = ((float) Speed) / SpeedCorrection;
  }
  else
  {
    // Invalid data
    Speed = 0;
    SpeedF = 0.0;
    ValidData = false;
  }
  
  // Get 12V battery voltage
  Timeout = SendOBD("ATRV", CommsTimeOut);    // Request vattery voltage
  if(ReceiveBuff[ReceiveBuffCount - 3] != 'V' || Timeout == true)
  {
    // Incorrect response
    BatteryVoltage == 1.2;
  }
  else
  {
    if(ReceiveBuffCount == 7)
    {
      // Result is >= 10.0V
      BatteryVoltage = (float)((ReceiveBuff[0] - '0') * 10) + (float)(ReceiveBuff[1] - '0') + ((float)(ReceiveBuff[3] - '0') / 10.0);
    }
    else if(ReceiveBuffCount == 6)
    {
      // Result is < 10.0V
      BatteryVoltage = (float)(ReceiveBuff[0] - '0') + ((float)(ReceiveBuff[2] - '0') / 10.0);
    }
    else
    {
      // Unknown
      BatteryVoltage = 2.3;
    }
  }
#else
  if(SOC < 90.0 && SOCDemoMode == 0)
  {
    SOCDemoMode = 1;
  }
  else if(SOC > 95.0 && SOCDemoMode == 1)
  {
    SOCDemoMode = 2;
  }
  else if(SOCDemoMode == 0 || SOCDemoMode == 2)
  {
    SOC = SOC - 0.01;
    if(SOC < 0.0)
    {
      SOC = 100.0;
      SOCDemoMode = 0;
    }
  }
  else
  {
    SOC = SOC + 0.01;
  }
  Temperature++;
  if(Temperature > 60)
  {
    Temperature = -12;
  }
  if(SpeedDirection == false)
  {
    SpeedF = SpeedF + 0.5;
  }
  else
  {
    SpeedF = SpeedF - 0.5;
  }
  if(SpeedF > 60.0)
  {
    SpeedDirection = true;
  }
  if(SpeedF < 0.0)
  {
    SpeedF = 0.0;
    SpeedDirection = false;
  }
#if BTMODULE == 1
  // Get 12V battery voltage
  Timeout = SendOBD("ATRV", CommsTimeOut);    // Request vattery voltage
  if(ReceiveBuff[ReceiveBuffCount - 3] != 'V' || Timeout == true)
  {
    // Incorrect response
    BatteryVoltage == 1.2;
  }
  else
  {
    if(ReceiveBuffCount == 7)
    {
      // Result is >= 10.0V
      BatteryVoltage = (float)((ReceiveBuff[0] - '0') * 10) + (float)(ReceiveBuff[1] - '0') + ((float)(ReceiveBuff[3] - '0') / 10.0);
    }
    else if(ReceiveBuffCount == 6)
    {
      // Result is < 10.0V
      BatteryVoltage = (float)(ReceiveBuff[0] - '0') + ((float)(ReceiveBuff[2] - '0') / 10.0);
    }
    else
    {
      // Unknown
      BatteryVoltage = 2.3;
    }
  }
#else
  BatteryVoltage = BatteryVoltage + 0.1;
  if(BatteryVoltage >= 16.0)
  {
    BatteryVoltage = 8.0;
  }
#endif
#endif
  
  // Work on buffered data
  // Shift buffer
  for(n = 299; n != 0; n--)
  {
    BufferD[n] = BufferD[n - 1];
    BufferP[n] = BufferP[n - 1];
  }
  // Save new data
  BufferD[0] = SpeedF;
  BufferP[0] = SOC;
  // Calculate ranges
  t = 0.0;
  for(n = 0; n != Pointer1; n++)
  {
    t = t + BufferD[n];
  }
  Distance1 = t * SpeedFactor;
  t = 0.0;
  for(n = 0; n != Pointer5; n++)
  {
    t = t + BufferD[n];
  }
  Distance5 = t * SpeedFactor;
  Percent1 = BufferP[Pointer1] - BufferP[0];
  Percent5 = BufferP[Pointer5] - BufferP[0];
  
  // CAUTION, need to do divide by zero check!!!!
  if(Distance1 != 0.0 && Percent1 != 0.0)
  {
    if(Percent1 < 0.0)
    {
      Range1 = 999.0;
    }
    else
    {
      Range1 = Distance1 / Percent1 * BufferP[0];
    }
  }
  else if(Distance1 == 0.0)
  {
    Range1 = 0.0;
  }
  else
  {
    Range1 = 999.0;
  }
  if(Distance5 != 0.0 && Percent5 != 0.0)
  {
    if(Percent5 < 0.0)
    {
      Range5 = 999.0;
    }
    else
    {
      Range5 = Distance5 / Percent5 * BufferP[0];
    }
  }
  else if(Distance5 == 0.0)
  {
    Range5 = 0.0;
  }
  else
  {
    Range5 = 999.0;
  }
  if(Pointer1 != 59)
  {
    Pointer1++;
  }
  if(Pointer5 != 299)
  {
    Pointer5++;
  }
  
  // If we have valid data, display it, otherwise all ----
  if(ValidData == true)
  {
    DisplayReadings();
  }
  else
  {
    DisplayBadData();
  }
  
  // Loop delay, while in the loop, check for a low on the backlight input
  //  (could be a PWM input) or the magnetic switch (mode change)
  IlluminationInput = false;
  ModeChangeInput = false;
  if(digitalRead(Button) == HIGH && ModeChange == true)
  {
    ModeChange = false;
  }

  while((LastUpdate + RefreshPeriod) > millis())
  {
    if(digitalRead(Illumination == LOW))
    {
      IlluminationInput = true;
    }
    if(digitalRead(Button) == LOW)
    {
      ModeChangeInput = true;
    }
  }
  
  // Check if the backlight has changed
  if(IlluminationInput == true && LastBacklight == false)
  {
    // Dim backlight
    ledcWrite(1, BackLightDim);
    LastBacklight = true;
  }
  if(IlluminationInput == false && LastBacklight == true)
  {
    // Full brightness
    ledcWrite(1, BackLightBright);
    LastBacklight = false;
  }
  
  // Check if the reed switch is activated to change the mode
  if(ModeChangeInput == true && ModeChange == false)
  {
    Mode++;
    if(Mode == NoOfModes)
    {
      Mode = 0;
    }
    ModeChange = true;
    // Save changes to EEPROM?
    if(UseEEPROM == true)
    {
      EEPROM.write(1, Mode);
      EEPROM.commit();
    }
  }
  else if(ModeChangeInput == false && ModeChange == true)
  {
    ModeChange = false;
  }
}

// ======================================================================================
// Display Routines
// ======================================================================================

void DisplayReadings()
{
  page.fillRect(0, 0, 160, 80, TFT_BLACK);
  //page.fillRect(0, 0, 160, 80, 0x38E7);   // Use to help see no of pixels out
  page.setFreeFont(FF20);
  if(SOC > SOCOkLevel)
  {
    page.setTextColor(SOCGood);
  }
  else if(SOC > SOCBadLevel)
  {
    page.setTextColor(SOCOk);
  }
  else
  {
    page.setTextColor(SOCBad);
  }

  // State of charge
  if(SOC >= 100.0)
  {
    page.setCursor(2, ROW1L);
    page.print(SOC, 1);
  }
  else if(SOC < 10.0)
  {
    page.setCursor(25, ROW1L);
    page.print(SOC, 2);
  }
  else
  {
    page.setCursor(2, ROW1L);
    page.print(SOC, 2);
  }
  page.print((char) 0x25);
  
  switch(Mode)
  {
    case 0:
      // Temperature and SOC Delta
      // Display temperature
      DisplayTemperature(0);
      // Display Delta
      DisplayDelta(63, true);
      break;
    case 1:
      // Range1 estimate and SOC Delta
      // Display Range 1
      DisplayRange1();
      DisplayDelta(63, false);
      break;
    case 2:
      // Range estimate and Speed
      // Display Range 1
      DisplayRange1();
      // Display Speed
      //DisplaySpeed(62);
      DisplaySpeed(95);
      break;
    case 3:
      // Temperature and Speed
      // Display temperature
      DisplayTemperature(0);
      // Display Speed
      DisplaySpeed(62);
      break;
    case 4:
      // Range estimate
      // Display Range 1
      DisplayRange1();
      // Display Range 5
      DisplayRange5();
      break;
    case 5:
      // Temperature and 12V Battery
      // Display temperature
      DisplayTemperature(0);
      // Display 12V Battery
      DisplayBattery(100);
      break;
    case 6:
      // 12V Battery and SOC Delta
      // Display 12V Battery
      DisplayBattery(0);
      // Display Delta
      DisplayDelta(63, true);
      break;
    case 7:
      // 12V Battery and Speed
      // Display 12V Battery
      DisplayBattery(0);
      // Display Speed
      DisplaySpeed(62);
      break;
    case 8:
      // Speed
      DisplaySpeedBig(16);
      break;
  }
  
  // Draw buffer to the LCD
  page.pushSprite(0, 0);
}

void DisplayRange1(void)
{
  page.setTextColor(TFT_GREEN);
  page.setFreeFont(FF18);
  page.setCursor (0, ROW2S);
  page.print("(1)");
  page.setTextColor(TFT_WHITE);
  if(Range1 < 0.0)
  {
    Range = 0;
  }
  else if(Range1 > 999.0)
  {
    Range = 999;
  }
  else
  {
    Range = (int) Range1;
  }
  page.print(Range);
}

void DisplayRange5(void)
{
  page.setTextColor(TFT_GREEN);
  page.setFreeFont(FF18);
  // 5 min
  page.setCursor (80, ROW2S);
  page.print("(5)");
  page.setTextColor(TFT_WHITE);
  if(Range5 < 0.0)
  {
    Range = 0;
  }
  else if(Range5 > 999.0)
  {
    Range = 999;
  }
  else
  {
    Range = (int) Range5;
  }
  page.print(Range);
}

void DisplayBattery(int offsetx)
{
  if(BatteryVoltage > VoltageHighLevel)
  {
    page.setTextColor(VoltageHigh);
  }
  else if(BatteryVoltage > VoltageCautionHighLevel)
  {
    page.setTextColor(VoltageCautionHigh);
  }
  else if(BatteryVoltage > VoltageOkLevel)
  {
    page.setTextColor(VoltageGood);
  }
  else if(BatteryVoltage <= VoltageLowBadLevel)
  {
    page.setTextColor(VoltageLowBad);
  }
  else if(BatteryVoltage <= VoltageLowLevel)
  {
    page.setTextColor(VoltageLow);
  }
  else
  {
    page.setTextColor(VoltageOk);
  }

  page.setFreeFont(FF18);
  if(BatteryVoltage > 9.99)
  {
    page.setCursor (offsetx, ROW2S);
  }
  else
  {
    page.setCursor (offsetx + 8, ROW2S);
  }
  page.print(BatteryVoltage, 1);
  page.print("V");
}

void DisplaySpeedBig(int offsetx)
{
  // No need to worry about 100+ as Corsa can't do more than 93
  page.setFreeFont(FF20);
  page.setTextColor(TFT_WHITE);
  if(SpeedF > 9.99)
  {
    page.setCursor (offsetx, ROW2L);
  }
  else
  {
    page.setCursor (offsetx + 26, ROW2L);
  }
  page.print(SpeedF, 1);
  page.setFreeFont(FF18);
  page.print("MPH");
}

void DisplaySpeed(int offsetx)
{
  // No need to worry about 100+ as Corsa can't do more than 93
  page.setFreeFont(FF18);
  page.setTextColor(TFT_WHITE);
  if(SpeedF > 9.99)
  {
    page.setCursor (offsetx,  ROW2S);
  }
  else
  {
    page.setCursor (offsetx + 13, ROW2S);
  }
  page.print(SpeedF, 1);
  page.print("MPH");
}

void DisplayDelta(int offsetx, bool FullArea)
{
  dSOC = SOC - StartSOC;
  if(dSOC < 0.0)
  {
    page.setTextColor(DeltaSOCNegative); // Light red
  }
  else
  {
    page.setTextColor(DeltaSOCPlus);
  }
  page.setFreeFont(FF18);
  if(dSOC < 0.0 && dSOC > -10.0)
  {
    page.drawTriangle(offsetx + 13, ROW2S - 15, offsetx + 7, ROW2S, offsetx + 19, ROW2S, DeltaSOCNegative);
    page.setCursor(offsetx + 21, ROW2S);
    page.print(dSOC, 2);
    page.print((char) 0x25);
  }
  else if(dSOC <= -10.0 && FullArea == true)
  {
    page.drawTriangle(offsetx, ROW2S - 15, offsetx - 6, ROW2S, offsetx + 6, ROW2S, DeltaSOCNegative);
    page.setCursor(offsetx + 8, ROW2S);
    page.print(dSOC, 2);
    page.print((char) 0x25);
  }
  else if(dSOC <= -10.0 && FullArea == false)
  {
    page.drawTriangle(offsetx + 13, ROW2S - 15, offsetx + 7, ROW2S, offsetx + 19, ROW2S, DeltaSOCNegative);
    page.setCursor(offsetx + 21, ROW2S);
    page.print(dSOC, 2);
    page.print((char) 0x25);
  }
  else if(dSOC >= 0 && dSOC < 9.999)
  {
    page.drawTriangle(offsetx + 21, ROW2S - 15, offsetx + 15, ROW2S, offsetx + 27, ROW2S, DeltaSOCPlus);
    page.setCursor(offsetx + 29, ROW2S);
    page.print(dSOC, 2);
    page.print((char) 0x25);
  }
  else
  {
    page.drawTriangle(offsetx + 8, ROW2S - 15, offsetx + 2, ROW2S, offsetx + 14, ROW2S, DeltaSOCPlus);
    page.setCursor(offsetx + 16, ROW2S);
    page.print(dSOC, 2);
    page.print((char) 0x25);
  }
}

void DisplayTemperature(int offsetx)
{
  page.setFreeFont(FF18);
  if(Temperature <= TempColdLevel)
  {
    page.setTextColor(TempCold);
  }
  else if(Temperature >= TempHotLevel)
  {
    page.setTextColor(TempHot);
  }
  else if(Temperature >= TempCautionLevel)
  {
    page.setTextColor(TempCaution);
  }
  else
  {
    page.setTextColor(TempOk);
  }
  if(Temperature <= -10)
  {
    page.setCursor(offsetx, ROW2S);
    page.print(Temperature);
  }
  else if(Temperature < 0)
  {
    page.setCursor(offsetx + 13, ROW2S);
    page.print(Temperature);
  }
  else if(Temperature < 10)
  {
    page.setCursor(offsetx + 21, ROW2S);
    page.print(Temperature);
  }
  else
  {
    page.setCursor(offsetx + 8, ROW2S);
    page.print(Temperature);
  }
  if(Temperature <= TempColdLevel)
  {
    page.drawCircle(offsetx + 38, ROW2S - 15, 3, TempCold);
    page.drawCircle(offsetx + 38, ROW2S - 15, 4, TempCold);
  }
  else if(Temperature >= TempHotLevel)
  {
    page.drawCircle(offsetx + 38, ROW2S - 15, 3, TempHot);
    page.drawCircle(offsetx + 38, ROW2S - 15, 4, TempHot);
  }
  else if(Temperature >= TempCautionLevel)
  {
    page.drawCircle(offsetx + 38, ROW2S - 15, 3, TempCaution);
    page.drawCircle(offsetx + 38, ROW2S - 15, 4, TempCaution);
  }
  else
  {
    page.drawCircle(offsetx + 38, ROW2S - 15, 3, TempOk);
    page.drawCircle(offsetx + 38, ROW2S - 15, 4, TempOk);
  }
  page.setCursor(offsetx + 42, ROW2S);
  page.print("c");
}

void DisplayBadData()
{
  // Only drawing bad data in the format of SOC first line and
  //  temperature and delta SOC on second line. No mode influence.
  page.fillRect(0, 0, 160, 80, TFT_BLACK);
  page.setFreeFont(FF20);
  page.setTextColor(InvalidDataColour);

  // SOC
  page.setCursor(26, ROW1L);
  page.print("---.--");
  page.print((char) 0x25);

  // Delta
  page.setFreeFont(FF18);
  page.drawTriangle(84, ROW2S - 15, 78, 75, 90, 75, InvalidDataColour);
  page.setCursor(99, ROW2S);
  page.print("--.--");
  page.print((char) 0x25);

  // Display temperature
  page.setCursor(11, ROW2S);
  page.print("--");
  page.drawCircle(38, ROW2S - 15, 3, InvalidDataColour);
  page.drawCircle(38, ROW2S - 15, 4, InvalidDataColour);
  page.setCursor(42,75);
  page.print("c");
  
  page.pushSprite(0, 0);
}

void DisplaySOH()
{
  page.fillRect(0, 0, 160, 80, TFT_BLACK);
  page.setFreeFont(FF20);
  if(SOH >= SOHOkLevel)
  {
    page.setTextColor(SOHGood);
  }
  else if(SOH >= SOHBadLevel)
  {
    page.setTextColor(SOHOk);
  }
  else
  {
    page.setTextColor(SOHBad);
  }

  if(SOH >= 100.0)
  {
    page.setCursor(2, ROW1L);
    page.print(SOH, 1);
  }
  else if(SOH < 10.0)
  {
    // Hope no one gets here
    page.setCursor(25, ROW1L);
    page.print(SOH, 2);
  }
  else
  {
    page.setCursor(2, ROW1L);
    page.print(SOH, 2);
  }
  page.print((char) 0x25);
  
  page.setFreeFont(FF18);
  page.setCursor(55, ROW2S);
  page.print("SOH");
  
  // Not sure what the extra bit is so just draw a flag if set
  if(SOHFlag == true)
  {
    // Draw a small flag
    page.drawLine(2, 60, 2, 80, SOHFlagPole);
    page.fillTriangle(4, 61, 10, 64, 4, 67, SOHFlagColour);
  }

  // Draw the buffer to the LCD
  page.pushSprite(0, 0);
}

// ======================================================================================
// OBD Communication Routine
// ======================================================================================

bool SendOBD(char *Command, int TimeOut)
{
  long sobdt;
  uint8_t sobdc = 0;
  uint8_t sobdd;
  uint8_t sobdw = 1;
  bool sobdto = false;
#if BTMODULE == 1
  // Clear the input buffer
  while(SerialBT.available())
  {
    sobdd = SerialBT.read();
  }
  while(Command[sobdc] != NULL)
  {
    SerialBT.write(Command[sobdc]);
    sobdc++;
  }
  SerialBT.write(0x0D);
  ReceiveBuffCount = 0;
  sobdt = millis() + TimeOut;
  while(sobdw)
  {
    // If we have data, save into buffer
    if(SerialBT.available())
    {
      sobdd = SerialBT.read();
      if(sobdd == '>')
      {
        sobdw = 0;
      }
      else
      {
        ReceiveBuff[ReceiveBuffCount] = sobdd;
        ReceiveBuffCount++;
      }
    }
    // Have we timed out?
    if(sobdt < millis())
    {
      sobdw=0;
      sobdto = true;
    }
  }
#endif
  return sobdto;
}

// ======================================================================================
// Other Routines
// ======================================================================================

uint8_t Hex2Dec(uint8_t First, uint8_t Second)
{
  uint8_t H2D = 0;
  if(First >= 'A')
  {
    H2D = First - '7';
  }
  else
  {
    H2D = First - '0';
  }
  H2D = H2D << 4;
  if(Second >= 'A')
  {
    H2D += Second - '7';
  }
  else
  {
    H2D += Second - '0';
  }
  return H2D;
}
