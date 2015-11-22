#include <SHT1x.h>

// TODO:20 переехать на wifi-соединение
#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <SPI.h>

////// Конфигурация

// SHT1x Humidity/Temp sensor setup
#define SENSOR_DATA_PIN  4
#define SENSOR_CLOCK_PIN 5

// Ethernet
byte MY_MAC[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress MY_IP(192, 168, 1, 50);
IPAddress MY_GATEWAY(192, 168, 1, 1);
IPAddress MY_SUBNET(255, 255, 255, 0);
#define MY_UDP_PORT 8888
#define MY_HTTP_PORT 80

IPAddress STATSD_SERVER_IP(52,30,205,62);
#define STATSD_SERVER_PORT 8125
#define DATA_SEND_INTERVAL 10000

// read data every second
#define SENSOR_READ_INTERVAL 1000

//////

SHT1x sht1x(SENSOR_DATA_PIN, SENSOR_CLOCK_PIN);

EthernetServer server(MY_HTTP_PORT);
EthernetUDP udp;

float temp_c;
float humidity;
unsigned long sensor_last_read;

unsigned long data_last_sent;

void setup() {
  Serial.begin(9600); // Open serial connection to report values to host
  Serial.println("Starting up");

  // start the Ethernet connection and the server:
  Serial.println("Starting Ethernet with DHCP");
  if (Ethernet.begin(MY_MAC) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP, using hardcoded IP");
    Ethernet.begin(MY_MAC, MY_IP);
  }
  Serial.print("My IP: ");
  Serial.println(Ethernet.localIP());

  udp.begin(MY_UDP_PORT);
  server.begin();

  // give the sensor and Ethernet shield time to set up:
  delay(1000);

  temp_c = 0;
  humidity = 0;
  sensor_last_read = 0;
}

void read_data() {
  unsigned long current_time = millis();
  if(sensor_last_read + SENSOR_READ_INTERVAL < current_time) {
    // Read values from the sensor
    temp_c = sht1x.readTemperatureC();
    humidity = sht1x.readHumidity();

    sensor_last_read = current_time;
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

void send_metric(char *name, float val) {
  char msg[255];
  char val_str[100];

  dtostrf(val, 0, 4, val_str);
  sprintf(msg, "%s:%s|g", name, val_str);

  Serial.println(msg);
  udp.beginPacket(STATSD_SERVER_IP, STATSD_SERVER_PORT);
  udp.write(msg);
  udp.endPacket();
}

void send_data() {
  unsigned long current_time = millis();

  if(data_last_sent + DATA_SEND_INTERVAL < current_time) {
    Serial.println("Sending data to statsd");

    send_metric("sensor.temperature", temp_c);
    send_metric("sensor.humidity", humidity);
    send_metric("sensor.uptime", millis()/1000);

    data_last_sent = current_time;
  }
}

void loop() {
  read_data();
  send_data();
  serve_http();
}
