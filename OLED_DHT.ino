//ESP-32 Core v2 needed
//DHT library Adafruit
//U8G2 library
//ESP32 Mail Client V3
//OneWire
//Partition scheme: Huge App ""

//V3.0 upgraded from v2xx to include bluetooth config
//v3.0 uses 
//v3.1 added buttons to work menu. added pump on function. added enter config mode function. added no wifi go into "manual" mode. increased pwm freq.
//v3.2 expanded on same work as 3.1
//v3.3 added wifi symbols
//v3.4 added water temp sensor 18b20 and added code to also upload water temp to server.
//v3.5 Started using gitHub to manage versions. 23 January 2024. Adding this line to test sync to github.. Added more for later deletion. Is it really real time??
//3.6 Worked on wifi-auto retry. 4 Feb 2025.

//*************************************************************************
//****Libraries included
//*************************************************************************
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
//Preferences for saving data in flash
#include <Preferences.h> //used for writing to flash permanent values
#include <WiFi.h>
#include <WiFiClient.h>
//#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//Timer library that uses millisDelay
#include <millisDelay.h>
//Used to enable http queries through wifi
#include <HTTPClient.h>
//ESP32 RTC library
#include <time.h>
//Digital humidity and temperature sensor
#include <DHT.h>
//LCD SCREEN INFO
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
#include <ESP_Mail_Client.h>//esp mail client by mobizt
#include <OneWire.h>

//*************************************************************************

U8G2_ST7565_ERC12864_ALT_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* cs=D5*/ 5, /* dc=*/ 14, /* reset=*/ 12); // contrast improved version for ERC12864// lcd pin 1:CS 2:Reset 3:DC(RS) 4:Clock 5:Data

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define ONE_WIRE_BUS 19
 
Session_Config config;
SMTPSession smtp;
//NTP client to get time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;
const int   daylightOffset_sec = 0;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"//UUID for the BLE service

//UOSystem Characteristics UUID
#define CHARACTERISTIC_UUID_name "8571a3a3-d56a-44f1-bf96-36f1ffe81930"
#define CHARACTERISTIC_UUID_ssid "442b031c-c8c5-4483-a2e8-8636f429c2a8"
#define CHARACTERISTIC_UUID_password "2faac076-cf8e-4a37-80e3-5517c48e8f5c"
#define CHARACTERISTIC_UUID_wifistr "f9f2a5c0-40fb-42ea-820a-b1128ad00841"
#define CHARACTERISTIC_UUID_mac "161e189d-6cb7-483d-8495-8129b08e43e1"
#define CHARACTERISTIC_UUID_authormail "c0a35b27-e52d-40ee-a189-59af1c07a328"
#define CHARACTERISTIC_UUID_authorpassword "563b8506-492f-48dc-a6ca-cc4464e2d770"
#define CHARACTERISTIC_UUID_recipientmail "7cba6ae5-5daa-415d-9abd-b1a520775f11"
#define CHARACTERISTIC_UUID_pumpTime "c60d4d1a-1a7f-4377-981e-f57dcc168e94"
#define CHARACTERISTIC_UUID_pumpOnTime "c3456aba-6c5f-4916-afeb-9c9e1858b1fd"
#define CHARACTERISTIC_UUID_pumpPower "09d5ae68-9426-46e1-b670-a4930d3111a0"
#define CHARACTERISTIC_UUID_timeLEDON "50f7a1d9-cc49-42ae-90b7-0e1ffa7f4d0d"
#define CHARACTERISTIC_UUID_timeLEDOFF "72887292-2a06-4ffd-af53-d4e8425b4586"
#define CHARACTERISTIC_UUID_lED1Bright "aa948ae7-5dd0-4df3-b4a7-617b143c7379"
#define CHARACTERISTIC_UUID_lED2Bright "22eaee07-56ea-4db7-ad22-02c61a5091a2"
#define CHARACTERISTIC_UUID_lED3Bright "8d9ae9bc-2de3-42ab-b174-81baf53c98b8"
#define CHARACTERISTIC_UUID_temp "425ae845-abaa-4790-9ad8-bf16033907ff"
#define CHARACTERISTIC_UUID_hum "8ded8c83-a959-401e-9623-58a733425ff1"
#define CHARACTERISTIC_UUID_pumpCount "2703ab3f-b0a5-4b08-a228-664f4439d540"
#define CHARACTERISTIC_UUID_flowCount "a2153f5e-8b9b-43ce-ab3f-08ccfbc5a998"
#define CHARACTERISTIC_UUID_floatSense "91b7f7f7-5f15-4e73-bb9b-8616d3289b0d"


BLECharacteristic *pCharacteristic_name;
BLECharacteristic *pCharacteristic_ssid;
BLECharacteristic *pCharacteristic_password;
BLECharacteristic *pCharacteristic_wifistr;
BLECharacteristic *pCharacteristic_mac;
BLECharacteristic *pCharacteristic_authormail;
BLECharacteristic *pCharacteristic_authorpassword;
BLECharacteristic *pCharacteristic_recipientmail;
BLECharacteristic *pCharacteristic_pumpTime;
BLECharacteristic *pCharacteristic_pumpOnTime;
BLECharacteristic *pCharacteristic_pumpPower;
BLECharacteristic *pCharacteristic_timeLEDON;
BLECharacteristic *pCharacteristic_timeLEDOFF;
BLECharacteristic *pCharacteristic_lED1Bright;
BLECharacteristic *pCharacteristic_lED2Bright;
BLECharacteristic *pCharacteristic_lED3Bright;
BLECharacteristic *pCharacteristic_temp;
BLECharacteristic *pCharacteristic_hum;
BLECharacteristic *pCharacteristic_pumpCount;
BLECharacteristic *pCharacteristic_flowCount;
BLECharacteristic *pCharacteristic_floatSense;


BLEServer *pServer;
BLEService *pService;

//bool variables
bool deviceConnected = false;
bool led;
//const char definitions
const char *myDebug[] = {"Program started","Pin definitions done"};

//const int definitions
//pins
const int LED_BUILTIN = 2;//built in led is on pin 2.
const int PUMP_OUT = 21;
//const int TDS_IN = 32;//TDS analogue in
const int FLOW_SENSOR = 16;
const int LEVEL_SENSOR = 4;
const int PB_ADC_IN = 33;
const int VIN_MON = 36;
const int LED_OUT1 = 22;
//const int LED_OUT2 = 1;
const int LED_LCD = 17;

const int freq = 20000;
const int LED_LCD_CH = 0;
const int LED_OUT1_CH = 1;
const int PUMP_OUT_CH = 2;
const int resolution = 8;

