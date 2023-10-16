#include <SFE_BMP180.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

SFE_BMP180 sensor;
LiquidCrystal LCD(2, 13, 14, 0, 26, 25); //[RS,EN,D4,D5,D6,D7]

#define PERTH_ALTITUDE 0
//deployment location altitude in meters

const int wifiSets = 2;
const char* ssid[wifiSets] = {"oi"};
const char* password[wifiSets] = {"okokokok"};
const char* serverUrl = "http://192.168.130.71:8000";
WiFiServer server(80);
bool online = false;
int frameCount = 0;

unsigned long thisTick, lastTick;

enum OverrideState {
  OFF,
  ON_OPEN,
  ON_CLOSE
};

OverrideState override = OFF;

// Data
struct Data {
  double temperature;
  double pressure;
};

Data* reservoir;
const int dataMax = 100000 / sizeof(Data); // esp32 has 160k heap memory
int writeIndex = 0;

bool openWindow(double temperature, double pressure) {
  if (override == ON_OPEN) {
    return true;
  } else {
    return false;
  }
}

void connect_wifi() {
  for (int i = 0; i < wifiSets; ++i) {
    WiFi.begin(ssid[i], password[i]);
    Serial.print("Connecting to WiFi..");
    for (int j = 0; j < 10; ++j) {
      Serial.print('.');
      if (WiFi.status() != WL_CONNECTED ) {
        online = true;
        delay(1000);
        break;
      }
      delay(1000);
    }
    if (online) {
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid[i]);
      Serial.print("IP:");
      Serial.print(WiFi.localIP());
      Serial.print(", MAC:");
      Serial.println(WiFi.macAddress());
      break;
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Reboot");

  // data init
  reservoir = (Data*)malloc(dataMax * sizeof(Data));
  writeIndex = 0;

  // sensor init
  if (sensor.begin()) {
    Serial.println("BMP180 init success");
  } else {
    Serial.println("BMP180 init failed\n\n");
    while (1); // pause forever, copied this from bmp180 example, may not be desirable
  }

  // Initialize LCD
  LCD.begin(16, 2);

  // Wifi section
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("scan wifi:");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.print(n);
      Serial.print(":");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
    }
    connect_wifi();
  }

  server.begin();

  if (!online) {
    Serial.println("Operating in offline mode");
  }

  // Time
  lastTick = millis();
}

int send_data(Data data) {
  // Send data to the server
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  char tempStr[6];
  dtostrf(data.temperature, 5, 2, tempStr);
  char presStr[8];
  dtostrf(data.pressure, 7, 2, presStr);
  String sendString = String(tempStr) + " " + String(presStr);
  Serial.print("Data:");
  Serial.println(sendString);
  int httpCode = http.POST(sendString);
  String payload = http.getString();
  Serial.print("http code:");
  Serial.println(httpCode);
  Serial.print("payload:");
  Serial.println(payload);
  http.end();
  return httpCode;
}

void push_reservoir(Data data) {
  if (writeIndex == dataMax) {
    memmove(&reservoir[0], &reservoir[1], (dataMax - 1) * sizeof(Data));
    --writeIndex;
  }
  reservoir[writeIndex] = data;
  ++writeIndex;
  Serial.print("Data pushed to reservoir, capacity: [");
  Serial.print(writeIndex);
  Serial.print("/");
  Serial.print(dataMax);
  Serial.println("]");
}


void loop() {
  ++frameCount;
  if (frameCount > 5000 * 30) {
    frameCount = 0;
    // if disconnected, attempt reconnect
    thisTick = millis();
    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
      online = false;
      Serial.println("wifi not connected");
    } else {
      online = true;
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(WiFi.SSID());
      Serial.print("IP:");
      Serial.print(WiFi.localIP());
      Serial.print(", MAC:");
      Serial.println(WiFi.macAddress());
    }

    int reconnectInterval = 60000; // 1min = 60s = 60,000ms
    if (!online && (thisTick - lastTick > reconnectInterval)) {
      connect_wifi();
      lastTick = thisTick;
    }

    Serial.print("provided altitude: ");
    Serial.print(PERTH_ALTITUDE, 0);
    Serial.println(" meters");
    LCD.clear();

    Data data;

    // if startTemperature() successful, number of ms to wait is returned
    // otherwise 0 is returned
    char tempQueryReturn = sensor.startTemperature();
    if (tempQueryReturn == 0) {
      Serial.println("startTemperature failed and returned 0");
      LCD.print("N/A C, ");
    } else {
      delay(tempQueryReturn);

      // temperature measurement
      tempQueryReturn = sensor.getTemperature(data.temperature);
      if (tempQueryReturn != 0) {
        double fahrenheit = (9.0 / 5.0) * data.temperature + 32.0;
        Serial.print("temperature: ");
        Serial.print(data.temperature, 2);
        Serial.print(" deg C, ");
        Serial.print(fahrenheit, 2);
        Serial.println(" deg F");

        LCD.print(data.temperature, 2);
        LCD.print("C,");
      }
    }

    char presQueryReturn = sensor.startPressure(3);
    if (presQueryReturn == 0) {
      printf("startPressure failed and returned 0");
      LCD.print("N/A mb");
    } else {
      delay(presQueryReturn);

      // this function requires temperature to calculate pressure
      presQueryReturn = sensor.getPressure(data.pressure, data.temperature);
      if (presQueryReturn == 0) {
        Serial.println("getPressure failed and returned 0");
      } else {
        double inHg = data.pressure * 0.0295333727;
        Serial.print("absolute pressure: ");
        Serial.print(data.pressure, 2);
        Serial.print(" mb, ");
        Serial.print(inHg, 2);
        Serial.println(" inHg");

        LCD.print(data.pressure, 2);
        LCD.print("mb");

        LCD.setCursor(0, 1);
        if (openWindow(data.temperature, data.pressure)) {
          LCD.print("Window Opened");
        } else {
          LCD.print("Window Closed");
        }
      }
    }

    if (online) {
      Serial.println("wifi connected, sending data...");
      if (writeIndex != 0) {
        Serial.print("Dumping ");
        Serial.print(writeIndex);
        Serial.println(" items from reservoir");
        while (writeIndex != 0) {
          --writeIndex;
          if (send_data(reservoir[writeIndex]) < 0) {
            Serial.println("Dump failed, cancel dump, pushing data to reservoir");
            ++writeIndex;
            push_reservoir(data);
            break;
          }
        }
        Serial.println("Reservoir dumped");
      }
      if (send_data(data) < 0) {
        Serial.println("Send data failed, pushing data to reservoir");
        push_reservoir(data);
      }
    } else {
      Serial.println("wifi not connected, pushing data to reservoir");
      push_reservoir(data);
    }
  } else if (online) {
      WiFiClient client = server.available();
      if (client) {
        Serial.println("Client Connected");
        while (client.connected()) {
          if (client.available()) {
            String request = client.readStringUntil('\r');
            if (request.startsWith("SET_OVERRIDE_OFF")) {
              override = OFF;
              Serial.println("Override set to OFF");
            } else if (request.startsWith("SET_OVERRIDE_OPEN")) {
              override = ON_OPEN;
              Serial.println("Override set to ON_OPEN");
            } else if (request.startsWith("SET_OVERRIDE_CLOSE")) {
              override = ON_CLOSE;
              Serial.println("Override set to ON_CLOSE");
            }
          }
        }

        client.stop();
        Serial.println("Client disconnected");
      }
  }
}
