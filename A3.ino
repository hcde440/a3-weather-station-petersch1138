
#include "config.h";
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Adafruit_MPL115A2.h>
#include <ArduinoJson.h>    //
char mac[18]; //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!

#include <ESP8266WiFi.h>    //Requisite Libraries . . .
#include "Wire.h"           
#include <PubSubClient.h>
Adafruit_MPL115A2 mpl; // mpl sensor object
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)
WiFiClient espClient;             // espClient object
PubSubClient mqtt(espClient);  // puubsubclient object
#define OLED_RESET     -1 // Reset pin # (-1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // OLED display object
float temp;
uint16_t lux;
unsigned long timer1; // hold the values of timers
char button[201]; 
char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array
// digital pin 2
#define BUTTON_PIN 2
sensor_t sensor;
char str_crnt[6]; 

// button state
bool current = false; // button states
bool last = false;

// Peter Schultz 4/23/19
// This code gets values for temperature and luminosity from two sensors and publishes them to an mqtt server.
// It also displays the values on a local OLED display.

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);

  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));
  timer1 = millis(); // now timer1 holds the time when the script started
  if (tsl.begin()) // initializes light sensor
  {
    Serial.println(F("Found a TSL2591 sensor"));
  } 
  else 
  {
    Serial.println(F("No sensor found ... check your wiring?"));
    while (1);
  }
  
  setup_wifi(); // call this to set up wifi
  mqtt.setServer(mqtt_server, 1883); // sets mqtt object server address

  tsl.getSensor(&sensor);
  mpl.begin(); // initialize sensors
  
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
}

void loop() {
  // put your main code here, to run repeatedly:

  if(millis() - timer1 > 5000) { // only runs every 5 seconds
    if (!mqtt.connected()) {
      reconnect(); // run reconnect method if connection stops
    }
    mqtt.loop(); //this keeps the mqtt connection 'active'
  
    temp = mpl.getTemperature();// these lines get the current sensor values
    lux = tsl.getLuminosity(TSL2591_VISIBLE);
  
    Serial.print("Temperature: ");
    Serial.println(temp, 1); 
    Serial.print("Luminosity: ");
    Serial.println(lux);
  
    char str_tmp[4];
    char str_lux[5];
  
    String(temp).toCharArray(str_tmp,4); //converts to char array
    String(lux).toCharArray(str_lux,5);
 
    sprintf(message, "{\"temp\":\"%s\", \"lum\":\"%s\"}", str_tmp, str_lux); // constructs message in json format
  
    mqtt.publish("fromPeter/tempLux", message); // publishes message to the topic 
    
    display.clearDisplay(); // clears display
    display.setCursor(10, 0);
    display.setTextSize(1); // Draw 2X-scale text
    display.setTextColor(WHITE);
    display.print("Temperature: ");
    display.println(temp); // writes the values for temp and lux to the OLED display
    display.print("Luminosity: ");
    display.println(lux);
    display.display();
    delay(100);
    timer1 = millis(); // update timer to be the last time it changed sensor values
    Serial.println("******************************"); //give us some space on the serial monitor read out
  }
  if(digitalRead(BUTTON_PIN) == LOW)
    current = true;
  else
    current = false;

  if(current == last) 
    return; // stop if the state hasnt changed
  else
    String(current).toCharArray(str_crnt,6);

    sprintf(button, "{\"button\":\"%s\"}",str_crnt); // constructs button state in json format
    mqtt.publish("fromPeter/buttonState", button); // publishes button state if the state has changed
    Serial.println(button);
    
  last = current; // updates last button state each iteration
}

////CONNECT/RECONNECT/////Monitor the connection to MQTT server, if down, reconnect, usees credentials in config.h
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("fromPeter/+"); //we are subscribing to 'fromPeter' and all subtopics below that topic
    } else {                        //please change 'fromPeter' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void setup_wifi() { // this method connects to the wifi using the variables contained in config.h
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  WiFi.macAddress().toCharArray(mac, 18);
  Serial.println(mac);  //.macAddress returns a byte array 6 bytes representing the MAC address
}           
