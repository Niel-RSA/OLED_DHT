//ESP-32 Core v2 needed
//DHT library Adafruit
//U8G2 library
//ESP32 Mail Client V3
//OneWire
//Partition scheme: Huge App ""

//3 Mar added code to better format time and date.


//*************************************************************************
//****Libraries included
//*************************************************************************
//#include <BLEDevice.h>
//#include <BLEUtils.h>
//#include <BLEServer.h>
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
//#include <HTTPClient.h>
//ESP32 RTC library
#include <time.h>
//Digital humidity and temperature sensor
#include <DHT.h>
//LCD SCREEN INFO
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
//#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
#include <ESP_Mail_Client.h>//esp mail client by mobizt
#include <OneWire.h>

//*************************************************************************

//U8G2_ST7565_ERC12864_ALT_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* cs=D5*/ 5, /* dc=*/ 14, /* reset=*/ 12); // contrast improved version for ERC12864// lcd pin 1:CS 2:Reset 3:DC(RS) 4:Clock 5:Data
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

 
const int LED_BUILTIN = 2;//built in led is on pin 2.
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;
const int   daylightOffset_sec = 0;

bool wifiCon;
bool boolBlink;

float temp;
float minTemp;
float maxTemp;

float hum;
float minHum;
float maxHum;

float tCur;

String ssid;
String password;

int wIFI_Retry;
int arrayCount;

//declare float arrays
float tempArray[1440];
float humArray[1440];


//millisDelay timers
millisDelay timerMinMax;
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


struct tm timeinfo;//time info retrieved from ntp client


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




void setup() {
  //Serial port start and first message sent
  Serial.begin(9600);
  Serial.setTimeout(20);//20ms timeout
  //Serial.println(myDebug[0]);//"Program Started"
  //pin mode definitions
  //ledcAttach(LED_LCD,freq, resolution);
  //ledcAttach(LED_OUT1,freq, resolution);
  //ledcAttach(PUMP_OUT,freq, resolution);
    
  u8g2.begin();
  //u8g2.setContrast(75);
  //u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setFont(u8g2_font_9x15B_mr);
  u8g2.clearBuffer();
  u8g2.setCursor(0, 10);
  u8g2.print("Initializing...");
  u8g2.sendBuffer();
  
  //updateLCD();
 
  //Wifi Settings
  
  WiFi.mode(WIFI_STA);
  WiFi.begin("CashX", "CASHX1819");
  
  delay(1000);
  int connCount = 0;
  while ((WiFi.status() != WL_CONNECTED)&&connCount <201) {
    wifiCon=true;
	u8g2.clearBuffer();
	u8g2.setCursor(20, 10);
	u8g2.print("Wifi Connecting");
	u8g2.setCursor(0,50);
    u8g2.print(wifiCon);
	u8g2.sendBuffer();
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
 
  //
  //Arduino over the air setup
  
  
  
  
  //
  //Display all value from flash for preferences/Config
  dht.begin();
  
  temp = dht.readTemperature();//reads the temperature from the DHT22
  hum = dht.readHumidity();//reads the humidity from the DHT22
  timerBlink.start(1000);
  timerMinMax.start(60000);
  
  blink();
  //bleTime.start(45000);//time the bluetooth config stays active removed for v3_1
  //systemState=1;
 
}
void loop() {
 
 
  //server.handleClient();
  
  if (timerBlink.justFinished())
  {
    blink();    
    timerBlink.restart();//restart the timer after it has lapsed.
  }
  if (timerMinMax.justFinished())
  {
	  getMinMax();
	  timerMinMax.restart();
  }
  
}



//Triggers once a second due to timer lapsing 
void blink()	
{
  temp = dht.readTemperature();//reads the temperature from the DHT22
  hum = dht.readHumidity();//reads the humidity from the DHT22
 
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
		WiFi.begin("CashX", "CASHX1819");
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
  
  if (wifiCon)
  {
    tCur = timeinfo.tm_hour+(timeinfo.tm_min/60.0);
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
  
  

  
  
  
  
}

void updateLCD()
{
	u8g2.setFont(u8g2_font_inb21_mf);
	
    u8g2.clearBuffer();	
	//u8g2.drawLine(0,5,20,5);
	//Time
	int offset = u8g2.getStrWidth("00:00");
    u8g2.setCursor((64-(offset/2)),21);
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
	u8g2.drawLine(0,23,128,23);
	
	//Date
	u8g2.setFont(u8g2_font_boutique_bitmap_9x9_bold_tr);

	offset = u8g2.getStrWidth("Mon, 03 Mar");
  u8g2.setCursor((64-(offset/2)),32);
	u8g2.print(&timeinfo, "%a");
  u8g2.print(", ");
  if (timeinfo.tm_mday>9)
	{
	  u8g2.print(timeinfo.tm_mday);  
	}
	else
	{
	  u8g2.print("0");
	  u8g2.print(timeinfo.tm_mday);  
	}

	u8g2.print(" ");
	u8g2.print(&timeinfo, "%b");//month
	//Temperature and humidity
	u8g2.setFont(u8g2_font_crox1cb_mf);
	u8g2.drawLine(64,39,64,64);
    u8g2.setCursor(2,48);
    u8g2.print(temp,1);
	//u8g2.print(" ");
	u8g2.print(char(176));
    u8g2.print("C");  
    u8g2.setCursor(67,48);      
    u8g2.print(hum,1);
    u8g2.print(" %");    
    
	
	//Min max
	u8g2.setFont(u8g2_font_boutique_bitmap_9x9_t_all);
    u8g2.setCursor(2,58);
    if (minTemp <=100.0 && maxTemp>=-100.0 && minTemp>0.0 && maxTemp>0.0)
	{
		u8g2.drawUTF8(2,58,"↓");
		u8g2.setCursor(12,58);
		u8g2.print(minTemp,0);
		u8g2.drawUTF8(40,58,"↑");
		u8g2.setCursor(50,58);
		u8g2.print(maxTemp,0);
		
		u8g2.drawUTF8(66,58,"↓");
		u8g2.setCursor(76,58);
		u8g2.print(minHum,0);
		u8g2.drawUTF8(104,58,"↑");
		u8g2.setCursor(114,58);
		u8g2.print(maxHum,0);
	}
    

  
	u8g2.sendBuffer();
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

void getMinMax( )
{
	minTemp = 100.0;
	maxTemp = -100.0;
	minHum = 100.0;
	maxHum = -100.0;
	
	tempArray[arrayCount] = temp;
	humArray[arrayCount] = hum;
	arrayCount++;
	if (arrayCount > 1440)
	{
		arrayCount = 0;
	}
	for (int i = 0; i<1440; i++)
	{
		//Serial.println(tempArray[i], 7);
		//Serial.println(minTemp, 7);
		if ((tempArray[i] < minTemp)&&(tempArray[i] > 0.0))
		{
			minTemp = tempArray[i];

		}
		if ((tempArray[i] > maxTemp)&&(tempArray[i] > 0.0))
		{
			maxTemp = tempArray[i];
		}
		if ((humArray[i] < minHum)&&(humArray[i] > 0.0))
		{
			minHum = humArray[i];

		}
		if ((humArray[i] > maxHum)&&(humArray[i] > 0.0))
		{
			maxHum = humArray[i];
		}
	}	
	
	
}


