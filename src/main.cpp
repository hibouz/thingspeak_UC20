#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP085.h>
#include "U8glib.h"
#include <SoftwareSerial.h>

SoftwareSerial UC20(2, 3);

Adafruit_BMP085 bmp;
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);

#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

float pressure = 0.0;
float tempBMP = 0.0;
float altitude = 0.0;
float tempDHT = 0.0;
float humedad = 0.0;

int pwrKey = 5;
int statKey = 7;
int uc20State = 0;

String str = "";


void SensorRead(float *pressure, float *tempBMP, float *altitude, float *tempDHT, float *humedad);
void DisplayPresTemp(float *pressure, float *tempBMP, float *altitude, float *tempDHT, float *humedad);
void power_on();
void config_uc20();
void Send_thingspeak(float *pressure, float *tempBMP, float *altitude, float *tempDHT, float *humedad);
int send_ATcommand(String ATcommand, char *response_correct, unsigned int time);

void setup()
{
  Serial.begin(9600);
  UC20.begin(9600);
  dht.begin();

  pinMode(statKey, INPUT);
  pinMode(pwrKey, OUTPUT);
  digitalWrite(pwrKey, LOW);

  if (u8g.getMode() == U8G_MODE_R3G3B2)
  {
    u8g.setColorIndex(255);
  }
  else if (u8g.getMode() == U8G_MODE_GRAY2BIT)
  {
    u8g.setColorIndex(3);
  }
  else if (u8g.getMode() == U8G_MODE_BW)
  {
    u8g.setColorIndex(1);
  }
  else if (u8g.getMode() == U8G_MODE_HICOLOR)
  {
    u8g.setHiColorByRGB(255, 255, 255);
  }

  for (int a = 0; a < 30; a++)
  {
    u8g.firstPage();

    do
    {
      u8g.setFont(u8g_font_fub11);
      u8g.setFontRefHeightExtendedText();
      u8g.setDefaultForegroundColor();
      u8g.setFontPosTop();
      u8g.drawStr(0, a, "HIBOUZ IoT"); //PARA CAMBIAR EL LOGO DE INICIO https://javl.github.io/image2cpp/
    } while (u8g.nextPage());
  }

  if (!bmp.begin())
  {
    u8g.firstPage();

    do
    {
      u8g.setFont(u8g_font_fub11);
      u8g.setFontRefHeightExtendedText();
      u8g.setDefaultForegroundColor();
      u8g.setFontPosTop();
      u8g.drawStr(4, 0, "BMP085 Sensor");
      u8g.drawStr(4, 20, " ERROR!");
    } while (u8g.nextPage());

    Serial.println("BMP085 sensor, ERROR!");

    while (1)
    {
    }
  }

}

void loop()
{
  uc20State = digitalRead(statKey);
  if (uc20State == HIGH)
  {
    power_on();
  }

  SensorRead(&pressure, &tempBMP, &altitude, &tempDHT, &humedad);
  delay(1000);
  DisplayPresTemp(&pressure, &tempBMP, &altitude, &tempDHT, &humedad);
  delay(1000);
  Send_thingspeak(&pressure, &tempBMP, &altitude, &tempDHT, &humedad);
  
  delay(10000);
}

void SensorRead(float *pressure, float *tempBMP, float *altitude, float *tempDHT, float *humedad)
{
  *pressure = bmp.readPressure() / 100.0;
  *altitude = bmp.readAltitude();
  *tempBMP = bmp.readTemperature();
  *tempDHT = dht.readTemperature();
  *humedad = dht.readHumidity();
}
void DisplayPresTemp(float *pressure, float *tempBMP, float *altitude, float *tempDHT, float *humedad)
{
  u8g.firstPage();
  do
  {
    u8g.setFont(u8g_font_8x13);
    u8g.setFontRefHeightExtendedText();
    u8g.setDefaultForegroundColor();
    u8g.setFontPosTop();

    u8g.drawStr(4, 0, "WEATHER STATION");

    u8g.drawStr(0, 15, "Pressure");
    u8g.setPrintPos(75, 15);
    u8g.print(*pressure, 1);

    u8g.drawStr(0, 27, "Altitude");
    u8g.setPrintPos(75, 27);
    u8g.print(*altitude, 1);

    u8g.drawStr(0, 40, "Temp BMP");
    u8g.setPrintPos(75, 40);
    u8g.print(*tempBMP, 1);

    u8g.drawStr(0, 53, "Humedad");
    u8g.setPrintPos(75, 53);
    u8g.print(*humedad, 1);

  } while (u8g.nextPage());
}

