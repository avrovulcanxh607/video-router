/*
 * NMS Modification of QUARTZ Q1600-AV-1616 Video Router
*/

#include <EtherCard.h>
#include <EEPROM.h>

//static byte mymac[] = { 0x51,0x75,0x61,0x72,0x74,0x7A };  // MAC Address (Quartz)
//static byte myip[] = { 192,168,1,234 }; // IP Address

static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
static byte myip[] = { 192,168,1,200 };

byte Ethernet::buffer[500];
BufferFiller bfill;

void setup() {
  pinMode(A0,OUTPUT); // Destination Address Line MSB
  pinMode(A1,OUTPUT); // Destination Address Line 2
  pinMode(A2,OUTPUT); // Destination Address Line 3
  pinMode(A3,OUTPUT); // Destination Address Line LSB
  pinMode(A4,OUTPUT); // Destination Enable (Active LOW)

  pinMode(2,OUTPUT); // Source Address Bus MSB
  pinMode(3,OUTPUT); // Source Address Bus Line 2
  pinMode(4,OUTPUT); // Source Address Bus LSB
  pinMode(5,OUTPUT); // Source Bank Select (1-8)
  pinMode(6,OUTPUT); // Source Bank Select (9-16)

  pinMode(9,OUTPUT); // Buzzer/Boot LED
  pinMode(8,OUTPUT); // Network Error LED
 
  digitalWrite(8,HIGH); // (off by default)

  if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0) // Set up Ethernet
    digitalWrite(8,LOW);  // Turn on the Network Error LED 
  ether.staticSetup(myip);
  
  digitalWrite(A4,HIGH);
  Serial.begin(9600); // Set up Serial
  for (int dest=0; dest<=15; dest++)
  {
    int source=EEPROM.read(dest);
    route(source,dest);
  }
  digitalWrite(9,LOW);
}

static word homePage() {
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Pragma: no-cache\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "\r\n"
    "[$D,$D,$D,$D,$D,$D,$D,$D,$D,$D,$D,$D,$D,$D,$D,$D]"),
      EEPROM.read(0), EEPROM.read(1), EEPROM.read(2), EEPROM.read(3), EEPROM.read(4), EEPROM.read(5), EEPROM.read(6), EEPROM.read(7), EEPROM.read(8),
      EEPROM.read(9), EEPROM.read(10), EEPROM.read(11), EEPROM.read(12), EEPROM.read(13), EEPROM.read(14), EEPROM.read(15));
  return bfill.position();
}

void loop() {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if (pos)  // check if valid tcp data is received
  {
    char* data = (char*) Ethernet::buffer + pos;
    data += 5;
    if(data[0] == '?')
    {
      int source = (data[1]-65);
      int dest = (data[2]-65);

      if(source < 16 && dest < 16)
      {
        digitalWrite(9,HIGH);
        route(source,dest);
        EEPROM.update(dest, source);
        digitalWrite(9,LOW);

        Serial.write(" ");
        Serial.print(bitRead(dest,3));
        Serial.print(bitRead(dest,2));
        Serial.print(bitRead(dest,1));
        Serial.print(bitRead(dest,0));
        Serial.write(" ");
        Serial.print(bitRead(source,3));
        Serial.print(bitRead(source,2));
        Serial.print(bitRead(source,1));
        Serial.print(bitRead(source,0));
        Serial.write(" \r\n[ OK ] Routed\r\n");
      }
    }
    ether.httpServerReply(homePage()); // send web page data
  }
  delay(100);
}

void route(int source, int destination) {
  /*
   * Sets up the source and destination busses for a particular address.
  */
  digitalWrite(A0,bitRead(destination,0));
  digitalWrite(A1,bitRead(destination,1));
  digitalWrite(A2,bitRead(destination,2));
  digitalWrite(A3,bitRead(destination,3));

  digitalWrite(2,bitRead(source,0));
  digitalWrite(3,bitRead(source,1)); // choose source
  digitalWrite(4,bitRead(source,2));

  if(bitRead(source,3))  // Do we want 1-8 or 9-16?
  {
    digitalWrite(6,LOW);
    digitalWrite(5,HIGH);
  }
  else
  {
    digitalWrite(6,HIGH);
    digitalWrite(5,LOW);
  }
  delay(100);
  digitalWrite(A4,LOW);
  delay(100);
  digitalWrite(A4,HIGH);
}
