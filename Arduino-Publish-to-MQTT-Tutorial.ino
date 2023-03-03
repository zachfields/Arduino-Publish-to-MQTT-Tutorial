#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

//DEFINE SENSORS
const int totalSensors = 4;
const int startSensorNum = 1;

SoftwareSerial sensor1(14, 6); //RX, TX
SoftwareSerial sensor2(15, 7); //RX, TX
SoftwareSerial sensor3(16, 8); //RX, TX
SoftwareSerial sensor4(17, 9); //RX, TX

SoftwareSerial* sensors[] = {&sensor1, &sensor2, &sensor3, &sensor4};

//DEFINE NETWORK
byte mac[] = {0xA8, 0x61, 0x0A, 0xAE, 0x94, 0xB6};
IPAddress ip(192, 168, 69, 22); //comment this line if you are using DHCP
IPAddress server(192, 168, 69, 30);
EthernetClient ethClient;

//MQTT Parameters
String topicBase = "spring-hill/garden/bed/center/";
const char id[] = "GardenCenter";
PubSubClient client(ethClient);

long lastReconnectAttempt = 0;
const int reconnectInterval = 5000;

boolean reconnect() {
  if (client.connect("arduinoClient")) {
    Serial.println("Connecting to MQTT broker...");

  } else {

    Serial.print("failed, rc=");
    Serial.println(client.state());
  }
  return client.connected();
}

//Loop Intervals
const long interval = 30000;
unsigned long previousMillis = 0;

void setup() {

  Serial.begin(9600);

  //Initialize Sensors
  sensor1.begin(9600);
  sensor2.begin(9600);
  sensor3.begin(9600);
  sensor4.begin(9600);

  // Ethernet.begin(mac); //Use this if you want to use DHCP
  Ethernet.begin(mac, ip); //Use this if you want a static iP.
  delay(1500); //Used to allow time for the network to connect.

  client.setServer(server, 1883);

}

void loop() {

  if (!client.connected()) {
    Serial.println("Client Not Connected...");
    long now = millis();
    if (now - lastReconnectAttempt > reconnectInterval) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        Serial.println("Connected!");
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    client.loop();

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {

      previousMillis = currentMillis;

      readSensors();

    }
  }
}

void readSensors() {

  String sensorJSONOutput = "[";

  String sensorFullPubTopic = topicBase + "sensors";

  for (int i = 0; i < totalSensors; i++) {
    
    int num = i + startSensorNum;

    sensors[i]->listen();
    Serial.print("Listening to sensor ");
    Serial.println(num);

    float temp = getTemp(i);
    float humid = getHumid(i);
    int wetness = getWetness(i);

    //Print Sensor Data
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.print(" Humidity: ");
    Serial.print(humid);
    Serial.print(" Wetness: ");
    Serial.println(wetness);

    //Create JSON Data
    sensorJSONOutput += "{\"sensor\":";
    sensorJSONOutput += num;
    sensorJSONOutput += ",\"temp\":";
    sensorJSONOutput += temp;
    sensorJSONOutput += ",\"humid\":";
    sensorJSONOutput += humid;
    sensorJSONOutput += ",\"wettness\":";
    sensorJSONOutput += wetness;
    sensorJSONOutput += "}";

    if (num < totalSensors) {
      sensorJSONOutput += ",";
    }

    sensors[i]->stopListening();

  }

  sensorJSONOutput += "]";

  //Send Data to MQTT
  if (client.publish(sensorFullPubTopic.c_str(), sensorJSONOutput.c_str(), true)) {
    Serial.println("Successfully Published Sensor Data!");
  } else {
    Serial.println("Failed to Publish Sensor Data :(");
  }
  Serial.println(sensorJSONOutput);

}

//SENSOR FUNCTIONS
int getWetness(int i) {
  sensors[i]->print("w");
  while (! sensors[i]->read() == '=') {};
  return sensors[i]->parseInt();
}

float getTemp(int i) {
  sensors[i]->print("t");
  while (! sensors[i]->read() == '=') {};
  return sensors[i]->parseFloat();
}

float getHumid(int i) {

  sensors[i]->print("h");
  while (! sensors[i]->read() == '=') {};
  return sensors[i]->parseFloat();
}