int send_ATcommand(String ATcommand, char *response_correct, unsigned int time)
{
  int x = 0;
  bool correct = 0;
  char response_ATommand[250];
  unsigned long past;

  memset(response_ATommand, '\0', 100);
  delay(5);

  while (UC20.available() > 0) UC20.read();

  UC20.println(ATcommand);
  x = 0;
  past = millis();

  do
  {
    if (UC20.available() != 0)
    {
      response_ATommand[x] = UC20.read();
      x++;
      if (strstr(response_ATommand, response_correct) != NULL)
      {
        correct = 1;
      }
    }
  } while ((correct == 0) && ((millis() - past) < time));
  Serial.println(response_ATommand);
  return correct;
}

void power_on()
{
  digitalWrite(pwrKey, HIGH);
  delay(200);
  digitalWrite(pwrKey, LOW);
  Serial.println("Encendiendo Modulo UC20");
  Serial.println("Esperar 10 segundos...");
  Serial.println("");
  delay(10000);
  config_uc20();
}
void config_uc20()
{
  Serial.println("Configurando Modulo UC20");
  Serial.println("");
  delay(1000);
  while (send_ATcommand("AT", "OK", 2000) == 0)
  {
  }
  delay(1000);
  while (send_ATcommand("AT+CMEE=2", "OK", 2000) == 0)
  {
  }
  delay(1000);
  while (send_ATcommand("AT+CPIN?", "+CPIN: READY", 1000) == 0)
  {
  }
  delay(1000);
  while (send_ATcommand("AT+CREG?", "+CREG: 0,1", 3000) == 0)
  {
  }
  delay(1000);
  while (send_ATcommand("AT+CGREG?", "+CGREG: 0,1", 3000) == 0)
  {
  }
  delay(1000);
  while (send_ATcommand("AT+COPS?", "OK", 3000) == 0)
  {
  }
  delay(1000);
  while (send_ATcommand("AT+CGATT=1", "OK", 3000) == 0)
  {
  }
  delay(1000);
  while (send_ATcommand("AT+QHTTPCFG=\"contextid\",1", "OK", 2000) == 0)
  {
  }
  delay(1000);
  while (send_ATcommand("AT+QICSGP=1,1,\"internet.movistar.com.co\",\"\",\"\",1", "OK", 3000) == 0)
  {
  }
  delay(1000);
}

void Send_thingspeak(float *pressure, float *tempBMP, float *altitude, float *tempDHT, float *humedad)     
{

  if (send_ATcommand("AT+QIACT?\r", "+QIACT: 1,1,1", 5000) == 1)
  {

    str = "http://api.thingspeak.com/update?api_key=JBGZYNFP7L2Z6U6V&field1=" + String(*pressure, 1) + "&field2=" + String(*altitude, 1) + "&field3=" + String(*tempBMP, 1) + "&field4=" + String(*humedad, 1)+"\n";

    unsigned int lenurl = str.length()+1;
    String tamURL = (String)lenurl;
    delay(1000);

    if (send_ATcommand("AT+QHTTPURL=" + tamURL + ",80", "CONNECT", 2000) == 1)
    {
      UC20.println(str);
      Serial.println(str);
    }
    delay(3000);

    if (send_ATcommand("AT+QHTTPGET=80", "OK", 5000) == 1)
    {
      Serial.println("Trama Enviada!!!");
    }

    delay(3000);

  } else if (send_ATcommand("AT+QIACT?\r", "ERROR", 5000) == 1)
  {
    send_ATcommand("AT+QIDEACT=1", "OK", 5000);
  } else if (send_ATcommand("AT+QIACT?\r", "OK", 5000) == 1)
  {
    send_ATcommand("AT+QIACT=1", "OK", 5000);
  }

  delay(5000);
}