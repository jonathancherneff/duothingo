// Duothingo 
// dual purpose thing: 
//   sensor hub: DS18B20 digital sensors on digital pin 3
//   power switch: sainsmart opto-isolated relay (4) on digital pin 4-7
//tested: working v.0 3/27/2017
//author: jc

//www.arduino.org/learning/tutorials/boards-tutorials/restserver-and-restclient
// OneWire DS18S20, DS18B20, DS1822 Temperature 
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library
#include <Wire.h>
#include <OneWire.h>
#include <UnoWiFiDevEd.h>

int poll_interval_ms = 10000;
int sensor_interval_ms = 3000;
//thinhghub constants ******SET THING_ID HERE
const char* connector = "rest";
const char* server = "www.thinghub.cloud";
const char* method = "GET";
const char* state_resource = "/deposit.svc/State?thing_id=3";
char* cmd = "/deposit.svc/SpokeTalk?t=3&e=28FFD3668B1603C6&d=-107.3";
CiaoData data;

int relayPin1 = 7;    // IN1 connected to digital pin 7
int relayPin2 = 6;    // IN2 connected to digital pin 6
int relayPin3 = 5;    // IN3 connected to digital pin 5
int relayPin4 = 4;    // IN4 connected to digital pin 4

OneWire  ds(3);  // on digital pin 3 (a 4.7K resistor is necessary)

void setup() {
 Serial.begin(9600);
 Serial.println("begin");
 pinMode(relayPin1, OUTPUT);      // sets the digital pin as output
 pinMode(relayPin2, OUTPUT);      // sets the digital pin as output
 pinMode(relayPin3, OUTPUT);      // sets the digital pin as output
 pinMode(relayPin4, OUTPUT);      // sets the digital pin as output
 digitalWrite(relayPin1, HIGH);   // Prevents relays from starting up engaged
 digitalWrite(relayPin2, HIGH);   // Prevents relays from starting up engaged
 digitalWrite(relayPin3, HIGH);   // Prevents relays from starting up engaged
 digitalWrite(relayPin4, HIGH);   // Prevents relays from starting up engaged
 Ciao.begin();
 pinMode(2, INPUT);  // from ciao sketch but why?
 delay(10000);       //allow time to connect to internet access pt
}

void loop() {
  byte addr[8];     // digital sensor SN (DSS) buffer
  float fahr_value; //farhenheit temperature
  String ext_id;    //string version of DSS
  String resp;
  doRequest(connector, server, state_resource, method);  //get switch states from thinghub
  resp = String(data.get(2));
  interpret_data(resp);
  while (ds.search(addr)){
      fahr_value = get_fahrenheit(addr);
      ext_id = get_str_address(addr);
      Serial.println(ext_id);
      repl(cmd,29,ext_id,false);
      repl(cmd,48,String(fahr_value,1),true);
      Serial.println(cmd);
      CiaoData data = Ciao.write(connector, server, cmd,"GET");
      delay(sensor_interval_ms);
  }
  ds.reset_search();
  delay(poll_interval_ms);
}

void doRequest(const char* conn, const char* server, const char* command, const char* method){
    data = Ciao.write(conn, server, command, method);
    if (!data.isEmpty()){
        Ciao.println( "State: " + String (data.get(1)) );
        Ciao.println( "Response: " + String (data.get(2)) );
        Serial.println( "State: " + String (data.get(1)) );
        Serial.println( "Response: " + String (data.get(2)) );
    }
    else{
        Ciao.println ("Write Error");
        Serial.println ("Write Error");
    }
}
void set_pin(int pin_no, int hilo){
  Serial.println("set pin " + String(pin_no) + " to " + String(hilo));
  if (hilo == 1){
    digitalWrite(pin_no, HIGH);
  }
  else {
    digitalWrite(pin_no, LOW);
  };
}
void interpret_data(String resp){
  int pin, hilo, token_start;
  char comma = ',';
  token_start = 0;
  pin=0;
  for (int i=0; i<resp.length();i++){
    //Serial.println(i);
    // Serial.println(resp.charAt(i));
   if (isDigit(resp.charAt(i))) {
      pin = 10 * pin + int(resp.charAt(i))-48;
      //Serial.print("pin = ");
      //Serial.println(pin);
    }
    else { 
      hilo = int(resp.charAt(i+1)) - 48;
      i=i+2;
      Serial.println(String(pin) + " " + String(hilo));
      set_pin(pin,hilo);
      pin = 0;
    }
  }
}
void repl(char* buf, int start, String ins, boolean null_t){
   int i;
   for (i=0; i < (ins.length()); i++){
    buf[start + i]=ins.charAt(i);
  }
  if (null_t){
    buf[start+i]=char(0);
  }
}

String get_str_address(byte addr[8]){
  byte i;
  String x_id;
  for( i = 0; i < 8; i++) {
    if (addr[i] < 16){   
      x_id += "0" + String(addr[i],HEX);
    }
      else {
      x_id += String(addr[i],HEX);
    }
  }
  x_id.toUpperCase();
  return x_id;
}
float get_fahrenheit(byte addr[8]){
  float celsius, fahrenheit;
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      //Serial.println("Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //Serial.println("Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //Serial.println("Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  //Serial.print("Data = ");
  //Serial.print(present, HEX);
  //Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    //Serial.print(data[i], HEX);
    //Serial.print(" ");
  }
  //Serial.print(" CRC=");
 // Serial.print(OneWire::crc8(data, 8), HEX);
  //Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  return fahrenheit;
}
