#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <Credentials.h>

#define batteryPin 36 // ESP32 pin GIOP36
#define SS_PIN 32     // ESP32 pin GIOP32
#define RST_PIN 33    // ESP32 pin GIOP33
#define OBS 4

int avoid;
int TIMER = 600;       // SECONDS
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

const char *user_mail = portal_mail;
const char *user_password = portal_password;

const char *mqtt_server = mqtt_server_name;
const char *mqtt_username = mqtt_server_username;
const char *mqtt_password = mqtt_server_password;
const int mqtt_port = mqtt_server_port;
const char *CLIENT_ID = client_id;
const char *badger = badger_topic;
const char *api = api_topic;
const char *birthday = has_to_ring_topic;

RTC_DATA_ATTR int deep_sleep_counter = 0;

byte block = 6;
byte len;

WiFiClientSecure espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

#define C6 1047
#define D6 1175
#define E6 1319
#define F6 1397
#define G6 1568
#define A6 1760
#define B6 1976
#define C7 2093

void setup()
{
  Serial.begin(9600);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0); // 1 = High, 0 = Low
  ledcAttachPin(ledR, 1);
  ledcAttachPin(ledG, 2);
  ledcAttachPin(ledB, 3);

  ledcSetup(1, 12000, 8);
  ledcSetup(2, 12000, 8);
  ledcSetup(3, 12000, 8);

  pinMode(OBS, INPUT);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("-------------------- WiFi conneted -------------------");
  Serial.print("SSID : ");
  Serial.println(ssid);
  Serial.print("Password : ");
  Serial.println(password);

  String fullUrl = getReferer();
  if (fullUrl != "connected")
  {
    String magic = getMagic(fullUrl);

    bool check = checkIsRulesPage(fullUrl);
    if (check)
    {
      acceptRules(fullUrl, magic);
    }
    connectFortigatePortal(fullUrl, magic, user_mail, user_password);
  }
  else
  {
    Serial.println("");
    Serial.println("-------------------- Already login --------------------");
    Serial.print("Wifi :");
    Serial.println(ssid);
    Serial.print("Account :");
    Serial.println(portal_mail);
    Serial.println("");
  }

  SPI.begin();
  rfid.PCD_Init();

  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");

  pinMode(buzzer, OUTPUT);

  espClient.setInsecure(); // set security or not to wifi connect
  Serial.println("");
  Serial.println("------------------- MQTT connection -------------------");
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  while (!Serial)
  {
    delay(1);
  }
  if (avoid == 0)
  {
    deep_sleep_counter = 0;
  }
}

