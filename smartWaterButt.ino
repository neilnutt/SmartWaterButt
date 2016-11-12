//#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SerialUI.h>
//#include <EEPROMAnything.h>

#define serial_baud_rate           57600
#define serial_input_terminator   '\n'
#define trigPin 3
#define echoPin 2

#define deviceid_maxlen  10
#define postcode_maxlen  8
#define owner_id_maxlen  30

unsigned long lastWaterLevelPublishTime = 0;
unsigned long lastPulseTime = 0;
long waterlevel;
float CurTime = 0;
 
// Update these with values suitable for your network.
byte mac[]    = { 0xDE, 0xAD, 0xAA, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 2, 178);
const char* server = "iot.eclipse.org";

// deviceInfo struct, a place to store our settings
typedef struct deviceInfo {
  char deviceid[deviceid_maxlen + 1];
  char postcode[postcode_maxlen + 1];
  char shortcode[postcode_maxlen -1];
  char owner_id[owner_id_maxlen + 1];
  float TopDistance; // mm
  float BottomDistance; // mm
  float ButtArea; 
  float RoofArea;
  float riseCorrectionFactor;
}
deviceInfo;
deviceInfo thisDevice = {0};
 
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);


SUI_DeclareString(device_greeting,
                  "+++ Smart Water Butt +++\r\nEnter '?' to list available options.");
SUI_DeclareString(top_menu_title, "Smart Water Butt Main Menu");
SUI_DeclareString(instructions_key, "Instructions");

SUI_DeclareString(set_deviceid_string_key, "0");
SUI_DeclareString(set_deviceid_string_help, "Device ID");
SUI_DeclareString(set_postcode_string_key, "1");
SUI_DeclareString(set_postcode_string_help, "Enter UK postcode (AB12 3CD)");
SUI_DeclareString(set_ownerid_string_key, "2");
SUI_DeclareString(set_ownerid_string_help, "Enter the owner's username");

SUI_DeclareString(set_topdistance_float_key, "3");
SUI_DeclareString(set_topdistance_float_help, "Sensor to top outlet (mm)");
SUI_DeclareString(set_bottomdistance_float_key, "4");
SUI_DeclareString(set_bottomdistance_float_help, "Top to bottom outlet (mm)");

SUI_DeclareString(set_roofarea_float_key, "5");
SUI_DeclareString(set_roofarea_float_help, "Area of roof drained (sqm)");
SUI_DeclareString(set_buttarea_float_key, "6");
SUI_DeclareString(set_buttarea_float_help, "Area of water butt (sqm))");

SUI_DeclareString(exit_key, "exit");
SUI_DeclareString(exit_help, "exit (and terminate Druid)");

SUI_DeclareString(err_cantadd_command, "Could not addCommand to mainMenu?");
// declare  variables
SUI_DeclareString(label_string0, "Device ID");
SUI_DeclareString(label_string1, "Postcode");
SUI_DeclareString(label_string2, "Owner ID");
SUI_DeclareString(label_float3, "Distance (mm)");
SUI_DeclareString(label_float4, "Distance (mm)");
SUI_DeclareString(label_float5, "Area (sqm)");
SUI_DeclareString(label_float6, "Area (sqm)");


// create a serial ui instance
SUI::SerialUI mySUI = SUI::SerialUI(device_greeting);
 
void setup()
{
  Serial.begin(57600);
  Ethernet.begin(mac, ip);
  // Allow the hardware to sort itself out
  delay(1500);
  mqttClient.setServer(server, 1883);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
 
  if (mqttClient.connect("myClientID")) {
    Serial.println("connection succeeded");
    mqttClient.setCallback(callback);
    subscriptions();
  } else {
    // connection failed
    Serial.println(mqttClient.state());
    // on why it failed.
}
}

void subscriptions()
{
  Serial.println("Processing subscriptions");
  Serial.println(mqttClient.subscribe("uniqueid/maxtargetwl"));
}


void publishWaterlevel(long waterlevel)
{
  char payload [10];
  dtostrf(waterlevel, 6, 2, payload); // Leave room for too large numbers!

  Serial.print("uniqueid/wl");
  Serial.print(" , ");
  Serial.println(payload);
  Serial.println(mqttClient.publish("uniqueid/wl", payload));
}
 
void loop()
{
  if (!mqttClient.connected()) {
    Serial.print("Connection state = ");
    Serial.print(mqttClient.state());
  }
  delay(100);
  if (mySUI.checkForUser(150))
  {  mySUI.enter();
    while (mySUI.userPresent())
    {  mySUI.handleRequests();
      CurTime = millis(); }  }
      
  mqttClient.loop();

  // Every second identify the current water level
  if(millis() - lastPulseTime > 1000) {
    lastPulseTime = millis();
    long duration;
    Serial.print(digitalRead(trigPin));
    digitalWrite(trigPin, LOW);  // Added this line
    Serial.print(digitalRead(trigPin));
    delayMicroseconds(2); // Added this line
    Serial.print(digitalRead(trigPin));
    digitalWrite(trigPin, HIGH);
    Serial.print(digitalRead(trigPin));
    delayMicroseconds(10); // Added this line
    digitalWrite(trigPin, LOW);
    Serial.println(digitalRead(trigPin));
    duration = pulseIn(echoPin, HIGH);
    long oldDistance = waterlevel;
    Serial.print(oldDistance);
    Serial.print(" - ");
    Serial.println(waterlevel);
    waterlevel = (50*((duration/2) / 2.91) + 50*oldDistance)/100;

    // publish a message every 15 seconds.
    if(millis() - lastWaterLevelPublishTime > 15000) {
      lastWaterLevelPublishTime = millis();
      publishWaterlevel(waterlevel);
  }
}
}// end of loop()



void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

 

