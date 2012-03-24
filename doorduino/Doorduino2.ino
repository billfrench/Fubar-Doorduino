#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>

#include <stdlib.h>

/*
doorduino.pde
 Fubar-Doorduino Project
 by Phil Gillhaus 6/11/10
 updated to Arudino 1.0 by Bill French 2/16/2012
 expanded to include a second door, more verbose status
 updates, temperature, and a relay circa 3/24/2012
 This sketch, when used in combination with the ethernet
 shield and the switch based lock and door sensors at 
 the FUBAR Labs hackerspace will serve a page declaring
 whether the space is open or closed.
 */


byte localmac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte localip[] = { 
  192, 168, 1, 3 };

EthernetServer server(80);


int Door2LED = 6;
int Door1LED = 7;
int Relay = 5;
int Door2 = 8;
int Door1 = 9;
int TempSensor = 2;
int DO_RESET_ETH_SHIELD = A3;

int Door1Status;
int Door2Status;

double lockTimer = 0;
double doorTimer = 0;
int lockBounce = 20;
int doorBounce = 20;

// ThingSpeak Variable Setup
long lastConnectionTime = 0; 
boolean lastConnected = false;
int resetCounter = 0;

byte ThingSpeakIP[]  = { 
  184, 106, 153, 149 }; // IP Address for the ThingSpeak API
String ThingSpeakWriteAPIKey = "4FX246PAE5LWMCU1";    // Write API Key for a ThingSpeak Channel
long ThingSpeakUpdateInterval = 6000;        // Time interval in milliseconds to update ThingSpeak   
EthernetClient ThingSpeakClient;

void setup()
{
  //Ethernet.begin(mac, ip);
  Serial.begin(9600);
  init_ethernet();
  server.begin();
  pinMode(Door1, INPUT);
  pinMode(Door2, INPUT);
  pinMode(Door1LED, OUTPUT);
  pinMode(Door2LED, OUTPUT);
  pinMode(Relay, OUTPUT);
  Serial.print("HELLO");
}

void loop()
{
  Door1Status = digitalRead(Door1);
  Door2Status = digitalRead(Door2);
  digitalWrite(Door1LED, Door1Status);
  digitalWrite(Door2LED, Door2Status);
  digitalWrite(Relay, Door2Status);

  EthernetClient webclient = server.available();
  servPage(webclient);

  //Start the ThingsSpeak Stuff
  if (ThingSpeakClient.available())
  {
    char c = ThingSpeakClient.read();
    //Serial.print(c);
  }

  // Disconnect from ThingSpeak
  if (!ThingSpeakClient.connected() && lastConnected)
  {
    //Serial.println();
    //Serial.println("...disconnected.");
    //Serial.println();

    ThingSpeakClient.stop();
  }

  // Update ThingSpeak
  if((millis() - lastConnectionTime) > ThingSpeakUpdateInterval)
  {
    // Serial.println("Times up!");
    if(!ThingSpeakClient.connected())
    {
      // Serial.println("Starting Thingspeak!");
      updateThingSpeak("field1="+String(Door2Status || Door1Status)+"&field2="+String(Door1Status && !Door2Status)+"&field3="+String(Door2Status)+"&field4="+String(Door1Status)+"&field5="+String(CurrentTemperature()), ThingSpeakClient);
    }
  }
  delay(1);
}


void updateThingSpeak(String tsData, EthernetClient client)
{
  if (client.connect(ThingSpeakIP, 80))
  { 
    //Serial.println("Connected to ThingSpeak...");
    //Serial.println();

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+ThingSpeakWriteAPIKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");

    client.print(tsData);

    lastConnectionTime = millis();

    resetCounter = 0;

  }
  else
  {
    //Serial.println("Connection Failed.");   
    //Serial.println();

    resetCounter++;

    if (resetCounter >=5 ) {
      resetEthernetShield(client);
    }

    lastConnectionTime = millis(); 
  }
}

void resetEthernetShield(EthernetClient client)
{
  Serial.println("Resetting Ethernet Shield.");   
  Serial.println();

  client.stop();
  delay(1000);

  Ethernet.begin(localmac, localip);
  delay(1000);
}

int CurrentTemperature()
{
  int reading = analogRead(TempSensor);  
  int tempF = map(reading, 0, 1023, -58, 852);
  return tempF;
}

void servPage(EthernetClient client) {
  if (client) {
    // an http request ends with a blank line
    boolean current_line_is_blank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if we've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so we can send a reply
        if (c == '\n' && current_line_is_blank) {
          // send a standard http response header 
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();

          if (Door1Status == HIGH || Door2Status == HIGH)
          {
            client.print("The space is open.<br>");
            if(Door1Status)
            {
              client.print("Classroom door is unlocked.<br>");
            }
            if(Door2Status)
            {
              client.print("Main door is unlocked.<br>");
            }
          }
          else
          {
            client.print("The space is closed.<br>");
          }

          client.print("Current Temperature: ");
          client.print(CurrentTemperature());
          client.print("F<br>");

          break;
        }
        if (c == '\n') {
          // we're starting a new line
          current_line_is_blank = true;
        } 
        else if (c != '\r') {
          // we've gotten a character on the current line
          current_line_is_blank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
  }
}
//From: http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1225354009/30
//User: burni
void init_ethernet()
{
  pinMode(DO_RESET_ETH_SHIELD, OUTPUT);      // sets the digital pin as output
  digitalWrite(DO_RESET_ETH_SHIELD, LOW);
  delay(1000);  //for ethernet chip to reset
  digitalWrite(DO_RESET_ETH_SHIELD, HIGH);
  delay(1000);  //for ethernet chip to reset
  pinMode(DO_RESET_ETH_SHIELD, INPUT);      // sets the digital pin input
  delay(1000);  //for ethernet chip to reset
  Ethernet.begin(localmac,localip);
  delay(1000);  //for ethernet chip to reset
  Ethernet.begin(localmac,localip);
  delay(1000);  //for ethernet chip to reset
}








