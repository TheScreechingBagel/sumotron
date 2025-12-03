#include "SPIFFS.h"
#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

// Motor control pins
#define AIN1 27
#define AIN2 14
#define BIN1 25 // TODO: change pin to 25 for the straight wiring
#define BIN2 33 // TODO: change pin to 33 for the straight wiring
#define PWMA 12
#define PWMB 32
#define STBY 26 // TODO: change to pin 26 for the straight wiring

/*
  The resolution of the PWM is 8 bit so the value is between 0-255
  We will set the speed between 100 to 255.
*/
enum speedSettings { SLOW = 100, NORMAL = 180, FAST = 255 };

class Car {
private:
  const int PWM_FREQ = 1000;    // 1kHz
  const int PWM_RESOLUTION = 8; // 8-bit (0-255)

  // holds the current speed settings, see values for SLOW, NORMAL, FAST
  speedSettings currentSpeedSettings;

public:
  Car() {
    // Initialize motor control pins
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT);
    pinMode(BIN2, OUTPUT);
    pinMode(PWMA, OUTPUT);
    pinMode(PWMB, OUTPUT);
    pinMode(STBY, OUTPUT);

    // Initialize PWM pins (standard analogWrite for ESP32)
    // ESP32 will use ledc behind the scenes automatically
    analogWrite(PWMA, 0);
    analogWrite(PWMB, 0);
    digitalWrite(STBY, HIGH);

    // Set PWM frequency for ESP32
    analogWriteFrequency(PWM_FREQ);
    analogWriteResolution(PWM_RESOLUTION);

    // Attach Pin to Channel
    //  ledcAttachPin(SPEED_CONTROL_PIN_1, channel_0);
    //  ledcAttachPin(SPEED_CONTROL_PIN_2, channel_1);

    // initialize default speed to SLOW
    setCurrentSpeed(speedSettings::NORMAL);
  }

  // Set left motor speed (-255 to 255)
  void setLeftMotor(int speed) {
    if (speed > 0) {
      digitalWrite(AIN1, HIGH);
      digitalWrite(AIN2, LOW);
      analogWrite(PWMA, speed);
    } else if (speed < 0) {
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
      analogWrite(PWMA, -speed);
    } else {
      digitalWrite(AIN1, HIGH);
      digitalWrite(AIN2, HIGH);
      analogWrite(PWMA, 0);
    }
  }

  // Set right motor speed (-255 to 255)
  void setRightMotor(int speed) {
    if (speed > 0) {
      digitalWrite(BIN1, HIGH);
      digitalWrite(BIN2, LOW);
      analogWrite(PWMB, speed);
    } else if (speed < 0) {
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, HIGH);
      analogWrite(PWMB, -speed);
    } else {
      digitalWrite(BIN1, HIGH);
      digitalWrite(BIN2, HIGH);
      analogWrite(PWMB, 0);
    }
  }

  // Turn the car left
  void turnLeft() {
    Serial.println("car is turning left...");
    setLeftMotor(-currentSpeedSettings);
    setRightMotor(currentSpeedSettings);
  }

  // Turn the car right
  void turnRight() {
    Serial.println("car is turning right...");
    setLeftMotor(currentSpeedSettings);
    setRightMotor(-currentSpeedSettings);
  }

  // Move the car forward
  void moveForward() {
    Serial.println("car is moving forward...");
    setLeftMotor(currentSpeedSettings);
    setRightMotor(currentSpeedSettings);
  }

  // Move the car backward
  void moveBackward() {
    Serial.println("car is moving backward...");
    setLeftMotor(-currentSpeedSettings);
    setRightMotor(-currentSpeedSettings);
  }

  // Stop the car
  void stop() {
    Serial.println("car is stopping...");
    setLeftMotor(0);
    setRightMotor(0);
  }

  // Set the motor speed (Legacy helper, not strictly needed but kept for
  // compatibility if needed internally)
  void setMotorSpeed() {
    // No-op, handled by individual moves now
  }

  // Set the current speed
  void setCurrentSpeed(speedSettings newSpeedSettings) {
    Serial.println("car is changing speed...");
    currentSpeedSettings = newSpeedSettings;
  }
  // Get the current speed
  speedSettings getCurrentSpeed() { return currentSpeedSettings; }
};