//millisDelay timers
millisDelay timer;
millisDelay timerBlink;//used as a 1 second timer that is used for updating the CPU running led and lcd update.
millisDelay timerPump;//used to time the turn on of the pump as defined.
millisDelay timerPumpON;//used to time the time the pump is ON once switched on
millisDelay timerFlowRate;
millisDelay timerFlowRateTimeOut;
millisDelay espRestart;//used to monitor startup and restart the controller if the wifi is hanging.
millisDelay bleTime;
millisDelay timerMenu;


//DHT instance for the sensor
DHT dht(15,DHT22);//DHT11 or DHT22. Preferably DHT22 if available.
//Bool definitions
bool boolBlink;
bool boolLevel;
bool boolLevelWarningSent;//used to send only one Whatsapp for level warning.
bool boolFlowSenseTimeOut;//Used to timeout the flow sense and return the value to 0.
bool boolTimeOnNeg;//used if the time ON for LED is later than the turn off. example: Turn ON 18:00 and turn off06:00
bool boolFlowCheck;
bool boolSendOnMsg;//used to send message when led is turned on
bool boolSendOffMsg;//used to send message when led is turned off
bool boolFlowPulse;//Used to detect change of state in flow sensor
bool boolLights;
bool wifiCon;//wifi connected used to enable wifi functions only when wifi is connected.
bool otaOn;//only update ota when the ota service has started
bool bleModeON;
bool pumpManual;
//unsigned in definitions
unsigned int pumpCounter;
//Int definitions
int systemState = 0;
int menuIndex=0;
int pulseCounts = 0;//used to read the flow rate from the flow sensor
int PB_In = 0; //used to adc read the push button input
int VIN =0;
int wIFI_Retry = 0;//used to count for delay to re-try wifi connection.
//float definitions
float flowRate = 0;//Used to calculate the l/min flow rate of the pump
float avgFlowRate = 0;//Used to calculate the amount of water pumped during the on cycle.
float atFlow = 0;//all time flow is saved in flash
float tOn=6;//used for the logic check if lights are to be switched on or off
float tOff=18;//used for the logic check if lights are to be switched on or off
float water_celsius =0;//used to stro 18b20 sensor reading, water temperature
//Preferences instance init
Preferences preferences;//Instance of the preferences library to store data in flash.

//Bluetooth Variables
String name;// R/W
String ssid;// R/W
String password;// R/W
int wifistr;//R
String mac;//R
String authormail;// R/W
String authorpassword;// R/W
String recipientmail;// R/W
//long pumpTime;// R/W
//long pumpOnTime;// R/W
int pumpTime;// R/W
int pumpOnTime;// R/W
int pumpPower;// R/W
String timeLEDON;// R/W
String timeLEDOFF;// R/W
String status = "Good";//R used for total system status, errors
int lED1Bright;// R/W
int lED2Bright;// R/W
int lED3Bright;// R/W
float temp;//R
float hum;//R
int pumpCount;//R
float flowCount;//R
int floatSense;//R

//byte
//bytes for one wire temp sensor 18b20
byte i_18b20;
byte present = 0;
byte type_s;
byte data[12];
byte addr[8];
OneWire oneWire(ONE_WIRE_BUS);

struct tm timeinfo;//time info retrieved from ntp client
/* Struct tm  (info for when time on and off shall be configurable)
int8_t 	tm_sec
int8_t 	tm_min
int8_t 	tm_hour
int8_t 	tm_mday
int8_t 	tm_wday
int8_t 	tm_mon
int16_t 	tm_year
 	Years since 1900. More...
int16_t 	tm_yday 
int16_t 	tm_isdst
 */
const struct tm timeLEDONMan = {0,0,6,1,1,1,1,1,0};//time the led outputs turn on 6:00AM
const struct tm timeLEDOFFMan = {0,0,18,1,1,1,1,1,0};//time the led outputs turn off 6:00PM

#define pumpWidth  16
#define pumpHeight 16
static unsigned char pump1Bmp[] = {
 0x00,0x00,0x00,0xc0,0xc0,0xff,0x20,0x80,0x10,0x80,0x08,0xe0,
 0x88,0xd7,0xe8,0x11,0x08,0x10,0x10,0x08,0x20,0x04,0xc0,0x03,
 0xf0,0x0f,0x08,0x10,0x04,0x20,0xfc,0x3f};
static unsigned char pump2Bmp[] = {
 0x00,0x00,0x00,0xc0,0xc0,0xff,0x20,0x80,0x90,0x80,0x88,0xe0,
 0x88,0xd1,0x88,0x11,0x08,0x11,0x10,0x09,0x20,0x04,0xc0,0x03,
 0xf0,0x0f,0x08,0x10,0x04,0x20,0xfc,0x3f};
 //Logo icon for lcd XBM
