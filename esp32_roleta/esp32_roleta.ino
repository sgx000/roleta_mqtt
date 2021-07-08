#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Encoder.h>
#include <SimpleTimer.h>
#include "OTA.h"
#include "Secret.h"


const char* topic = "iot/roleta_";

#define mqtt_server "192.168.1.6"
#define mqtt_port 1883
#define mqtt_user ""
#define mqtt_password ""

const char* pos_topic  = "/position";
const char* req_topic  = "/request";

const char* encoder_topic     = "/encoder/value";
const char* encoder_topic_in  = "/encoder/input";
const char* encoder_topic_max = "/encoder/max";

#define MOTOR_A   13
#define MOTOR_B   15
#define MOTOR_PWM 19

SimpleTimer firstTimer(1000);
bool flag = false;

int roleta_pos = 0;
int roleta_request = 0; 
int encoder_value = 0;
int encoder_max = 100;
int encoder_in = 0;
int boot = 0;
int loop_increment = 0;


WiFiClient espClient;
PubSubClient client;
ESP32Encoder encoder;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setup_wifi();
  client.setClient(espClient);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  encoder.attachHalfQuad(17, 16);
  pinMode(MOTOR_A, OUTPUT);
  pinMode(MOTOR_B, OUTPUT);
  pinMode(MOTOR_PWM, OUTPUT);
  encoder.clearCount();
  setupOTA("roleta", wifi_ssid, wifi_password);
}


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  delay(500);
  Serial.println(WiFi.localIP());
  String clientId = composeClientID() ;
  Serial.println(clientId);
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    //if (i < 5)
    //  result += ':';
  }
  return result;
}

String composeClientID() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String clientId;
  
  clientId += macToStr(mac);
  return clientId;
}

void reconnect() {
  String subscription;
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESPClient", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      subscription = "";
      subscription += topic;
      subscription += composeClientID() ;
      subscription += req_topic;
      client.subscribe(subscription.c_str());
      Serial.println(subscription);
      subscription = "";
      subscription += topic;
      subscription += composeClientID() ;
      subscription += encoder_topic_max;
      client.subscribe(subscription.c_str());
      Serial.println(subscription);
      subscription = "";
      subscription += topic;
      subscription += composeClientID() ;
      subscription += encoder_topic_in;
      client.subscribe(subscription.c_str());
      Serial.println(subscription);
      
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char temp_array[10] = {};
  int encoder_val=0;
  String tmp;
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");
 
 

 
 //if(strcmp(topic, req_topic) > 0){
 if(String(topic).substring(23) == req_topic){
    for (int i = 0; i < length; i++) {
        char receivedChar = (char)payload[i];
        temp_array[i] = (char)payload[i];
        Serial.print(receivedChar);
      
    }
    sscanf(temp_array, "%d", &roleta_request);
 }
 


 //if(strcmp(topic, encoder_topic_max) > 0){
 if(String(topic).substring(23) == encoder_topic_max){
    for (int i = 0; i < length; i++) {
        char receivedChar = (char)payload[i];
        temp_array[i] = (char)payload[i];
        Serial.print(receivedChar);

    }
    
    sscanf(temp_array, "%d", &encoder_max);
   
 }
 //Input last state of encoder
 if(String(topic).substring(23) == encoder_topic_in){
    for (int i = 0; i < length; i++) {
        char receivedChar = (char)payload[i];
        temp_array[i] = (char)payload[i];
        Serial.print(receivedChar);

    }
    
    sscanf(temp_array, "%d", &encoder_in);
    encoder.setCount(encoder_in);
   
 }
  
 
 //roleta_pos = roleta_request;
 Serial.println();
}
  
void loop() {
  String temp;
  // put your main code here, to run repeatedly:
 if (!client.connected()) {
    reconnect();
  }
if(boot ==0){
  boot=1;
      temp = "";
      temp += topic;
      temp += composeClientID() ;
      temp += "/startup";
    client.publish(temp.c_str(), "hello", true);
}
if(loop_increment >10){
  if(roleta_request < roleta_pos){
      digitalWrite(MOTOR_A,   HIGH);
      digitalWrite(MOTOR_B,   LOW);
      digitalWrite(MOTOR_PWM, HIGH);
  } 
      
  if(roleta_request > roleta_pos){
      digitalWrite(MOTOR_PWM, HIGH);
      digitalWrite(MOTOR_B, HIGH);
      digitalWrite(MOTOR_A, LOW);    
  }

  if (roleta_request == roleta_pos){
      digitalWrite(MOTOR_PWM, LOW);
  }
}
  encoder_value = encoder.getCount();
  //roleta_pos = map(encoder_value, 0, encoder_max, 0, 100);
  roleta_pos =  encoder_value / (encoder_max/100.0) ;
  client.loop();
  if (firstTimer.isReady()){ 
    ArduinoOTA.handle();
    if(loop_increment<10){
      loop_increment++;
      Serial.println(loop_increment);
    }
    // Publishes a random 0 and 1 like someone switching off and on randomly (random(2))
      temp = "";
      temp += topic;
      temp += composeClientID() ;
      temp += pos_topic;
    client.publish(temp.c_str(), String(roleta_pos).c_str(), true);
      temp = "";
      temp += topic;
      temp += composeClientID() ;
      temp += encoder_topic;
    client.publish(temp.c_str(), String(encoder_value).c_str(), true);

    firstTimer.reset(); 
  }
  //client.publish(encoder_topic_calc, String(roleta_pos*(encoder_max/100.0)).c_str(), true);
  
}
