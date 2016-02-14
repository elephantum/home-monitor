#include "DHT.h"
#define DHTPIN D4     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define WIFI_SSID "ft108"
#define WIFI_PASS "makamaka"

#define SENSOR_ID "wm1"

////// Конфигурация

// Ethernet
#define MY_UDP_PORT 8888

IPAddress STATSD_SERVER_IP(52,30,30,39);
#define STATSD_SERVER_PORT 8125
#define DATA_SEND_INTERVAL 10000

// read data every second
#define SENSOR_READ_INTERVAL 1000

//////

DHT dht(DHTPIN, DHTTYPE);

//EthernetServer server(MY_HTTP_PORT);
WiFiUDP udp;

float temp_c;
float humidity;
unsigned long sensor_last_read;

unsigned long data_last_sent;

void setup() {
  Serial.begin(9600); // Open serial connection to report values to host
  Serial.println("Starting up");

  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("Starting UDP");
  udp.begin(MY_UDP_PORT);
  Serial.print("Local port: ");
  Serial.println(MY_UDP_PORT);
  
  dht.begin();

  temp_c = 0;
  humidity = 0;
  sensor_last_read = 0;
}

void read_data() {
  unsigned long current_time = millis();
  if(sensor_last_read + SENSOR_READ_INTERVAL < current_time) {
    // Read values from the sensor
    temp_c = dht.readTemperature();
    humidity = dht.readHumidity();

    sensor_last_read = current_time;
  }
}

void send_metric(char *name, float val) {
  char msg[255];
  char val_str[100];

  dtostrf(val, 0, 4, val_str);
  sprintf(msg, "sensor.%s.%s:%s|g", SENSOR_ID, name, val_str);

  Serial.println(msg);
  udp.beginPacket(STATSD_SERVER_IP, STATSD_SERVER_PORT);
  udp.write(msg);
  udp.endPacket();
}

void send_data() {
  unsigned long current_time = millis();

  if(data_last_sent + DATA_SEND_INTERVAL < current_time) {
    Serial.println("Sending data to statsd");

    send_metric("temperature", temp_c);
    send_metric("humidity", humidity);
    send_metric("uptime", millis()/1000);

    data_last_sent = current_time;
  }
}

void loop() {
  read_data();
  send_data();
}
