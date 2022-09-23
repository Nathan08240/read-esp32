#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

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

const char *ssid = "iPhone de Nathan";
const char *password = "Nathan51100";

// 0e1222c02scfl7
const char *mqtt_server = "9ee6fa03f5754817a1ead63bf198898a.s1.eu.hivemq.cloud"; // replace with your broker url
const char *mqtt_username = "nolah";
const char *mqtt_password = "#jz6DMAFn*XAr,$rW;P9";
const char *CLIENT_ID = "badger_2";
const int mqtt_port = 8883;

WiFiClientSecure espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

const char *badger = "badger/cesi/reims/2";
const char *api = "api/cesi/reims/2";

static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

void setup()
{

  Serial.begin(9600);
  delay(10);

  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());

  SPI.begin();
  rfid.PCD_Init();

  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");

  pinMode(buzzer, OUTPUT);

  ledcAttachPin(ledR, 1);
  ledcAttachPin(ledG, 2);
  ledcAttachPin(ledB, 3);

  ledcSetup(1, 12000, 8);
  ledcSetup(2, 12000, 8);
  ledcSetup(3, 12000, 8);

  while (!Serial)
    delay(1);

  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop()
{

  if (!client.connected())
    reconnect();
  client.loop();

  ledcWrite(1, 0);
  ledcWrite(2, 0);
  ledcWrite(3, 255);
  delay(1);

  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;
  byte block;
  byte len;
  if (!rfid.PICC_IsNewCardPresent())
  {
    return;
  }
  if (!rfid.PICC_ReadCardSerial())
  {
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

    ledcWrite(1, 255);
    ledcWrite(2, 0);
    ledcWrite(3, 0);
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

  Serial.println(header);
  Serial.println(student_id);

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
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(CLIENT_ID, mqtt_username, mqtt_password))
    {
      Serial.println("connected");

      client.subscribe(api);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
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

void checkCode(int code)
{
  if (code == 0)
  {
    Serial.println("Correct input");
    ledcWrite(1, 0);
    ledcWrite(2, 255);
    ledcWrite(3, 0);
    tone(buzzer, 5000); // Send sound signal (1KHz = 1000)
    delay(200);
    noTone(buzzer);
    delay(10);
  }
  else if (code == 1)
  {
    Serial.println("Already used");
    ledcWrite(1, 255);
    ledcWrite(2, 255);
    ledcWrite(3, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(500);
    noTone(buzzer);
    delay(10);
  }
  else if (code == 2)
  {
    Serial.println("Not Found");
    ledcWrite(1, 255);
    ledcWrite(2, 0);
    ledcWrite(3, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(500);
    noTone(buzzer);
    delay(10);
  }
  else if (code == 3)
  {
    Serial.println("Incorrect input");
    ledcWrite(1, 255);
    ledcWrite(2, 0);
    ledcWrite(3, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(500);
    noTone(buzzer);
    delay(10);
  }
  else if (code == 4)
  {
    Serial.println("Unknown error");
    ledcWrite(1, 255);
    ledcWrite(2, 0);
    ledcWrite(3, 0);
    tone(buzzer, 500); // Send sound signal (1KHz = 1000)
    delay(500);
    noTone(buzzer);
    delay(10);
  }
}