#define logoWidth  32
#define logoHeight 32
static unsigned char logoBmp[] = {
 0x00,0x00,0x00,0x07,0x00,0x00,0xe0,0x05,0x00,0x00,0x30,0x05,
 0x80,0x07,0x18,0x05,0xc0,0x3c,0x88,0x06,0x80,0x69,0x6c,0x02,
 0x00,0x53,0x14,0x03,0x00,0xce,0x87,0x01,0x00,0xf8,0xe3,0x00,
 0x00,0xc0,0x3f,0x00,0x04,0x00,0x03,0x20,0xf4,0xff,0xff,0x2f,
 0xe8,0xff,0xff,0x27,0x18,0x00,0x03,0x30,0x10,0x80,0x03,0x10,
 0x30,0x70,0x03,0x10,0x20,0x08,0x0f,0x18,0x40,0xc8,0x13,0x08,
 0xc0,0x60,0x33,0x0c,0x00,0x31,0x23,0x06,0x00,0x13,0x0f,0x02,
 0x00,0x06,0x0a,0x03,0x00,0x0c,0x88,0x01,0x00,0x18,0xc0,0x00,
 0x00,0x30,0x60,0x00,0x00,0xc0,0x1f,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

#define Wifi_Weak_width  8
#define Wifi_Weak_height 8
static unsigned char Wifi_Weak_bits[] = {
 0x00,0x00,0x00,0x00,0x00,0x18,0x3c,0x18};

 #define Wifi_Discon_width  8
#define Wifi_Discon_height 8
static unsigned char Wifi_Discon_bits[] = {
 0x81,0x42,0x24,0x18,0x18,0x24,0x5a,0x99};

 #define Wifi_OK_width  8
#define Wifi_OK_height 8
static unsigned char Wifi_OK_bits[] = {
 0x00,0x00,0x18,0x24,0x00,0x18,0x3c,0x18};

#define Wifi_Good_width  8
#define Wifi_Good_height 8
static unsigned char Wifi_Good_bits[] = {
 0x3c,0x42,0x99,0x24,0x00,0x18,0x3c,0x18};


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      uint8_t* value2 = pCharacteristic->getData();
     
      if (pCharacteristic->getUUID().toString()=="8571a3a3-d56a-44f1-bf96-36f1ffe81930")
      {
        name = preferences.putString("name",value.c_str());
        
        
      }
      if (pCharacteristic->getUUID().toString()=="442b031c-c8c5-4483-a2e8-8636f429c2a8")
      {
        
        ssid = preferences.putString("ssid",value.c_str());
        
      }
      if (pCharacteristic->getUUID().toString()=="2faac076-cf8e-4a37-80e3-5517c48e8f5c")
      {
        
        password = preferences.putString("password",value.c_str());
        
      }
      if (pCharacteristic->getUUID().toString()=="c0a35b27-e52d-40ee-a189-59af1c07a328")
      {
        
        authormail = preferences.putString("authormail",value.c_str());
        
      }
      if (pCharacteristic->getUUID().toString()=="563b8506-492f-48dc-a6ca-cc4464e2d770")
      {
        
        authorpassword = preferences.putString("authorpassword",value.c_str());
        
      }
      if (pCharacteristic->getUUID().toString()=="7cba6ae5-5daa-415d-9abd-b1a520775f11")
      {
        
        recipientmail = preferences.putString("recipientmail",value.c_str());
        
      }
      if (pCharacteristic->getUUID().toString()=="c60d4d1a-1a7f-4377-981e-f57dcc168e94")
      {
        int val=0;
        for (int i=0;i<sizeof(value2);i++)
        {
          val=val+(value2[i]*pow(256,i));
          Serial.println(value2[i]);
        }
        
        pumpTime = preferences.putInt("pumpTime",val);//convert std::String to int for pumpTime value
        
        
      }
      if (pCharacteristic->getUUID().toString()=="c3456aba-6c5f-4916-afeb-9c9e1858b1fd")
      {
        
        int val=0;
        for (int i=0;i<sizeof(value2);i++)
        {
          val=val+(value2[i]*pow(256,i));
          Serial.println(value2[i]);
        }
        pumpOnTime = preferences.putInt("pumpOnTime",val);
        
      }
      if (pCharacteristic->getUUID().toString()=="09d5ae68-9426-46e1-b670-a4930d3111a0")
      {
        
        int val=0;
        for (int i=0;i<sizeof(value2);i++)
        {
          val=val+(value2[i]*pow(256,i));
          Serial.println(value2[i]);
        }
        pumpPower = preferences.putInt("pumpPower",val);
        
      }
      if (pCharacteristic->getUUID().toString()=="50f7a1d9-cc49-42ae-90b7-0e1ffa7f4d0d")
      {
        
        timeLEDON = preferences.putString("timeLEDON",value.c_str());
        
      }
      if (pCharacteristic->getUUID().toString()=="72887292-2a06-4ffd-af53-d4e8425b4586")
      {
        
        timeLEDOFF = preferences.putString("timeLEDOFF",value.c_str());
        
      }
      if (pCharacteristic->getUUID().toString()=="aa948ae7-5dd0-4df3-b4a7-617b143c7379")
      {
        
        int val=0;
        for (int i=0;i<sizeof(value2);i++)
        {
          val=val+(value2[i]*pow(256,i));
          Serial.println(value2[i]);
        }
        lED1Bright = preferences.putInt("lED1Bright",val);
        
      }
      if (pCharacteristic->getUUID().toString()=="22eaee07-56ea-4db7-ad22-02c61a5091a2")
      {
        int val=0;
        for (int i=0;i<sizeof(value2);i++)
        {
          val=val+(value2[i]*pow(256,i));
          Serial.println(value2[i]);
        }
        lED2Bright = preferences.putInt("lED2Bright",val);
        
      }
      if (pCharacteristic->getUUID().toString()=="8d9ae9bc-2de3-42ab-b174-81baf53c98b8")
      {
        
        int val=0;
        for (int i=0;i<sizeof(value2);i++)
        {
          val=val+(value2[i]*pow(256,i));
          Serial.println(value2[i]);
        }
        lED3Bright = preferences.putInt("lED3Bright",val);
        
      }
    }
    
};
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      pServer->startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      pServer->startAdvertising();
    }
};

