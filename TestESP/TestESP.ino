#include "SoftwareSerial.h"

SoftwareSerial esp8266(6, 7); // RX, TX

void setup()
{
  Serial.begin(19200); // serial port used for debugging
  esp8266.begin(19200);  // your ESP's baud rate might be different
}

void sendCmd(String cmd) {
  Serial.print(">> ");
  Serial.println(cmd);
  esp8266.println(cmd);
}

String HTTPGET("GET / HTTP/1.0\r\n\r\n");

void sendYa() {
  sendCmd("AT+CIPSTART=\"TCP\",\"213.180.204.3\",80");
  delay(2000);
  
  if(Serial.find("Error")) {
    Serial.println("fail");
    return;
  }
  
  sendCmd("AT+CIPSEND=18");
  if(Serial.find(">")) {
    sendCmd(HTTPGET);
  }
}

void loop()
{
  if(esp8266.available())  // check if the ESP is sending a message
  {
    while(esp8266.available())
    {
      char c = esp8266.read();  // read the next character.
      Serial.write(c);  // writes data to the serial monitor
    }
  }
 
  if(Serial.available())
  {
    delay(500);  // wait to let all the input command in the serial buffer

    // read the input command in a string
    String cmd = "";
    while(Serial.available())
    {
      cmd += (char)Serial.read();
    }

    if(cmd == "ya\r\n") {
      sendYa();
    } else {
      // print the command and send it to the ESP
      Serial.println("---------------------");
      Serial.print(">> ");
      Serial.println(cmd);
      Serial.println("---------------------");
      esp8266.println(cmd); // send the read character to the esp8266
    }
  }
}
