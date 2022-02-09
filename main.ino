#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#endif

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

//===========================================================> Deklarasi Sensor TDS Meter
#define TDS D0
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;
//========================================================================================

//===========================================================> Deklarasi Sensor PH 4502c
#define PH D3
int samples = 10;
float adc_resolution = 1024.0;
//======================================================================================

// Replace with your network credentials
const char* ssid     = "nodemcu";
const char* password = "nodemcu123";

// REPLACE with your Domain name and URL path or IP address with path
String host = "rendi-wahyu.000webhostapp.com";
const char fingerprint[] PROGMEM = "F3 1B B7 47 29 59 39 C1 91 7D B4 61 DA 4D EC 0D 8C E1 E7 C1";
const int httpPort = 443;  //HTTPS= 443 and HTTP = 80

String data = "";

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  pinMode(TDS, OUTPUT);
  pinMode(PH, OUTPUT);

  lcd.init(); // initialize the lcd
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(WiFi.localIP());
  delay(1000);
}

float ph (float voltage) {
  return 7 + ((2.5 - voltage) / 0.18);
}

int readTDS() {
  digitalWrite(TDS, HIGH);
  digitalWrite(PH, LOW);
  return analogRead(0);
}

int readPH() {
  digitalWrite(TDS, LOW);
  digitalWrite(PH, HIGH);
  return analogRead(0);
}

void loop() {
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U)  //every 40 milliseconds,read the analog value from the ADC
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = readTDS();    //read the analog value and store into the buffer
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 60000U)
  {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0); //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge = averageVoltage / compensationCoefficient; //temperature compensation
    tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
    //Serial.print("voltage:");
    //Serial.print(averageVoltage,2);
    //Serial.print("V   ");
    int measurings = 0;

    for (int i = 0; i < samples; i++)
    {
      measurings += readPH();
      delay(10);

    }

    float voltage = 5 / adc_resolution * measurings / samples;
    Serial.print("pH= ");
    Serial.println(ph(voltage));
    Serial.print("TDS Value:");
    Serial.print(tdsValue, 0);
    Serial.println("ppm");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(WiFi.localIP());
    lcd.setCursor(0, 1);
    lcd.print(ph(voltage));
    lcd.print(" ph ");
    lcd.print(tdsValue, 0);
    lcd.print(" ppm");

    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {

      WiFiClientSecure client;

      client.setInsecure();

      //  Serial.printf("Using fingerprint '%s'\n", fingerprint);
      client.setFingerprint(fingerprint);
      client.setTimeout(15000); // 15 Seconds
      client.setInsecure(); // this is the magical line that makes everything work

      if (!client.connect(host, httpPort)) { //works!
        Serial.println("connection failed");
        return;
      }

      // We now create a URI for the request
      data = "?ph=" + String(ph(voltage)) + "&ppm=" + String(tdsValue);
      String url = "/post.php";
      //        url += "?ph=10.00&ppm=8";
      url += data;

      Serial.print("Requesting URL: ");
      Serial.println(url);

      // This will send the request to the server
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");

      Serial.println();
      Serial.println("closing connection");
    }
  }
}

int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}