void setup() {
  //Serial port start and first message sent
  Serial.begin(9600);
  Serial.setTimeout(20);//20ms timeout
  Serial.println(myDebug[0]);//"Program Started"
  //pin mode definitions
  //ledcAttach(LED_LCD,freq, resolution);
  //ledcAttach(LED_OUT1,freq, resolution);
  //ledcAttach(PUMP_OUT,freq, resolution);
  ledcSetup(LED_LCD_CH, freq, resolution);
  ledcSetup(LED_OUT1_CH, freq, resolution);
  ledcSetup(PUMP_OUT_CH, freq, resolution);
  ledcAttachPin(LED_LCD, LED_LCD_CH);
  ledcAttachPin(LED_OUT1, LED_OUT1_CH);
  
  ledcAttachPin(PUMP_OUT, PUMP_OUT_CH);
  for (int i=0;i<255;i++)
  {
    //ledcWrite(LED_LCD_CH,i);
    ledcWrite(LED_LCD_CH,i);
    //ledcWrite(LED_OUT1_CH,i);
    delay(1);
  }
  for (int i=0;i<3;i++)
  {
    //ledcWrite(LED_OUT1_CH,255);
    ledcWrite(LED_OUT1_CH,255);
    delay(500);
    ledcWrite(LED_OUT1,0);
    delay(500);
  }
  ledcWrite(LED_OUT1_CH,255);
  //Digital io
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(FLOW_SENSOR,INPUT);
  pinMode(LEVEL_SENSOR,INPUT_PULLUP);
  
  
  ledcWrite(LED_OUT1_CH,0);
  ledcWrite(LED_LCD_CH,128);//LCD Brightness
  ledcWrite(PUMP_OUT_CH,0);//Pump off
  
  u8g2.begin();
  u8g2.setContrast(75);
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.clearBuffer();
  u8g2.setCursor(0, 10);
  u8g2.print("Initializing...");
  u8g2.sendBuffer();
  
  updateLCD();
  //BLE setup code
  BLEDevice::init("Urban Oasis Hydroponics");
  pServer = BLEDevice::createServer();
  //pService = pServer->createService(SERVICE_UUID);
  pService = pServer->createService(BLEUUID(SERVICE_UUID), 60, 0);
  
  pCharacteristic_name = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_name,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );                                     
  pCharacteristic_ssid = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_ssid,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_password = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_password,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_authormail = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_authormail,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_authorpassword = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_authorpassword,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_recipientmail = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_recipientmail,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_pumpTime = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_pumpTime,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_pumpOnTime = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_pumpOnTime,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_pumpPower = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_pumpPower,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_timeLEDON = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_timeLEDON,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );  
  pCharacteristic_timeLEDOFF = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_timeLEDOFF,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_lED1Bright = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_lED1Bright,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_lED2Bright = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_lED2Bright,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_lED3Bright = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_lED3Bright,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic_wifistr = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_wifistr,
                                         BLECharacteristic::PROPERTY_READ                                        
                                       );
  pCharacteristic_mac = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_mac,
                                         BLECharacteristic::PROPERTY_READ                                         
                                       );                                                                                                                        
  pCharacteristic_temp = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_temp,
                                         BLECharacteristic::PROPERTY_READ                                         
                                       );
  pCharacteristic_hum = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_hum,
                                         BLECharacteristic::PROPERTY_READ                                         
                                       );
  pCharacteristic_pumpCount = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_pumpCount,
                                         BLECharacteristic::PROPERTY_READ                                         
                                       );
  pCharacteristic_flowCount = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_flowCount,
                                         BLECharacteristic::PROPERTY_READ                                         
                                       );
  pCharacteristic_floatSense = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_floatSense,
                                         BLECharacteristic::PROPERTY_READ                                         
                                       );                                                                                                                                                                                                      
  //connect characteristics to call backs that handle the data update when the value is change from the client side 
  pCharacteristic_name->setCallbacks(new MyCallbacks());
  pCharacteristic_ssid->setCallbacks(new MyCallbacks());
  pCharacteristic_password->setCallbacks(new MyCallbacks());
  pCharacteristic_authormail->setCallbacks(new MyCallbacks());
  pCharacteristic_authorpassword->setCallbacks(new MyCallbacks());
  pCharacteristic_recipientmail->setCallbacks(new MyCallbacks());
  pCharacteristic_pumpTime->setCallbacks(new MyCallbacks());
  pCharacteristic_pumpOnTime->setCallbacks(new MyCallbacks());
  pCharacteristic_pumpPower->setCallbacks(new MyCallbacks());
  pCharacteristic_timeLEDON->setCallbacks(new MyCallbacks());
  pCharacteristic_timeLEDOFF->setCallbacks(new MyCallbacks());
  pCharacteristic_lED1Bright->setCallbacks(new MyCallbacks());
  pCharacteristic_lED2Bright->setCallbacks(new MyCallbacks());
  pCharacteristic_lED3Bright->setCallbacks(new MyCallbacks());
  pCharacteristic_wifistr->setCallbacks(new MyCallbacks());
  
  
  
  //
  
  //Preferences written in flash
  preferences.begin("credentials", false);//credentials for the wifi username and password.
  name = preferences.getString("name","UOH System");
  ssid = preferences.getString("ssid","HAdmin");
  password = preferences.getString("password","HAdmin");
  wifistr=0;
  mac = ESP.getEfuseMac();
  authormail=preferences.getString("authormail","admin@admin.com");
  authorpassword=preferences.getString("authorpassword","admin");
  recipientmail = preferences.getString("recipientmail","client@admin.com");
  pumpTime = preferences.getInt("pumpTime",60000);
  pumpOnTime = preferences.getInt("pumpOnTime",10000);
  pumpPower = preferences.getInt("pumpPower",128);
  timeLEDON = preferences.getString("timeLEDON","06:00");
  timeLEDOFF = preferences.getString("timeLEDOFF","18:00");
  lED1Bright = preferences.getInt("lED1Bright",50);
  lED2Bright = preferences.getInt("lED2Bright",50);
  lED3Bright = preferences.getInt("lED3Bright",50);
  temp = 0;
  hum = 0;
  pumpCount = preferences.getInt("pumpCounter",0);
  flowCount = preferences.getFloat("atFlow",0.0);
  floatSense = 0;
  //
  //Set values for characteristics from read flash values  
  
  pCharacteristic_name->setValue(name.c_str());
  pCharacteristic_ssid->setValue(ssid.c_str());
  pCharacteristic_password->setValue(password.c_str());
  pCharacteristic_wifistr->setValue(wifistr);
  pCharacteristic_mac->setValue(mac.c_str());
  pCharacteristic_authormail->setValue(authormail.c_str());
  pCharacteristic_authorpassword->setValue(authorpassword.c_str());
  pCharacteristic_recipientmail->setValue(recipientmail.c_str());
  pCharacteristic_pumpTime->setValue(pumpTime);
  pCharacteristic_pumpOnTime->setValue(pumpOnTime);
  pCharacteristic_pumpPower->setValue(pumpPower);
  pCharacteristic_timeLEDON->setValue(timeLEDON.c_str());
  pCharacteristic_timeLEDOFF->setValue(timeLEDOFF.c_str());
  pCharacteristic_lED1Bright->setValue(lED1Bright);
  pCharacteristic_lED2Bright->setValue(lED2Bright);
  pCharacteristic_lED3Bright->setValue(lED3Bright);
  pCharacteristic_temp->setValue(temp);
  pCharacteristic_hum->setValue(hum);
  pCharacteristic_temp->setValue(temp);
  pCharacteristic_pumpCount->setValue(pumpCount);
  pCharacteristic_flowCount->setValue(flowCount);
  pCharacteristic_floatSense->setValue(floatSense);
  //
  //Wifi Settings
  Serial.println(ssid);
  Serial.println(password);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  delay(1000);
  int connCount = 0;
  while ((WiFi.status() != WL_CONNECTED)&&connCount <201) {
    wifiCon=true;
    if (connCount == 200)
    {
      
      WiFi.disconnect();
      //ESP.restart();
      wifiCon=false;
      break;
    }
    
    delay(100);
    connCount++;
    
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    int restartCounter;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      while (!printLocalTime())
      {
        restartCounter++;
        delay(500);
        Serial.print(".");
        if (restartCounter == 60)
        {
          ESP.restart();
        }
      }
  }
  digitalWrite(2,HIGH);
  //
  //Arduino over the air setup
  
  
  
  
  //
  //Display all value from flash for preferences/Config
  dht.begin();
  read_Temp_18B20();//read temp from water sensor
  temp = dht.readTemperature();//reads the temperature from the DHT22
  hum = dht.readHumidity();//reads the humidity from the DHT22
  timerBlink.start(750);
  //bleTime.start(45000);//time the bluetooth config stays active removed for v3_1
  systemState=1;
  //*************************************************************************
  //****OTA setup
  //*************************************************************************
  ArduinoOTA
    .onStart([]() {
      BLEDevice::stopAdvertising();
      BLEDevice::deinit();
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  
    })
    .onEnd([]() {
  
    })
    .onProgress([](unsigned int progress, unsigned int total) {
  
    })
    .onError([](ota_error_t error) {
  
   
    });
  ArduinoOTA.begin();
  otaOn=true;
  //*************************************************************************

  if (wifiCon)
  {
    httpUpdate();
  }
  read_Temp_18B20();//read temp from water sensor
}

