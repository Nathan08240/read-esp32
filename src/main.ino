#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <Credentials.h>

#define SS_PIN 5   // ESP32 pin GIOP5
#define RST_PIN 33 // ESP32 pin GIOP27

const int buzzer = 21; // buzzer to pin 21

uint8_t ledR = 25;
uint8_t ledG = 26;
uint8_t ledB = 27;

uint8_t ledArray[3] = {1, 2, 3};

const boolean invert = true;

uint8_t color = 0;
uint32_t R, G, B;
uint8_t brightness = 255;

MFRC522 rfid(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

const char *ssid = ssid_name;
const char *password = ssid_password;

// 0e1222c02scfl7
const char *mqtt_server = mqtt_server_name;
const char *mqtt_username = mqtt_server_username;
const char *mqtt_password = mqtt_server_password;
const int mqtt_port = mqtt_server_port;
const char *CLIENT_ID = client_id;
const char *badger = badger_topic;
const char *api = api_topic;

WiFiClientSecure espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

void setup()
{

  Serial.begin(9600);
  delay(10);

  ledcAttachPin(ledR, 1);
  ledcAttachPin(ledG, 2);
  ledcAttachPin(ledB, 3);

  ledcSetup(1, 12000, 8);
  ledcSetup(2, 12000, 8);
  ledcSetup(3, 12000, 8);

  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    setColor(255, 0, 0);
    delay(150);
    setColor(0, 0, 0);
  }
  randomSeed(micros());
  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());

  SPI.begin();
  rfid.PCD_Init();

  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");

  pinMode(buzzer, OUTPUT);

  espClient.setInsecure(); // set security or not to wifi connect
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  while (!Serial)
    delay(1);
}

void loop()
{

  if (!client.connected())
    reconnect();
  client.loop();

  setColor(0, 0, 255);

  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;
  byte block;
  byte len;
  if (!rfid.PICC_IsNewCardPresent())
  {
    setColor(255, 0, 0);
    return;
  }
  if (!rfid.PICC_ReadCardSerial())
  {
    setColor(255, 0, 0);
    return;
  }
  Serial.println(F("**Card Detected:**"));
  byte buffer1[18];
  block = 6;
  len = sizeof(buffer1);
  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("Authentication failed: "));
    Serial.println(rfid.GetStatusCodeName(status));

    setColor(255, 0, 0);
    delay(500);

    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(240);
    noTone(buzzer); // Son qui blink
    delay(25);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(240);
    noTone(buzzer);
    delay(1);

    return;
  }
  status = rfid.MIFARE_Read(block, buffer1, &len);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("Reading failed: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }
  String header = "";
  for (uint8_t i = 0; i < 4; i++)
  {
    header += (char)buffer1[i];
  }

  String student_id = "";
  for (uint8_t i = 4; i < 11; i++)
  {
    student_id += buffer1[i];
  }

  if (header == "CESI")
  {
    String message = "{\"school_student_id\":\"" + student_id + "\"}";
    publishMessage(badger, message, true);
  }
  // else pour renvoie erreur

  Serial.println(F("\n**End Reading**\n"));
  delay(1);
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection…");
    if (client.connect(CLIENT_ID, mqtt_username, mqtt_password))
    {
      Serial.println("connected");

      client.subscribe(api);
    }
    else
    {
      Serial.print("failed,");
      Serial.println(" try again..");
      setColor(0, 0, 255);
      delay(150);
      setColor(0, 0, 0);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String incommingMessage = "";
  for (int i = 0; i < length; i++)
    incommingMessage += (char)payload[i];

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, incommingMessage);
  int code = doc["code"];
  checkCode(code);

  delay(300);
}

void publishMessage(const char *topic, String payload, boolean retained)
{
  if (client.publish(topic, payload.c_str(), true))
    Serial.println("Message publised [" + String(topic) + "]: " + payload);
  delay(300);
}

void setColor(uint8_t red, uint8_t green, uint8_t blue)
{
  ledcWrite(1, red);
  ledcWrite(2, green);
  ledcWrite(3, blue);
}

void checkCode(int code)
{
  if (code == 0)
  {
    Serial.println("Code 0 : Correct input");
    setColor(0, 255, 0);
    tone(buzzer, 5000); // Send sound signal (1KHz = 1000)
    delay((500));
    noTone(buzzer);
    setColor(0, 0, 255);
    delay(10);
  }
  else if (code == 1)
  {
    Serial.println("Code 1 : Already used");
    setColor(255, 0, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(500);
    noTone(buzzer);
    setColor(0, 0, 255);
    delay(10);
  }
  else if (code == 2)
  {
    Serial.println("Code 2 : Not Found");
    setColor(255, 0, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(500);
    noTone(buzzer);
    setColor(0, 0, 255);
    delay(10);
  }
  else if (code == 3)
  {
    Serial.println("Code 3 : Incorrect input");
    setColor(255, 0, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(500);
    noTone(buzzer);
    setColor(0, 0, 255);
    delay(10);
  }
  else if (code == 4)
  {
    Serial.println("Code 4 : Unknown error");
    setColor(255, 0, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(500);
    noTone(buzzer);
    setColor(0, 0, 255);
    delay(10);
  }
}