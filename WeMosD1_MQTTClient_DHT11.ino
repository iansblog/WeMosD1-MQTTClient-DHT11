#include "DHT.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

String payLoadType = "WeMosD1";
String payLoadID = "Loft";

// Update these with values suitable for your network.

const char* ssid = "WiFiName";
const char* pswd = "WiFiPassword";
const char* mqtt_server = "iotserver.local";
const char* topic = "sensorMessages";    //This is the [root topic]

#define DHTPIN D5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Define NTP Client to get time
const long utcOffsetInSeconds = 3600;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
int value = 0;

int status = WL_IDLE_STATUS;     // the starting Wifi radio's status

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pswd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}


String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

String composeClientID() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String clientId;
  clientId += "esp-";
  clientId += macToStr(mac);
  return clientId;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientId = composeClientID() ;
    clientId += "-";
    clientId += String(micros() & 0xff, 16); // to randomise. sort of

    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(topic, ("connected " + composeClientID()).c_str() , true );
      // ... and resubscribe
      // topic + clientID + in
      String subscription;
      subscription += topic;
      subscription += "/";
      subscription += composeClientID() ;
      subscription += "/in";
      client.subscribe(subscription.c_str() );
      Serial.print("subscribed to : ");
      Serial.println(subscription);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" wifi=");
      Serial.print(WiFi.status());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  dht.begin();

  timeClient.begin();
}

void loop() {
    Serial.println("======================================");
    Serial.println("Just confirming we are still connected to mqtt server");
    
    if (!client.connected()) {
      reconnect();
      }
      client.loop();
      
    float h = dht.readHumidity();
    float tc = dht.readTemperature();
  
    //Look to get a value from the DHT sensor, do not look at the sencor more then every 2 seconds so not to overload the sencor. 
    bool getValue = false;
    do {
      if (h == h) {
        getValue = true;
        } else {
          Serial.println("The DHT sensor did not give us a valid reading, asking again to get a valid one. ");
          delay(2000);
          h = dht.readHumidity();
          tc = dht.readTemperature();
          }  
        } while (!getValue);
        
    timeClient.update();
    
    String payLoad = "{\"type\": \""+payLoadType+"\", \"ID\": \""+payLoadID+"\", \"humidity\": "+h+", \"temperatureC\": "+tc+", \"EpochTime\": \""+ timeClient.getEpochTime() +"\" }";

    //Debug messages - Start
    //----------------------------------------------------------------------------------- 
    Serial.print("Publish topic: ");
    Serial.println(topic);

    Serial.print("Publish message: ");
    Serial.println(payLoad);
    //----------------------------------------------------------------------------------- 
    //Debug messages - End
    
    client.publish( topic, (char*) payLoad.c_str(), true );

    Serial.println("30 Second delay starting.");
    Serial.println("======================================");

    delay(30000); //Run this every 30 seconds
}