const char *ssid = "FBI_van";
const char *password = "security12345";

// AsyncWebserver runs on port 80 and the asyncwebsocket is initialize at this
// point also
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Our car object
Car car;

// Function to send commands to car
void sendCarCommand(const char *command) {
  // command could be either "left", "right", "forward" or "reverse" or "stop"
  // or speed settingg "slow-speed", "normal-speed", or "fast-speed"
  // or "M:left,right" for sumo mode
  if (strncmp(command, "M:", 2) == 0) {
    int left, right;
    if (sscanf(command + 2, "%d,%d", &left, &right) == 2) {
      car.setLeftMotor(left);
      car.setRightMotor(right);
    }
  } else if (strcmp(command, "left") == 0) {
    car.turnLeft();
  } else if (strcmp(command, "right") == 0) {
    car.turnRight();
  } else if (strcmp(command, "up") == 0) {
    car.moveForward();
  } else if (strcmp(command, "down") == 0) {
    car.moveBackward();
  } else if (strcmp(command, "stop") == 0) {
    car.stop();
  } else if (strcmp(command, "slow-speed") == 0) {
    car.setCurrentSpeed(speedSettings::SLOW);
  } else if (strcmp(command, "normal-speed") == 0) {
    car.setCurrentSpeed(speedSettings::NORMAL);
  } else if (strcmp(command, "fast-speed") == 0) {
    car.setCurrentSpeed(speedSettings::FAST);
  }
}

// Processor for index.html page template.  This sets the radio button to
// checked or unchecked
String indexPageProcessor(const String &var) {
  String status = "";
  if (var == "SPEED_SLOW_STATUS") {
    if (car.getCurrentSpeed() == speedSettings::SLOW) {
      status = "checked";
    }
  } else if (var == "SPEED_NORMAL_STATUS") {
    if (car.getCurrentSpeed() == speedSettings::NORMAL) {
      status = "checked";
    }
  } else if (var == "SPEED_FAST_STATUS") {
    if (car.getCurrentSpeed() == speedSettings::FAST) {
      status = "checked";
    }
  }
  return status;
}

// Callback function that receives messages from websocket client
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
  case WS_EVT_CONNECT:
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    break;

  case WS_EVT_DISCONNECT:
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
    break;

  case WS_EVT_DATA: {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len &&
        info->opcode == WS_TEXT) {
      // Create a null-terminated copy safely
      char msg[len + 1];
      memcpy(msg, data, len);
      msg[len] = '\0';
      sendCarCommand(msg);
    }
    break;
  }

  case WS_EVT_PONG:
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len,
                  (len) ? (char *)data : "");
    break;

  case WS_EVT_ERROR:
    // Serial.printf("ws[%s][%u] error\n", server->url(), client->id());
    break;
  }
}

// Function called when resource is not found on the server
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup() {
  // Initialize the serial monitor baud rate
  Serial.begin(115200);
  Serial.println("bagel :c");

  WiFi.mode(WIFI_AP);
  bool apStarted = WiFi.softAP(ssid, password);
  if (apStarted) {
    Serial.println("Hotspot up!");
    Serial.print("IP Address: ");
    Serial.print("WOKE");
    Serial.println(WiFi.softAPIP().toString());
  } else {
    Serial.println("Failed to create hotspot.");
    return;
  }

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Add callback function to websocket server
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Requesting index page...");
    request->send(SPIFFS, "/index.html", "text/html", false,
                  indexPageProcessor);
  });

  // Route to load entireframework.min.css file
  server.on("/css/entireframework.min.css", HTTP_GET,
            [](AsyncWebServerRequest *request) {
              request->send(SPIFFS, "/css/entireframework.min.css", "text/css");
            });

  // Route to load custom.css file
  server.on("/css/custom.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/css/custom.css", "text/css");
  });

  // Route to load custom.js file
  server.on("/js/custom.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/custom.js", "text/javascript");
  });

  // On Not Found
  server.onNotFound(notFound);

  // Start server
  server.begin();
}

void loop() {
  // No code in here.  Server is running in asynchronous mode
}
