/*---------------------------------------------------------------------------------------------
  Starting code taken from the Open Sound Control (OSC) library for the ESP8266/ESP32

  Originally it was an example for sending messages from the ESP8266/ESP32 to a remote computer
  The example is sending "hello, osc." to the address "/test".

  This example code is in the public domain.

--------------------------------------------------------------------------------------------- */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <Wire.h>
#include "MMA7660.h"
MMA7660 acc;

char ssid[] = "XXXXXX";          // your network SSID (name) 
char pass[] = "XXXXXX";                    // your network password   

WiFiUDP Udp;                                // A UDP instance to let us send and receive packets over UDP  TO  BE UPDATED
const IPAddress outIp(XXX,XXX,XXX,XX);        // remote IP of your computer 
const unsigned int outPort = 8001;          // remote port to receive OSC, one unique port number per cube 8000, 8001, 8002, 80003, 8004
const unsigned int localPort = 8888;        // local port to listen for OSC packets (actually not used for sending)


float alpha = 0.7f;
float historyVal = 0.f;
float expHist1 = 0.f;
float expHist2 = 0.f;
float expHist3 = 0.f;
float maximum = -1.f;

int8_t x;
int8_t y;
int8_t z;

float attack_coef;
float release_coef;
float envelope;

float delayTime = 100.f;

float attack_in_ms = 0.01;
float release_in_ms = 10000.f;
float samplerate;


void setup() {
    Serial.begin(115200);

    // Connect to WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println("Starting UDP");
    Udp.begin(localPort);
    Serial.print("Local port: ");
    Serial.println(Udp.localPort());

acc.init();
samplerate = 1.f / (delayTime/1000.f);
attack_coef = pow(0.01, 1.0/( attack_in_ms * samplerate * 0.001));
release_coef = pow(0.01, 1.0/( release_in_ms * samplerate * 0.001));
envelope = 0.0;

}

void loop() {

    acc.getXYZ(&x, &y, &z);
    
    float val = sqrt(x*x + y*y + z*z); //Euclidean distance == total movement.
    float movement = abs(val - historyVal);
    historyVal = val;
    
    movement = expFilter(movement, alpha, expHist1);
    maximum = max(maximum, movement); 
    
    movement = mapfloat(movement, 0.f, 16.1f, 0.f, 1.f);
    movement = constrain(movement, 0.f, 1.f);
    
    float tmp = fabs(movement);
    if(tmp > envelope) 
        envelope = attack_coef * (envelope - tmp) + tmp;
    else
        envelope = release_coef * (envelope - tmp) + tmp;
        
    Serial.println(envelope);

    // OSC Messages to be sent
    OSCMessage msg("/ableton/track1/vol"); //this message can be modified for something more meaningful
    msg.add(envelope);
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    delay(delayTime);
}
    

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
 return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float expFilter(float val, float alpha, float history)
{
   float y = val * alpha + (1.f - alpha)* history;
   history = y;
   return y;
} 