void loop() {
  if (otaOn)
  {
    ArduinoOTA.handle();
  }
  if (pumpManual)
  {
    ledcWrite(PUMP_OUT_CH,pumpPower); 
    timerPump.stop();
    timerPumpON.stop();
  }
  //wifistr=WiFi.RSSI();
  if (boolFlowPulse!=digitalRead(FLOW_SENSOR))
  {
    pulseCounts++;
    boolFlowPulse=digitalRead(FLOW_SENSOR);
  }
  
  if (!timerFlowRate.isRunning()&&systemState==2)
  {
    timerFlowRate.start(1000);
  }
  if (timerFlowRate.justFinished())
  {
    flowRate = pulseCounts/7.5;//98 for yf-s401 7.5 for yf-s201
    avgFlowRate+=flowRate;
    pulseCounts=0;
    if (flowRate>0.25&&boolFlowCheck)
    {
      boolFlowCheck=false;
    }
  }
  if(!boolLevel)
  {
    boolLevelWarningSent=false;
  }
  if (systemState == 1 && !pumpManual)
  {
    ledcWrite(PUMP_OUT_CH,0);//PUMP OFF
    if (!timerPump.isRunning())
    {
      timerPump.start(pumpTime);
    }
  }
  if (timerPump.justFinished()&&!pumpManual)
  {
    boolLevel=digitalRead(LEVEL_SENSOR);//read water level just before pump starts
	floatSense = boolLevel;
    if (boolLevel&&(!boolLevelWarningSent)&&!systemState==2&&wifiCon)//only show errors on level when not pumping.//move to just before pump activates.
    {
      SendMail(4);
      boolLevelWarningSent=true;    
    }
    //ledcWrite(PUMP_OUT_CH,165);//Pump 65%
    for (int i=0;i<pumpPower;i++)
    {
      ledcWrite(PUMP_OUT_CH,i);
      delay(10);
    }
    pumpCount++;
    preferences.putInt("pumpCounter",pumpCount);
    systemState=2;
    avgFlowRate=0.0;
    boolFlowCheck=true;//set flag to check if flow is active
    timerPumpON.start(pumpOnTime);
  }
  if (timerPumpON.justFinished())
  {
    systemState=1;
    flowCount+=avgFlowRate/60.0;
    preferences.putFloat("atFlow",flowCount);
    if (boolFlowCheck&&wifiCon)
    {
      //whatsappint+=4;
      SendMail(3);
      boolFlowCheck=false;
    }
    timerPump.start(pumpTime);
  }
  //server.handleClient();
  temp = dht.readTemperature();//reads the temperature from the DHT22
  hum = dht.readHumidity();//reads the humidity from the DHT22
  if (timerBlink.justFinished())
  {
    blink();    
    timerBlink.restart();//restart the timer after it has lapsed.
  }
  if (timerFlowRateTimeOut.justFinished())
  {
    flowRate=0;
  }
  //1L = 5880 pulses for YFS401 Sensor
}
void bleON()
{
  systemState = 3;
  bleModeON=true;
  timerPump.stop();
  timerPumpON.stop();
  pService->start();
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
}

void buttonHandle(int PB)
{
  if (PB >4000)//left button button 1
  {
    if (bleModeON)
    {
      ESP.restart(); 
    }
    else
    {
      if (!timerMenu.isRunning())
      {
        systemState=4;
        timerMenu.start(15000);
        menuIndex=1;
      }
      else
      {
        timerMenu.start(15000);
        if (menuIndex<3)
        {
          menuIndex++;
        }
        else
        {
          menuIndex=1;
        }
      }
      
    }
  }
  else if (PB > 800)//button4
  {
    if (pumpManual)
    {
      pumpManual=false;
      systemState=1;
      ledcWrite(PUMP_OUT_CH,0); 
      timerPump.restart();
      timerPumpON.restart();
    }
    if (timerMenu.isRunning())
    {
      if (menuIndex==2)
      {
        bleON();
      }
      if (menuIndex==1)
      {
        pumpManual=true;
      }
    }
  }
  
}

//Triggers once a second due to timer lapsing 
void blink()	
{
  read_Temp_18B20();//read temp from water sensor
  PB_In = analogRead(PB_ADC_IN);
  buttonHandle(PB_In);
  if (timerMenu.justFinished()&&!bleModeON&&!pumpManual)//returns to normal display 
  {
    systemState = 1;
  }
  
  VIN = analogRead(VIN_MON);
  if (PB_In > 3000)
  {
    //ESP.restart();
  }
  //Update time and date info  
  if (wifiCon)
  {
    getLocalTime(&timeinfo);
  }
  else
	  //Added the else code to auto retry wifi connect.
  {
	  if (wIFI_Retry < 60)
	  {
		wIFI_Retry++;
	  }
	  else if (ssid!="HAdmin")
	  {
		Serial.println("Wifi Auto Retry");
		wIFI_Retry = 0;
		WiFi.disconnect();
		WiFi.begin(ssid, password);
		int connCount = 0;
		while ((WiFi.status() != WL_CONNECTED)&&connCount <201) 
		{
			wifiCon=true;
			if (connCount == 200)
			{
			  
			  WiFi.disconnect();
			  //ESP.restart();
			  wifiCon=false;
			  break;
        Serial.println("Retry Failed");
			}
			
			delay(100);
			connCount++;
			
		}
		if (WiFi.status() == WL_CONNECTED)
		{
      Serial.println("Retry Time Request");
			int restartCounter;
			configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
			  while (!printLocalTime())
			  {
				restartCounter++;
				delay(500);
				Serial.print(".");
				if (restartCounter == 60)
				{
				  ESP.restart();
				}
			  }
		}
	  }
		
  }
  
  //Run the routine to update the info on the lcd
  updateLCD();
  //Run the check is the LEDs needs to turn on or off.
  float tCur =0;
  if (wifiCon)
  {
    tCur = timeinfo.tm_hour+(timeinfo.tm_min/60.0);
  }
  if (!boolTimeOnNeg)//if tON is smaller than tOff
  {
    if (tCur>tOn && tCur<tOff)  
    {
      ledcWrite(LED_OUT1_CH,255);
      if (boolSendOnMsg)
      {
        boolLights=true;
        boolSendOnMsg=false;
        boolSendOffMsg=true;
      }
    }
    else
    {
      ledcWrite(LED_OUT1_CH,0);
      
      if (boolSendOffMsg)
      {
        boolLights=false;
        boolSendOffMsg=false;
        boolSendOnMsg=true;
      }
    }
  }
  else//if tON is larger than tOff
  {
    if (tCur>tOn || tCur<tOff)  
    {
      ledcWrite(LED_OUT1_CH,255);
      if (boolSendOnMsg)
      {
        boolLights=true;
        boolSendOnMsg=false;
        boolSendOffMsg=true;
      }
    }
    else
    {
      ledcWrite(LED_OUT1_CH,0);
      if (boolSendOffMsg)
      {
        boolLights=false;
        boolSendOffMsg=false;
        boolSendOnMsg=true;
      }
    }
  }
  if (boolBlink)//Used to blink the on board led for CPU running.
  {
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    boolBlink = false;
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    boolBlink = true;
  }
  if (timeinfo.tm_min==30&&timeinfo.tm_sec==0&&wifiCon)
  {
    //whatsappint+=8;
    SendMail(2);//Send the status update mail for Counter and Temp/Humid
  }
  if (timeinfo.tm_sec==0&&wifiCon)//update when sec==0 means every minute :)
  {
    httpUpdate();
    Serial.print("Update at: ");
    if (timeinfo.tm_hour>9)
    {
      Serial.print(timeinfo.tm_hour);  
    }
    else
    {
      Serial.print("0");
      Serial.print(timeinfo.tm_hour);  
    }
    Serial.print(":");  
    if (timeinfo.tm_min>9)
    {
      Serial.print(timeinfo.tm_min);  
    }
    else
    {
      Serial.print("0");
      Serial.print(timeinfo.tm_min);  
    }
    Serial.print(":");  
    if (timeinfo.tm_sec>9)
    {
      Serial.println(timeinfo.tm_sec);  
    }
    else
    {
      Serial2.print("0");
      Serial.println(timeinfo.tm_sec);  
    }
    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity: ");
    Serial.println(hum);
    Serial.print("Number of pump turn on's: ");
    Serial.println(pumpCount);
    Serial.print("Number of litres pumped: ");
    Serial.println(flowCount);
    
  }
  if (bleTime.justFinished())
  {
    systemState=0;
    updateLCD();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println(ssid);
    Serial.println(password);
    int restartCounter = 0;
    while (WiFi.status() != WL_CONNECTED) 
    {
      restartCounter++;
      delay(500);
      Serial.print(".");
      if (restartCounter == 120)
      {
        ESP.restart();
      }
    }
    restartCounter=0;
    Serial.println("Time");
    Serial.println(WiFi.RSSI());
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    while (!printLocalTime())
    {
      restartCounter++;
      delay(500);
      Serial.print(".");
      if (restartCounter == 60)
      {
        ESP.restart();
      }
    }
    BLEDevice::stopAdvertising();
    BLEDevice::deinit();
    wifiCon=true;
    systemState=1;  
    ArduinoOTA
    .onStart([]() {
      BLEDevice::stopAdvertising();
      BLEDevice::deinit();
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  
    })
    .onEnd([]() {
  
    })
    .onProgress([](unsigned int progress, unsigned int total) {
  
    })
    .onError([](ota_error_t error) {
  
   
    });

  ArduinoOTA.begin();
  otaOn=true;
  }
  

  
  
  
  
}

