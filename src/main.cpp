#include "main.h"

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

char macAddress[12];
unsigned long lastMillis = 0;
unsigned long publishDelayMs = PUBLISH_DELAY;
const char aws_iot_endpoint[] = MQTT_HOST;
const int aws_iot_port = MQTT_PORT;
const char aws_thing_name[] = MQTT_THING;

// epoch timestamp
const char *ntpServer = "pool.ntp.org";
unsigned long epochTime;

unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return (0);
  }
  time(&now);
  return now;
}

void handleTLSError()
{
  char errorBuffer[100];
  if (wifiClient.lastError(errorBuffer, sizeof(errorBuffer)) != 0)
  {
    Serial.print("TLS ERROR: ");
    Serial.println(errorBuffer);
  }
}

void connectWiFi()
{
  // Check if connected before trying to connect
  if (WiFi.status() == WL_CONNECTED)
    return;

  Serial.println("WiFi: Connecting...");
  // Setup and try to connect
  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Set certificates for AWS IoT Core and check for errors
  Serial.println("Setting AWS Root CA");
  wifiClient.setCACert(keys::CACert);
  handleTLSError();
  Serial.println("Setting Thing Certificate");
  wifiClient.setCertificate(keys::deviceCert);
  handleTLSError();
  Serial.println("Setting the Thing Private Key");
  wifiClient.setPrivateKey(keys::privateRSAKey);
  handleTLSError();

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  // Offset is not needed for time as we only get epoch time
  configTime(0, 0, ntpServer);
  setupMQTT();
}

void setupMQTT()
{
  Serial.println("MQTT: Connecting...");

  // Increase bufferSize 128->1024 to allow bigger messages
  mqttClient.setBufferSize(1024);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(onMQTTMessage);

  connectMQTT();
}

void connectMQTT()
{
  while (!mqttClient.connected())
  {
    Serial.println("MQTT trying to connect...");
    // Correct username needed according to AWS Iot Core policy to connect
    if (mqttClient.connect(MQTT_USERNAME))
    {
      Serial.println("MQTT CONNECTED!");

      Serial.println("Trying to subscribe...");
      if (mqttClient.subscribe(MQTT_TOPIC_CMD))
      {
        Serial.print("Subscribed to: ");
        Serial.println(MQTT_TOPIC_CMD);
      }
      else
      {
        Serial.println("Failed to subscribe!");
        Serial.println(mqttClient.state());
      }
    }
    else
    {
      Serial.print("MQTT Connection Failed, status = ");
      Serial.println(mqttClient.state());
    }
  }
}

void onMQTTMessage(char *topic, byte *payload, unsigned int length)
{
  Serial.print("MQTT: Message received from topic: ");
  Serial.println(topic);

  Serial.println(" --- Message START --- ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println(" --- Message END --- ");
}

void publishTemperature()
{
  epochTime = getTime();

  // Convert epochTime to char array
  char t_buffer[128];
  snprintf(t_buffer, sizeof(t_buffer), "%ld", epochTime);
  char *str_time = t_buffer;

  // Convert temperature reading to char array
  double temperature = measure();
  char str_val[6];
  dtostrf(temperature, 4, 2, str_val);

  char payload[640];

  /*
  * Format the payload to JSON for publishing
  * example:
  * {"id": "2T9BEA943123" , "temperature": 9.13 , "Date": 1639638580 } 
  * 
  */
  sprintf(payload, "%s", ""); // Clean the payload
  sprintf(payload, "{\"%s\":", "id");
  sprintf(payload, "%s \"%s\"", payload, macAddress);
  sprintf(payload, "%s , \"%s\":", payload, "temperature");
  sprintf(payload, "%s %s", payload, str_val);
  sprintf(payload, "%s , \"%s\":", payload, "Date");
  sprintf(payload, "%s %s", payload, str_time);
  sprintf(payload, "%s }", payload);

  // Try to publish to MQTT
  if (mqttClient.publish(MQTT_TOPIC_SEND, payload))
  {
    Serial.println(" --- Published to MQTT --- ");
    Serial.println(payload);
  }
  else
  {
    Serial.println("Failed to publish!");
  }
}

void setup()
{
  Serial.begin(BAUDRATE);
  Serial.println();
  Serial.println();

  // Delay incase of errors, so easier to flash again
  delay(3000);

  Serial.print("TEMP Monitor: ");
  // Read MAC address of ESP32, that should be unique
  uint8_t macAddr[6];
  esp_efuse_mac_get_default(macAddr);
  sprintf(macAddress, "%02X%02X%02X%02X%02X%02X", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

  Serial.println(macAddress);

  // Start WiFi Connection
  connectWiFi();
}

void loop()
{
  if (!mqttClient.connected())
  {
    connectMQTT();
  }
  mqttClient.loop();

  // Try to publish every (default 5) minutes
  if (millis() - lastMillis > publishDelayMs)
  {
    lastMillis = millis();

    publishTemperature();
  }
}