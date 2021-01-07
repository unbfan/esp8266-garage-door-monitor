#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/* This version 2 enables regualr WIFI connection check */
/* EPS-03 board is used for this project */
/* PCB board name: BFS2018A */
#define _DEBUG false

// defines pins numbers
#define PIN_TRIG 2
#define PIN_ECHO 0
#define PIN_ONE_WIRE 12 //Pin to which is attached a DS18B20 temperature sensor
#define PIN_LED 13

// for blink LED
unsigned long previousMillis = 0;
const long interval = 200;
int ledState = LOW;

// DS18B20
OneWire oneWire(PIN_ONE_WIRE);
DallasTemperature DS18B20(&oneWire);

const char *ssid = ... SSID...;
const char *password = ... WIFI - PASSWORD...;

const char *host = "api.thingspeak.com";
const char *privateKey = ... API - KEY...;

// const int READ_INTERVAL_DELAY = 10 * 60 * 1000; //delay 10 minutes = 10*60*1000ms
const int READ_INTERVAL_DELAY = 5 * 60 * 1000; //delay 5 minutes
// const int READ_INTERVAL_DELAY = 5 * 1000; //delay 5 sec

void setup()
{
  Serial.begin(115200); // Starts the serial communication

  digitalWrite(PIN_LED, HIGH); // Turn on LED before connecting to WIFI

  pinMode(PIN_TRIG, OUTPUT); // Sets the PIN_TRIG as an Output
  pinMode(PIN_ECHO, INPUT);  // Sets the PIN_ECHO as an Input
  pinMode(PIN_LED, OUTPUT);
  delay(10);

  // Connecting to a WiFi network
  if (_DEBUG)
  {
    Serial.println();
    Serial.print("Connecting to ... ");
    Serial.println(ssid);
  }

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    blink();
    delay(500);
    if (_DEBUG)
    {
      Serial.print(".");
    }
  }

  if (_DEBUG)
  {

    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  digitalWrite(PIN_LED, LOW); // Turn off LED after connected to WIFI
  // --- Done Wifi setup ---

  //Setup DS18b20 temperature sensor
  DS18B20.begin();
}

/*********************************************************************/

void checkWifiConnection()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    blink(); //blink led to indicate a disconnection happens

    // Connect to WiFi network
    WiFi.begin(ssid, password);

    if (_DEBUG)
    {
      Serial.println();
      Serial.print("\n\r \n\rRe-connecting");
    }

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
      blink();
      delay(500);
      blink();

      if (_DEBUG)
      {
        Serial.print(".");
      }
    }

    if (_DEBUG)
    {
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }

    digitalWrite(PIN_LED, LOW); // Turn off LED after connected to WIFI
  }
}

void blink()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    if (ledState == LOW)
    {
      ledState = HIGH; // Note that this switches the LED *off*
    }
    else
    {
      ledState = LOW; // Note that this switches the LED *on*
    }
    digitalWrite(PIN_LED, ledState);
  }
}

/*********************************************************************/

void loop()
{

  checkWifiConnection();

  int distance = readDistance();
  if (_DEBUG)
  {
    Serial.print("distance= ");
    Serial.print(distance);
    Serial.print("CM");
  }
  float temperature = readTemperature();

  if (_DEBUG)
  {
    Serial.print(", temperature= ");
    Serial.print(temperature, 4);
    Serial.println(" C");
  }

  digitalWrite(PIN_LED, HIGH);
  pushValue(distance, temperature);
  digitalWrite(PIN_LED, LOW);

  if (_DEBUG)
  {
    Serial.println("delay starts");
  }
  delay(READ_INTERVAL_DELAY); // delay in x minutes
  // Deep Sleep
  //Serial.println("ESP8266 in sleep mode");
  //ESP.deepSleep(10 * 1000);
  if (_DEBUG)
  {
    Serial.println("delay done");
  }
}

/**
   Update the reading to the thingspeak
*/
void pushValue(int distance, float temperature)
{
  if (_DEBUG)
  {
    Serial.print("connecting to ");
    Serial.println(host);
  }

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort))
  {
    if (_DEBUG)
    {
      Serial.println("connection failed");
    }
    return;
  }

  // Create a URI for the request
  String url = "/update";
  url += "?api_key=";
  url += privateKey;
  url += "&field1=";
  url += distance;
  url += "&field2=";
  url += temperature;

  if (_DEBUG)
  {
    Serial.print("Requesting URL: ");
    Serial.println(url);
  }
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 5000)
    {
      Serial.println(">>> Client Timeout ! <<<");
      client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available())
  {
    String line = client.readStringUntil('\r');
    if (_DEBUG)
    {
      Serial.println("Responses: ");
      Serial.println(line);
    }
  }
  if (_DEBUG)
  {
    Serial.println("Done updating cycle, waiting for next one to start.");
  }
}

/**
   Measure distance, in CM
   Using HC-SR04 module
*/
int readDistance()
{
  long duration;
  int distance;

  // Clears the PIN_TRIG
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  // Sets the PIN_TRIG on HIGH state for 10 micro seconds
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Reads the PIN_ECHO, returns the sound wave travel time in microseconds
  duration = pulseIn(PIN_ECHO, HIGH);

  // Calculating the distance
  distance = duration * 0.034 / 2; //== > in CM
  return distance;
}

/**
   Measuring the temperature, in Celsius
   Using DS18B20 Module
*/
float readTemperature()
{
  DS18B20.setWaitForConversion(false);      //No waiting for measurement
  DS18B20.requestTemperatures();            //Initiate the temperature measurement
  float tempC = DS18B20.getTempCByIndex(0); //Measuring temperature in Celsius, 0 refers to the first IC on the wire
  return tempC;
}