void updateLCD()
{
  //0 Wifi connect with logo
  //1 Countdown to pump on
  //2 Pump on and count down to pump stop.
  //3 BLE Config
  //4 Menu
//Start by clearing the lcd buffer to write new info to screen without old artifacts  

  u8g2.clearBuffer();	
  if (systemState ==0)
  {
    u8g2.setFont(u8g2_font_squeezed_b7_tr);	// choose a suitable font u8g2_font_squeezed_b7_tr does fit
    u8g2.setCursor(0, 10);
    u8g2.print("Hydro Gallery");
    
    u8g2.drawXBM(0, 16, logoWidth, logoHeight, logoBmp);
    u8g2.drawStr(49, 25, "Fresh produce");
    u8g2.drawStr(64, 35, "grown at");
    u8g2.drawStr(68, 45, "home");
    u8g2.setCursor(0, 60);
    u8g2.print("Waiting for WiFi...");
    u8g2.sendBuffer();
    return;
  }
  if (systemState ==3)
  {
    u8g2.setFont(u8g2_font_squeezed_b7_tr);	// choose a suitable font
    u8g2.setCursor(0, 10);
    u8g2.print("Hydro Gallery");
    
    u8g2.drawXBM(0, 16, logoWidth, logoHeight, logoBmp);
    u8g2.drawStr(49, 25, "Fresh produce");
    u8g2.drawStr(64, 35, "grown at");
    u8g2.drawStr(68, 45, "home");
    u8g2.setCursor(0, 50);
    u8g2.print("Bluetooth Configuration");
    u8g2.setCursor(0, 60);
    u8g2.print("Press Menu Button to Restart");
    u8g2.sendBuffer();
    return;
  }
  if (systemState ==1)
  {
    u8g2.setCursor(0,10);
    if (wifiCon)
    {
      int sig = WiFi.RSSI();
      if (sig>-55)
      {
        //u8g2.print("Excel");  
        u8g2.drawXBM(0, 0, Wifi_Good_width, Wifi_Good_height, Wifi_Good_bits);
      }
      else if (sig>-60)
      {
        //u8g2.print("Good");  
        u8g2.drawXBM(0, 0, Wifi_Good_width, Wifi_Good_height, Wifi_Good_bits);
      }
      else if (sig>-67)
      {
        u8g2.drawXBM(0, 0, Wifi_OK_width, Wifi_OK_height, Wifi_OK_bits);
      }
      else if (sig>-80)
      {
        //u8g2.print("Weak");  
        u8g2.drawXBM(0, 0, Wifi_Weak_width, Wifi_Weak_height, Wifi_Weak_bits);
      }
      else
      {
        //u8g2.print("Discon");              
        u8g2.drawXBM(0, 0, Wifi_Discon_width, Wifi_Discon_height, Wifi_Discon_bits);
      }
      
    }
    else
    {
      u8g2.print("No WiFi");
    }
    u8g2.setCursor(32,10);//Move the cursor after the wifi logos
    u8g2.print(" ");
    //u8g2.print(PB_In);
    u8g2.print(" ");
    float volt = VIN*0.0045922852;
    u8g2.print(volt);
    u8g2.print(" V");
    
    //Print time to lcd
    u8g2.setCursor(81,10);//Move the time the top right of the lcd screen.
    if (timeinfo.tm_hour>9)
    {
      u8g2.print(timeinfo.tm_hour);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(timeinfo.tm_hour);  
    }
    u8g2.print(":");  
    if (timeinfo.tm_min>9)
    {
      u8g2.print(timeinfo.tm_min);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(timeinfo.tm_min);  
    }
    u8g2.print(":");  
    if (timeinfo.tm_sec>9)
    {
      u8g2.print(timeinfo.tm_sec);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(timeinfo.tm_sec);  
    }  
    //******************************//
    //Line 1
    u8g2.setCursor(0,10);
    //Line 2
    u8g2.setCursor(0,20);
    
    //Line 3
    u8g2.setCursor(0,30);
    u8g2.print(temp,1);
    u8g2.print(" C");  
    u8g2.print(" | ");      
    u8g2.print(hum,1);
    u8g2.print(" %");    
    u8g2.print(" | ");
    u8g2.print("H2O: ");
    u8g2.print(water_celsius,1);
    u8g2.print(" C");  
    //Line 4
    u8g2.setCursor(0,40);
    u8g2.print("Pump timer: ");
    unsigned long timeLeft = timerPump.remaining();
    int pumpH = timeLeft / 3600000;
    int pumpM = ((timeLeft-pumpH*3600000) / 60000);
    int pumpS = ((timeLeft-pumpM*60000) / 1000);
    if (pumpH>9)
    {
      u8g2.print(pumpH);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(pumpH);  
    }
    u8g2.print(":");  
    if (pumpM>9)
    {
      u8g2.print(pumpM);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(pumpM);  
    }
    u8g2.print(":");  
    if (pumpS>9)
    {
      u8g2.print(pumpS);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(pumpS);  
    }  
    
    //Line 5
    u8g2.setCursor(0,50);
    u8g2.print("Level Sensor: ");
    u8g2.print(boolLevel);
    //Line 6
    u8g2.setCursor(0,60);
    u8g2.drawLine(0,51,128,51);
    u8g2.print("Menu");
    

    u8g2.sendBuffer();
    return;//if systemState==1
      
  }//system State == 1
  if (systemState ==2)
  {
    u8g2.setCursor(0,10);
    if (wifiCon)
    {
      int sig = WiFi.RSSI();
      if (sig>-55)
      {
        //u8g2.print("Excel");  
        u8g2.drawXBM(0, 0, Wifi_Good_width, Wifi_Good_height, Wifi_Good_bits);
      }
      else if (sig>-60)
      {
        //u8g2.print("Good");  
        u8g2.drawXBM(0, 0, Wifi_Good_width, Wifi_Good_height, Wifi_Good_bits);
      }
      else if (sig>-67)
      {
        u8g2.drawXBM(0, 0, Wifi_OK_width, Wifi_OK_height, Wifi_OK_bits);
      }
      else if (sig>-80)
      {
        //u8g2.print("Weak");  
        u8g2.drawXBM(0, 0, Wifi_Weak_width, Wifi_Weak_height, Wifi_Weak_bits);
      }
      else
      {
        //u8g2.print("Discon");              
        u8g2.drawXBM(0, 0, Wifi_Discon_width, Wifi_Discon_height, Wifi_Discon_bits);
      }
      
    }
    else
    {
      u8g2.print("No WiFi");
    }
    //Print time to lcd
    u8g2.setCursor(81,10);//Move the time the top right of the lcd screen.
    if (timeinfo.tm_hour>9)
    {
      u8g2.print(timeinfo.tm_hour);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(timeinfo.tm_hour);  
    }
    u8g2.print(":");  
    if (timeinfo.tm_min>9)
    {
      u8g2.print(timeinfo.tm_min);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(timeinfo.tm_min);  
    }
    u8g2.print(":");  
    if (timeinfo.tm_sec>9)
    {
      u8g2.print(timeinfo.tm_sec);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(timeinfo.tm_sec);  
    }  
    //******************************//
    //Line 1
    u8g2.setCursor(0,10);
    //Line 2
    u8g2.setCursor(0,20);
    
    //Line 3
    u8g2.setCursor(0,30);
    u8g2.print("Pump ON: ");
    unsigned long timeLeft = timerPumpON.remaining();
    int pumpH = timeLeft / 3600000;
    int pumpM = ((timeLeft-pumpH*3600000) / 60000);
    int pumpS = ((timeLeft-pumpM*60000) / 1000);
    if (pumpH>9)
    {
      u8g2.print(pumpH);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(pumpH);  
    }
    u8g2.print(":");  
    if (pumpM>9)
    {
      u8g2.print(pumpM);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(pumpM);  
    }
    u8g2.print(":");  
    if (pumpS>9)
    {
      u8g2.print(pumpS);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(pumpS);  
    } 
    //Line 4
    u8g2.setCursor(0,40);
    u8g2.print("Flow Rate: ");
    u8g2.print(flowRate); 
    u8g2.print(" l/min");
    
    //Line 5
    u8g2.setCursor(0,50);
    //Show the pump animation. boolBlink switches once a second
    if (boolBlink)
    {
    u8g2.drawXBM(110, 46, pumpWidth, pumpHeight, pump1Bmp);  
    }
    else
    {
      u8g2.drawXBM(110, 46, pumpWidth, pumpHeight, pump2Bmp);
    }
      u8g2.sendBuffer();
    return;//if systemState==2
      
  }//if systemState==2
  if (systemState ==4)
  {
    u8g2.setCursor(0,10);
    
    //Print time to lcd
    u8g2.setCursor(81,10);//Move the time the top right of the lcd screen.
    if (timeinfo.tm_hour>9)
    {
      u8g2.print(timeinfo.tm_hour);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(timeinfo.tm_hour);  
    }
    u8g2.print(":");  
    if (timeinfo.tm_min>9)
    {
      u8g2.print(timeinfo.tm_min);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(timeinfo.tm_min);  
    }
    u8g2.print(":");  
    if (timeinfo.tm_sec>9)
    {
      u8g2.print(timeinfo.tm_sec);  
    }
    else
    {
      u8g2.print("0");
      u8g2.print(timeinfo.tm_sec);  
    }  
    //******************************//
    //Line 1
    u8g2.setCursor(0,10);
    
    //Line 2
    u8g2.setCursor(0,20);
    u8g2.setFont(u8g2_font_t0_22b_tr);
    u8g2.print("Menu");
    u8g2.setFont(u8g2_font_squeezed_b7_tr);
    //Line 3
    u8g2.setCursor(0,30);
    
    //Line 4
    u8g2.setCursor(0,40);
    switch (menuIndex)
    {
      case 0: break;
      case 1: if (!pumpManual)
              {
                u8g2.print("Manual Pump ON");
              }
              else
              {
                u8g2.print("Manual Pump ON");
                u8g2.setCursor(0,50);
                u8g2.print("Currently pumping");

              }
              break;
      case 2: u8g2.print("Enter Bluetooth Config");
              break;
      case 3: u8g2.print("Manually set Time");
              break;
              
      default:break;
    }
    
    //Line 5
    u8g2.setCursor(0,50);
    u8g2.sendBuffer();
    return;//if systemState==4
      
  }//if systemState==4
  
  
}
//gets the local time from the wifi connection. Returns true if a valid time is found.
bool printLocalTime()
{
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return false;
  }
  
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");
  
  return true;
}

