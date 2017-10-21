#include <ArduinoJson.h>
#include <DHT.h>

#define DHTPIN 22
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#include <WiFiClientSecure.h>

const char* ssid     = "";
const char* password = "";

const char* server = "xxxxxxxx.amazonaws.com";
const int httpsPort = 8443;

const char* test_root_ca = \
  "-----BEGIN CERTIFICATE-----\n" \
  "-----END CERTIFICATE-----";

const char* test_client_key = \
  "-----BEGIN CERTIFICATE-----\n" \
  "-----END CERTIFICATE-----\n";

const char* test_client_cert = \
  "-----BEGIN RSA PRIVATE KEY-----\n" \
  "-----END RSA PRIVATE KEY-----\n";

int pushButton = 4;
WiFiClientSecure client;

void setup()
{
  Serial.begin(115200);
  pinMode(pushButton, INPUT);
  dht.begin();
  delay(10);
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  int buttonState = digitalRead(pushButton);

  float temp, humi;

  if (buttonState == LOW) {
    dht11(&temp, &humi);
    request(temp, humi);
    delay(1000);
  }
}

void request(float temp_data, float humi_data) {

  //jsonデータ作成
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& array = jsonBuffer.createArray();
  JsonObject& reported = array.createNestedObject();
  JsonObject& data = array.createNestedObject();
  data["Humidity"] = humi_data;
  data["Temperature"] = temp_data;

  reported["reported"] = data;
  root["state"] = reported;

  String output;
  root.printTo(output);

  client.setCACert(test_root_ca);
  client.setCertificate(test_client_key);
  client.setPrivateKey(test_client_cert);

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, httpsPort))
    Serial.println("Connection failed!");
  else {
    Serial.println("Connected to server!");
    // Make a HTTP request:
    
    // シャドウステータスの更新
    // client.println("POST /things/ESP32/shadow HTTP/1.1");
    // トピックの更新
    client.println("POST /topics/ESP32/DHT11 HTTP/1.1");
    
    client.println("Host: axxxxxxxx.amazonaws.comm");
    client.print("Content-Length: ");
    client.println(String(output.length()));
    client.println("Connection: close");
    client.println();
    client.println(output);
    Serial.println(output);

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        //Serial.print(line);
        break;
      }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    client.stop();
  }
}

void dht11(float* rtn_temp, float* rtn_humi) {

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
  }
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  *rtn_temp = hic;
  *rtn_humi = h;
}

