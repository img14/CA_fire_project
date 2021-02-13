//Based on examples from ArduinoJson and Arduino Wifi Rev 2 Tutorial

#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include "secrets.h"

bool isVerbose = true;

//ALL THE DATA
int numFiresWithin20 = 0;
int numFiresWithin50 = 0;
int numFiresWithin100 = 0;
int numFiresWithin200 = 0;

int totalFiresLA = 0;

int totalFires = 0;

int airQuality = 0;

int someKindOfFireWatch = 0;
int veryCloseFireWatch = 0;

bool flashLight = true; //Sorry...

int status = WL_IDLE_STATUS;

char fireServer[] = "services3.arcgis.com";  
char airServer[] = "api.airvisual.com";
char alertServer[] = "api.weather.gov";

WiFiSSLClient client;
void setup() 
{ 
  //Buttons and Lights
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(2, INPUT_PULLUP); //Refresh Button
  attachInterrupt(digitalPinToInterrupt(2), buttonISR, FALLING);

  testLights();
  
  Serial.begin(9600);
  //Uncomment to use serial
  /*while (!Serial) 
  {
    ;
  }*/
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) 
  {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  flashTwice();

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) 
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }

  flashTwice();
  digitalWrite(12, HIGH);

  Serial.println("Connected to wifi");
  Serial.println("\nStarting connection to server...");

  getFireData();
  if(isVerbose)
  {
    Serial.println();
    Serial.println("Fire Data Summary: ");
    Serial.println("Number of Fires within 20 mi: " + String(numFiresWithin20));
    Serial.println("Number of Fires within 50 mi: " + String(numFiresWithin50));
    Serial.println("Number of Fires within 100 mi: " + String(numFiresWithin100));
    Serial.println("Number of Fires within 200 mi: " + String(numFiresWithin200));
    Serial.println("Total Number of Fires in California: " + String(totalFires));
    Serial.println("Total Number of Fires in Los Angeles: " + String(totalFiresLA));
    Serial.println();
  }

  getAirData();
  if(isVerbose)
  {
    Serial.println("Air Quality: " + String(airQuality));
    Serial.println();
  }

  getAlertData();

  digitalWrite(12, LOW);
    
}
void loop()
{
   if(numFiresWithin20)
      digitalWrite(3, HIGH);
   else if(numFiresWithin50)
      digitalWrite(4, HIGH);
   else if(numFiresWithin100)
      digitalWrite(5, HIGH);
  else if(numFiresWithin200)
      digitalWrite(6, HIGH);
  if(airQuality > 250)
  {
    if(flashLight)
      digitalWrite(7, HIGH);
    else
      digitalWrite(7, LOW);
    flashLight = !flashLight;
  }
  else if(airQuality > 200)
    digitalWrite(7, HIGH);
  else if(airQuality > 150)
  {
    if(flashLight)
      digitalWrite(8, HIGH);
    else
      digitalWrite(8, LOW);
    flashLight = !flashLight;
  }
  else if(airQuality > 100)
    digitalWrite(8, HIGH);
  else if(airQuality > 50)
  {
    if(flashLight)
      digitalWrite(9, HIGH);
    else
      digitalWrite(9, LOW);
    flashLight = !flashLight;
  }
  else if(airQuality > 0)
    digitalWrite(9, HIGH);
  if(veryCloseFireWatch > 0)
    digitalWrite(10, HIGH);
  else if(someKindOfFireWatch > 0)
    digitalWrite(11, HIGH);
  delay(750);
}