void SendMail(int index)
{
  //String authormail;// R/W
//String authorpassword;// R/W
//String recipientmail;// R/W
  SMTP_Message message;
  message.sender.name = F("Urban Oasis Hydroponics");
  message.sender.email = authormail;
  message.addRecipient(F("User"), recipientmail);
  if (index==1)
  {
    message.subject = F("Hydroponics System Startup ");
    //message.subject+=espMAC;
    String textMsg = "Your Urban Oasis Hydroponics system has restarted";
    message.text.content = textMsg.c_str();
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
    message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
  }
  if (index==2)
  {
    message.subject = F("Hydroponics System Hourly Update ");
    //message.subject+=espMAC;
    String textMsg = "Your Urban Oasis Hydroponics system is still running.\n";
    textMsg += "Current Temperature and Humidity are: ";
    textMsg += temp;
    textMsg += " C, ";
    textMsg += hum;
    textMsg += " %.\n";
    textMsg += "Current pump counter: ";
    textMsg += pumpCounter;
    textMsg += "\n";
    textMsg += "All time flow: ";
    textMsg += atFlow;
    textMsg += " litres. \n";
    textMsg += "Measured EC: ";
    //textMsg += ec;
    textMsg += "\n";
    textMsg += "Float level: ";
    textMsg += boolLevel;
    textMsg += "\n";
    textMsg += "Lights: ";
    textMsg += boolLights;
    textMsg += "\n";
    
    message.text.content = textMsg.c_str();
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
    message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
  }
  if (index==3)
  {
    message.subject = F("Hydroponics System Flow Failure! ");
    //message.subject+=espMAC;
    String textMsg = "Warning! Your Urban Oasis Hydroponics system had no flow with pump switched on.\n";
    message.text.content = textMsg.c_str();
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
    message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
  }
  if (index==4)
  {
    message.subject = F("Hydroponics System Reservoir Level Low! ");
    //message.subject+=espMAC;
    String textMsg = "Warning! Your Urban Oasis Hydroponics system's reservoir level is low.\n";
    message.text.content = textMsg.c_str();
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
    message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
  }
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()){
    Serial.println("\nNot yet logged in.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
  
}
void smtpCallback(SMTP_Status status)
{
   /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)
      
      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
  
}
//used to send info to server
void httpUpdate()
{
  /*
  name = preferences.getString("name","UOH System");
  ssid = preferences.getString("ssid","Admin");
  password = preferences.getString("password","Admin");
  wifistr=0;
  mac = ESP.getEfuseMac();
  authormail=preferences.getString("authormail","admin@admin.com");
  authorpassword=preferences.getString("authorpassword","admin");
  recipientmail = preferences.getString("recipientmail","client@admin.com");
  pumpTime = preferences.getInt("pumpTime",60000);
  pumpOnTime = preferences.getInt("pumpOnTime",10000);
  pumpPower = preferences.getInt("pumpPower",128);
  timeLEDON = preferences.getString("timeLEDON","06:00");
  timeLEDOFF = preferences.getString("timeLEDOFF","18:00");
  lED1Bright = preferences.getInt("lED1Bright",50);
  lED2Bright = preferences.getInt("lED2Bright",50);
  lED3Bright = preferences.getInt("lED3Bright",50);
  temp = 0;//float
  hum = 0;//float
  pumpCount = preferences.getInt("pumpCounter",0);
  flowCount = preferences.getFloat("atFlow",0.0);
  floatSense = 0;
  */

  HTTPClient http;  
  
  String apiKeyValue = "tQmAT5Ah3j3F9";
  String http_id_name = name + mac;
  String http_pumpTime = String(pumpTime);
  String http_pumpOnTime = String(pumpOnTime);
  String http_pumpPower = String(pumpPower);
  String http_timeLEDON = String(timeLEDON);
  String http_timeLEDOFF = String(timeLEDOFF);
  String http_temp = String(temp);
  String http_tempWater = String(water_celsius);
  String http_hum = String(hum);
  String http_pumpCount = String(pumpCount);
  String http_flowCount = String(flowCount);
  String http_floatSense = String(floatSense);
  String http_status = String(status);

  Serial.println("***HTTP UPdate***");
  Serial.println(http_id_name);
  Serial.println(http_pumpTime);
  Serial.println(http_pumpOnTime);
  Serial.println(http_pumpPower);
  Serial.println(http_timeLEDON);
  Serial.println(http_timeLEDOFF);
  Serial.println(http_temp);
  Serial.println(http_tempWater);
  Serial.println(http_hum);
  Serial.println(http_pumpCount);
  Serial.println(http_flowCount);
  Serial.println(http_floatSense);
  Serial.println(http_status);

  String data_to_send = "action=write&api_key=" + apiKeyValue +//create the string that is sent via hhtp POST to write to database
                        "&name=" + http_id_name + 
                        "&pumpTime=" + http_pumpTime + 
                        "&pumpOnTime=" + http_pumpOnTime + 
                        "&pumpPower=" + http_pumpPower + 
                        "&timeLEDON=" + http_timeLEDON + 
                        "&timeLEDOFF=" + http_timeLEDOFF + 
                        "&temp=" + http_temp + 
                        "&tempWater=" + http_tempWater + 
                        "&hum=" + http_hum + 
                        "&pumpCount=" + http_pumpCount + 
                        "&flowCount=" + http_flowCount + 
                        "&floatSense=" + http_floatSense + 
                        "&status=" + http_status + "";

Serial.println("***data to send***");
Serial.println(data_to_send);

http.begin("http://www.hydrogallery.co.za/hydro-post-data.php");   //Indicate the destination webpage 
http.addHeader("Content-Type", "application/x-www-form-urlencoded"); 
int response_code = http.POST(data_to_send);                                //Send the POST. This will giveg us a response code
if(response_code > 0)
{
  Serial.println("HTTP code " + String(response_code));
  String response_body = http.getString();
  Serial.println (response_body);
}

http.end();


}
void read_Temp_18B20()
{
   if (!oneWire.search(addr)) {
    //Serial.println("No more addresses.");
    oneWire.reset_search();
    delay(250);
    return;
  }
  //Serial.print("Address: ");
  /* Serial.println(addr[0]);
  Serial.println(addr[1]);
  Serial.println(addr[2]);
  Serial.println(addr[3]);
  Serial.println(addr[4]);
  Serial.println(addr[5]);
  Serial.println(addr[6]);
  Serial.println(addr[7]); */
 
  // check the CRC of the address
  if (OneWire::crc8(addr, 7) != addr[7]) {
    //Serial.println("CRC is not valid!");
    return;
  }
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      //Serial.println("Device is not recognized!");
      return;
  }
 
  // start conversion
  oneWire.reset();
  oneWire.select(addr);
  oneWire.write(0x44, 1);
 
  // wait for conversion to complete
  //delay(1000);
 
  // read scratchpad
  present = oneWire.reset();
  oneWire.select(addr);
  oneWire.write(0xBE);
 
  // read data
  for (i_18b20 = 0; i_18b20 < 9; i_18b20++) {
    data[i_18b20] = oneWire.read();
  }
 
  // convert temperature data
  int16_t raw = (data[1] << 8) | data[0];
  water_celsius = (float)raw / 16.0;
 
  // print temperature
  //Serial.print("Temperature: ");
  //Serial.print(water_celsius);
  //Serial.println(" °C");
 
  // delay before reading from the sensor again
  //delay(1000);
}
