#include <SHT1x.h>

#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <SPI.h>

 
// SHT1x Humidity/Temp sensor setup
#define SENSOR_DATA_PIN  4
#define SENSOR_CLOCK_PIN 5
SHT1x sht1x(SENSOR_DATA_PIN, SENSOR_CLOCK_PIN);


// Ethernet
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
// assign an IP address for the controller:
IPAddress ip(192, 168, 1, 50);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetServer server(80);

// read data every second
#define READ_DELAY 1000

float temp_c;
float humidity;
unsigned long sensor_last_read;

void setup() {
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();

  Serial.begin(9600); // Open serial connection to report values to host
  Serial.println("Starting up");

  // give the sensor and Ethernet shield time to set up:
  delay(1000);
  
  temp_c = 0;
  humidity = 0;
  sensor_last_read = 0;
}

void read_data() {
  if(sensor_last_read + READ_DELAY < millis()) {
    // Read values from the sensor
    temp_c = sht1x.readTemperatureC();
    humidity = sht1x.readHumidity();

    sensor_last_read = millis();

    // Print the values to the serial port
    Serial.print("Temperature: ");
    Serial.print(temp_c, DEC);
    Serial.print("C ");
    Serial.print(" Humidity: ");
    Serial.print(humidity);
    Serial.println("%");
  }
}

void serve_http() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Got a client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          // print the current readings, in HTML format:
          client.print("Temp: ");
          client.print(temp_c);
          client.print("<br>");

          client.print("Humidity: ");
          client.print(humidity);
          client.print("<br>");
          
          client.print("Last update time: ");
          client.print(sensor_last_read);
          client.print("<br>");

          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}


void loop() {
  read_data();
  serve_http();
}