void loop()
{
  avoid = digitalRead(OBS); // lecture de la valeur du signal

  int batteryVoltage = analogRead(batteryPin);          // read the battery voltage
  float batteryLevel = batteryVoltage / 1024.0 * 100.0; // convert to percentage

  if (deep_sleep_counter > TIMER * 10)
  {
    Serial.println("Going to sleep");
    esp_deep_sleep_start();
    return;
  }

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  setColor(0, 0, 255);

  if (!rfid.PICC_IsNewCardPresent())
  {
    deep_sleep_counter++;
    delay(100);
    return;
  }

  if (!rfid.PICC_ReadCardSerial())
  {

    setColor(255, 0, 0);
    delay(1000);
    return;
  }

  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("**Card Detected:**"));
  deep_sleep_counter = 0;
  byte buffer1[18];
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
  else
  {
    setColor(255, 0, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(500);
    noTone(buzzer);
    setColor(0, 0, 255);
    delay(10);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void reconnect()
{
  while (!client.connected())
  {

    Serial.print("Attempting MQTT connection…");
    Serial.println(mqtt_server);
    setColor(0, 0, 255);
    delay(150);
    setColor(0, 0, 0);
    if (client.connect(CLIENT_ID, mqtt_username, mqtt_password))
    {
      client.subscribe(api);
      client.subscribe(birthday);
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed,");
      Serial.println(" try again..");
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  if (strcmp(topic, api) == 0)
  {
    callBackApiBadger(topic, payload, length);
  }
  else if (strcmp(topic, birthday) == 0)
  {
    callBackBirthday(topic, payload, length);
  }
}

void callBackApiBadger(char *topic, byte *payload, unsigned int length)
{
  String incommingMessage = "";
  for (int i = 0; i < length; i++)
    incommingMessage += (char)payload[i];

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, incommingMessage);
  int code = doc["code"];
  checkCode(code);
  boolean has_to_ring = doc["has_to_ring"];

  if (has_to_ring)
  {
    String msg = "birthday";
    String message = "{\"has_to_ring\":\"" + msg + "\"}";
    publishBirthday(birthday, message, true);
  }

  Serial.println("Message received [" + String(topic) + "]: " + incommingMessage);
  delay(300);
}

void callBackBirthday(char *topic, byte *payload, unsigned int length)
{
  String incommingMessage = "";
  for (int i = 0; i < length; i++)
    incommingMessage += (char)payload[i];

  Serial.println(incommingMessage);
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, incommingMessage);
  String has_to_ring = doc["has_to_ring"];
  Serial.println("Message received [" + String(topic) + "]: " + has_to_ring);
  birthdaySong();
}

void publishMessage(const char *topic, String payload, boolean retained)
{
  if (client.publish(topic, payload.c_str(), true))
    Serial.println("Message published [" + String(topic) + "]: " + payload);
  delay(300);
}

void publishBirthday(const char *topic, String payload, boolean retained)
{
  if (client.publish(topic, payload.c_str(), true))
    Serial.println("Message published [" + String(topic) + "]: " + payload);
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
    delay((250));
    noTone(buzzer);
    setColor(0, 0, 255);
    delay(10);
    return;
  }
  if (code == 1)
  {
    Serial.println("Code 1 : Already used");
    setColor(255, 0, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(250);
    noTone(buzzer);
    setColor(0, 0, 255);
    delay(10);
    return;
  }
  if (code == 2)
  {
    Serial.println("Code 2 : Not Found");
    setColor(255, 0, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(250);
    noTone(buzzer);
    setColor(0, 0, 255);
    delay(10);
    return;
  }
  if (code == 3)
  {
    Serial.println("Code 3 : Incorrect input");
    setColor(255, 0, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(250);
    noTone(buzzer);
    setColor(0, 0, 255);
    delay(10);
    return;
  }
  if (code == 4)
  {
    Serial.println("Code 4 : Unknown error");
    setColor(255, 0, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(250);
    noTone(buzzer);
    setColor(0, 0, 255);
    delay(10);
    return;
  }
}

String getReferer()
{
  HTTPClient http;
  http.begin("http://www.neverssl.com/");
  http.addHeader("Connection", "keep-alive");
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  String location;
  int httpCode = http.GET();

  if (httpCode > 0)
  {
    String location = http.getLocation();
    if (location != "")
    {
      Serial.println("location : " + location);
    }

    http.end();

    if (httpCode == 200)
    {
      return "connected";
    }
    Serial.println("-------------------- Get location --------------------");
    Serial.print("Code HTTP : ");
    Serial.println(httpCode);

    return location;
  }
  else
  {
    Serial.println("-------------------- Get location --------------------");
    Serial.print("Code HTTP : ");
    Serial.println(httpCode);
    Serial.println("Erreur dans l'envoi de la requête location");
    http.end();
    return "Error";
  }
}

String getMagic(String url)
{
  Serial.println("--------------------- Get magic ----------------------");
  int index = url.indexOf("?");
  String magic = url.substring(index + 1);
  Serial.println("magic : " + magic);
  return magic;
}

void acceptRules(String url, String magic)
{
  Serial.println("-------------------- Accept rules --------------------");
  HTTPClient http;
  http.begin(url);
  http.addHeader("Referer", url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Connection", "keep-alive");
  String payload = "magic=" + magic + "&answer=1";
  int httpCode = http.POST(payload);
  if (httpCode < 0)
  {
    Serial.println("Error accept rules: " + http.errorToString(httpCode));
  }
  Serial.print("Code HTTP : ");
  Serial.println(httpCode);
  Serial.println("Payload : " + payload);
}

bool checkIsRulesPage(String url)
{
  Serial.println("-------------- Check page is rules page --------------");
  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();
  Serial.print("Code HTTP : ");
  Serial.println(httpCode);
  if (httpCode > 0)
  {
    String response = http.getString();
    if (response.indexOf("Conditions de connexion à nos ressources réseau") != -1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    Serial.println("Error check rule: " + http.errorToString(httpCode));
    return false;
  }
}

void connectFortigatePortal(String fullUrl, String magic, String username, String password)
{
  Serial.println("----------------- Connect to portal ------------------");
  HTTPClient http;
  http.begin(fullUrl);
  http.GET();
  http.getString();
  http.addHeader("Referer", fullUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Connection", "keep-alive");

  String payload = "magic=" + magic + "&username=" + username + "&password=" + password;

  int httpCode = http.POST(payload);
  Serial.print("Code HTTP : ");
  Serial.println(httpCode);
  Serial.println("Payload : " + payload);
  if (httpCode < 0)
  {
    Serial.println("Error connection portal: " + http.errorToString(httpCode));
  }
  connectTo("http://www.neverssl.com/");

  http.end();
}

void connectTo(String url)
{
  HTTPClient http;
  http.begin(url);
  Serial.println("---------- Connect to " + url + " ------------");
  int httpCode = http.GET();
  Serial.print("Code HTTP : ");
  Serial.println(httpCode);
  if (httpCode == 200)
  {
    Serial.println("You are connecting");
  }
  else if (httpCode == 303)
  {
    Serial.print("You are not connecting");
  }
  else
  {
    Serial.println("Error " + url + " : " + http.errorToString(httpCode));
  }

  http.end();
}

void birthdaySong()
{
  Serial.println("Joyeux Anniversaire !");
  // delay(100);
  // tone(buzzer, 523); // Original frequency: 131
  // delay(250);
  // noTone(buzzer);
  // delay(125);
  // tone(buzzer, 523); // Original frequency: 131
  // delay(250);
  // tone(buzzer, 587); // Original frequency: 147
  // delay(500);
  // tone(buzzer, 523); // Original frequency: 131
  // delay(500);
  // tone(buzzer, 698); // Original frequency: 175
  // delay(500);
  // tone(buzzer, 659); // Original frequency: 165
  // delay(1000);
  // tone(buzzer, 523); // Original frequency: 131
  // delay(250);
  // noTone(buzzer);
  // delay(125);
  // tone(buzzer, 523); // Original frequency: 131
  // delay(250);
  // tone(buzzer, 587); // Original frequency: 147
  // delay(500);
  // tone(buzzer, 523); // Original frequency: 131
  // delay(500);
  // tone(buzzer, 784); // Original frequency: 196
  // delay(500);
  // tone(buzzer, 698); // Original frequency: 175
  // delay(1000);
  // tone(buzzer, 523); // Original frequency: 131
  // delay(250);
  // noTone(buzzer);
  // delay(125);
  // tone(buzzer, 523); // Original frequency: 131
  // delay(250);
  // tone(buzzer, 1046); // Original frequency: 262
  // delay(500);
  // tone(buzzer, 880); // Original frequency: 220
  // delay(500);
  // tone(buzzer, 698); // Original frequency: 175
  // delay(500);
  // tone(buzzer, 659); // Original frequency: 165
  // delay(500);
  // tone(buzzer, 587); // Original frequency: 147
  // delay(500);
  // tone(buzzer, 988); // Original frequency: 233
  // delay(250);
  // noTone(buzzer);
  // delay(125);
  // tone(buzzer, 988); // Original frequency: 233
  // delay(250);
  // tone(buzzer, 880); // Original frequency: 220
  // delay(500);
  // tone(buzzer, 698); // Original frequency: 175
  // delay(500);
  // tone(buzzer, 784); // Original frequency: 196
  // delay(500);
  // tone(buzzer, 698); // Original frequency: 175
  // delay(1000);
  // noTone(buzzer);
  // delay(100);
}