void getFireData()
{
  if (client.connect(fireServer, 443)) { //SSL, use 80 for normal
    Serial.println("connected to server");
  client.println(F("GET /T4QMspbfLg3qTGWY/arcgis/rest/services/Active_Fires/FeatureServer/0/query?where=POOState%20%3D%20%27US-CA%27&outFields=CalculatedAcres,InitialLongitude,PercentContained,POOCounty,InitialLatitude,POOCity&outSR=4326&f=json HTTP/1.0"));
  client.println(F("Host: services3.arcgis.com"));
  client.println(F("Connection: close"));
  if (client.println() == 0) 
  {
    Serial.println(F("Failed to send request"));
    return;
  }

  
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  // Allocate JsonBuffer
  DynamicJsonDocument jsonBuffer(4000); //Hoping no single object will be bigger than this

  client.find("\"features\":[");
  do {
    deserializeJson(jsonBuffer, client);
    String county = jsonBuffer["attributes"]["POOCounty"];
    float latCoord = jsonBuffer["attributes"]["InitialLatitude"]; //Not sure why i have to do this step
    float longCoord = jsonBuffer["attributes"]["InitialLongitude"];
    float distance = sqrt(sq((latCoord - houseLatCoord)*69) + sq((longCoord - houseLongCoord)*55));
    if (jsonBuffer["attributes"]["CalculatedAcres"])
    {
      if (distance < 20) //stop judging me.
      {
        numFiresWithin20++;
      }
      if (distance < 50)
      {
        numFiresWithin50++;
      }
      if (distance < 100)
      {
        numFiresWithin100++;
      }
      if (distance < 200)
      {
        numFiresWithin200++;
      }
      if (county == "Los Angeles")
      {
        totalFiresLA++;
      }
      totalFires++;
      Serial.println("Latitude: " + String(latCoord));
      Serial.println("Longitude: " + String(longCoord));
      Serial.print("Area: ");
      serializeJson(jsonBuffer["attributes"]["CalculatedAcres"], Serial);
      Serial.println(" Acres");
      Serial.println("County: " + county);
      Serial.println();
      //Serial.print("Percent Contained: ");
      //serializeJson(jsonBuffer["attributes"]["PercentContained"], Serial);
      //Serial.println("%");
    }
    
} while (client.findUntil(",","]"));

  // Disconnect
  client.stop();
  Serial.println();
  Serial.println("Finished");
}

void getAirData()
{
  if (client.connect(airServer, 443)) { //SSL, use 80 for normal
    Serial.println("connected to server");
  String request = "GET /v2/nearest_city?lat="+ String(houseLatCoord,2) + "&lon=" + String(houseLongCoord,2) + "&key=" + key + " HTTP/1.0";
  client.println(request);
  client.println(F("Host: api.airvisual.com"));
  client.println(F("Connection: close"));
  if (client.println() == 0) 
  {
    Serial.println(F("Failed to send request"));
    return;
  }

  
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  // Allocate JsonBuffer
  DynamicJsonDocument jsonBuffer(4000); //Hoping no single object will be bigger than this
  deserializeJson(jsonBuffer, client);
  airQuality = jsonBuffer["data"]["current"]["pollution"]["aqius"];
  // Disconnect
  client.stop();
  Serial.println();
  Serial.println("Finished");
}

void getAlertData()
{
  if (client.connect(alertServer, 443)) { //SSL, use 80 for normal
    Serial.println("connected to server");
  client.println(F("GET /alerts/active?area=CA HTTP/1.0"));
  client.println(F("Host: api.weather.gov"));
  client.println(F("User-Agent: ArduinoWiFi/1.1"));
  client.println(F("Connection: close"));
  if (client.println() == 0) 
  {
    Serial.println(F("Failed to send request"));
    return;
  }

  
  }
  delay(5000);
  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0 && strcmp(status, "HTTP/1.0 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    client.stop();
    }
 
  // Allocate JsonBuffer
  DynamicJsonDocument jsonBuffer(4000); //Hoping no single object will be bigger than this

  client.find("\"features\": [");
  do {
      deserializeJson(jsonBuffer, client);
      String event = jsonBuffer["properties"]["event"];
      String sender = jsonBuffer["properties"]["senderName"];
      if(event == "Red Flag Warning" || event == "Fire Warning" || event == "Fire Weather Watch")
      {
        JsonArray zones = jsonBuffer["properties"]["geocode"]["UGC"].as<JsonArray>();
        for(int i = 0; i < zones.size(); i++)
        {
          String data = zones[i];
          if(data == "CAZ241")
          {
            veryCloseFireWatch = 1;
          }
          else if(data == "CAZ041" || data[3] == '2')
          {
            someKindOfFireWatch++;
         }
        }
        //https://www.weather.gov/media/pimar/FireZone/ca_s_firezone.pdf
        //https://www.weather.gov/media/pimar/PubZone/ca_s_zone.pdf
      }
      if(event != "null")
      {
        Serial.println(event + ", Issued by " + sender);
      }
    } while (client.findUntil(",","]"));
  // Disconnect
  client.stop();
  Serial.println();
  Serial.println("Finished");
}

void buttonISR()
{
  client.stop();
  Serial.println("Here from the interrupt");
  numFiresWithin20 = 0;
  numFiresWithin50 = 0;
  numFiresWithin100 = 0;
  numFiresWithin200 = 0;
  digitalWrite(12, HIGH);
  getFireData();
  if(isVerbose)
  {
    Serial.println();
    Serial.println("Fire Data Summary: ");
    Serial.println("Number of Fires within 20 mi: " + String(numFiresWithin20));
    Serial.println("Number of Fires within 50 mi: " + String(numFiresWithin50));
    Serial.println("Number of Fires within 100 mi: " + String(numFiresWithin100));
    Serial.println("Number of Fires within 200 mi: " + String(numFiresWithin200));
    Serial.println("Total Number of Fires in California: " + String(totalFires));
    Serial.println("Total Number of Fires in Los Angeles: " + String(totalFiresLA));
    Serial.println();
  }

  getAirData();
  if(isVerbose)
  {
    Serial.println("Air Quality: " + String(airQuality));
    Serial.println();
  }

 //getAlertData();
  digitalWrite(12, LOW);
}

void flashTwice()
{
  digitalWrite(12, HIGH);
  delay(250);
  digitalWrite(12, LOW);
  delay(250);
  digitalWrite(12, HIGH);
  delay(250);
  digitalWrite(12, LOW);
}

void testLights()
{
  digitalWrite(3, HIGH);
  delay(250);
  digitalWrite(3, LOW);
  digitalWrite(4, HIGH);
  delay(250);
  digitalWrite(4, LOW);
  digitalWrite(5, HIGH);
  delay(250);
  digitalWrite(5, LOW);
  digitalWrite(6, HIGH);
  delay(250);
  digitalWrite(6, LOW);
  digitalWrite(7, HIGH);
  delay(250);
  digitalWrite(7, LOW);
  digitalWrite(8, HIGH);
  delay(250);
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH);
  delay(250);
  digitalWrite(9, LOW);
  digitalWrite(10, HIGH);
  delay(250);
  digitalWrite(10, LOW);
  digitalWrite(11, HIGH);
  delay(250);
  digitalWrite(11, LOW);
  digitalWrite(12, HIGH);
  delay(250);
  digitalWrite(12, LOW);
}
