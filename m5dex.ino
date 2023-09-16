#include <vector>

// M5Stick support
#include <M5StickCPlus.h>

// Bluetooth support
#include <BLEDevice.h>
#include <BLEServer.h>
// #include <BLEUtils.h> // Seems to be unnecessary
#include <BLE2902.h>

#define BLE_SERVICE_NAME     "M5DEX"
#define SERVICE_UUID         "ad5eac61-ba3c-45cb-9d32-ca512d71fba4"
#define CHARACTERISTIC_UUID  "246e55b0-b515-412c-984c-c3bf2fe76553"

BLEServer *pServer = NULL;
BLECharacteristic * pCharacteristic = NULL;

bool deviceJustConnected = false;
bool deviceConnected = false;
bool shouldDisplayConnectScreen = true;

int timeAwake = 0;
int timeSinceLastRender = 0;

int maxVal = 300;
// int hyper = 250;
int inRangeUpper = 180;
int inRangeLower = 70;

int screenBrightness = 20;

int currentBg = 100;
std::string timeStr = "";
std::string minsAgo = "";
int velocity;
std::vector<int> bgValues = {100}; // {184, 192, 190, 202, 187, 170, 155, 130, 110, 90, 75, 66, 60, 70, 72, 82, 94, 107, 128};

int displayOn = true;
int screenRotation = 3; // 1 button on right, 3 button on left

void renderScreen() {
  float leftPos = 20;
  float rightPos = 206;

  M5.Lcd.fillScreen(BLACK);
  // M5.Lcd.clear(BLACK); // doesn't work

  M5.Lcd.setRotation(screenRotation);
  // M5.Lcd.setTextWrap(false);

  if (shouldDisplayConnectScreen) {
    M5.Lcd.setTextDatum(CC_DATUM);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.drawString("Connect via", 120, 60);
    M5.Lcd.drawString("Bluetooth to M5DEX", 120, 80);
    return;
  }

  // Display current BG value
  int currentBgInt = static_cast<int>(currentBg);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(getColorFor(currentBgInt));
  M5.Lcd.drawString(std::to_string(currentBgInt).c_str(), 20, 16);

  // Display time since last update
  M5.Lcd.setTextDatum(TR_DATUM);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_DARKGREY);
  M5.Lcd.drawString(minsAgo.c_str(), 206, 21);

  // Display chart y axis labels
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_LIGHTGREY);
  M5.Lcd.drawString("300", 212, scaleToScreen(maxVal) - 3);
  M5.Lcd.drawString("180", 212, scaleToScreen(inRangeUpper) - 3);
  M5.Lcd.drawString("70", 212, scaleToScreen(inRangeLower) - 3);

  // Draw upper line (300)
  M5.Lcd.drawLine(leftPos, scaleToScreen(maxVal), rightPos, scaleToScreen(maxVal), TFT_DARKGREY);

  // 180
  M5.Lcd.drawLine(leftPos, scaleToScreen(inRangeUpper), rightPos, scaleToScreen(inRangeUpper), ORANGE);

  // Draw lower line
  M5.Lcd.drawLine(leftPos, scaleToScreen(inRangeLower), rightPos, scaleToScreen(inRangeLower), RED);

  int i = leftPos + 1;
  for (int bgValue : bgValues) {
    int color = getColorFor(bgValue);
    if (bgValue >= 40) {
      M5.Lcd.fillCircle(i, scaleToScreen(bgValue), 2, color);
    }
    i += 10;
  }

  drawArrow();
}

int getColorFor(int value) {
  return value > inRangeUpper ? ORANGE : value < inRangeLower ? RED : WHITE;
}

void drawArrow() {
  int topOffset = 20;
  int leftOffset = 88;
  if (velocity > 16) {
    // UP ARROW
    M5.Lcd.drawLine(leftOffset, topOffset, leftOffset, topOffset + 12, WHITE);
    M5.Lcd.drawLine(leftOffset + 1, topOffset, leftOffset + 1, topOffset + 12, WHITE);

    M5.Lcd.drawLine(leftOffset + 1, topOffset, leftOffset + 6, topOffset + 5, WHITE);
    M5.Lcd.drawLine(leftOffset + 2, topOffset, leftOffset + 7, topOffset + 5, WHITE);

    M5.Lcd.drawLine(leftOffset, topOffset, leftOffset - 5, topOffset + 5, WHITE);
    M5.Lcd.drawLine(leftOffset - 1, topOffset, leftOffset - 6, topOffset + 5, WHITE);
  } else if (velocity > 8) {
    // UP AND RIGHT
    M5.Lcd.drawLine(leftOffset - 6, topOffset + 11, leftOffset + 6, topOffset, WHITE);
    M5.Lcd.drawLine(leftOffset - 6, topOffset + 12, leftOffset + 6, topOffset + 1, WHITE);

  } else if (velocity > -8) {
    // ACROSS
    M5.Lcd.drawLine(leftOffset - 6, topOffset + 6, leftOffset + 6, topOffset + 6, WHITE);
    M5.Lcd.drawLine(leftOffset - 6, topOffset + 7, leftOffset + 6, topOffset + 7, WHITE);

    M5.Lcd.drawLine(leftOffset, topOffset, leftOffset + 6, topOffset + 6, WHITE);
    M5.Lcd.drawLine(leftOffset, topOffset + 1, leftOffset + 6, topOffset + 7, WHITE);

    M5.Lcd.drawLine(leftOffset, topOffset + 12, leftOffset + 6, topOffset + 6, WHITE);
    M5.Lcd.drawLine(leftOffset, topOffset + 13, leftOffset + 6, topOffset + 7, WHITE);
  } else if (velocity > -16) {
    // DOWN AND RIGHT
    M5.Lcd.drawLine(leftOffset - 6, topOffset, leftOffset + 6, topOffset + 11, WHITE);
    M5.Lcd.drawLine(leftOffset - 6, topOffset + 1, leftOffset + 6, topOffset + 12, WHITE);
  } else {
    // DOWN
    M5.Lcd.drawLine(leftOffset, topOffset, leftOffset, topOffset + 12, WHITE);
    M5.Lcd.drawLine(leftOffset + 1, topOffset, leftOffset + 1, topOffset + 12, WHITE);

    M5.Lcd.drawLine(leftOffset + 1, topOffset + 12, leftOffset + 6, topOffset + 7, WHITE);
    M5.Lcd.drawLine(leftOffset + 2, topOffset + 12, leftOffset + 7, topOffset + 7, WHITE);

    M5.Lcd.drawLine(leftOffset, topOffset + 12, leftOffset - 5, topOffset + 7, WHITE);
    M5.Lcd.drawLine(leftOffset - 1, topOffset + 12, leftOffset - 6, topOffset + 7, WHITE);
  }
}

float scaleToScreen(float value) {
  float scaled = scaleLinear(value, 40, maxVal, 130, 48);
  if (scaled > maxVal) {
    return maxVal;
  }

  return scaled;
}

// scaleLinear(5, 0, 10, 0, 300) === 150
float scaleLinear(float d, float inMin, float inMax, float outMin, float outMax) {
  float scaledIn = (d - inMin) / (inMax - inMin);
  float scaledOut = (scaledIn * (outMax - outMin)) + outMin;

  return scaledOut;
}

void splitStringIntoInts(const std::string& str, std::vector<int>& result, char delimiter = ',') {
  size_t start = 0;
  size_t end = str.find(delimiter);

  while (end != std::string::npos) {
    result.push_back(std::stoi(str.substr(start, end - start)));
    start = end + 1;
    end = str.find(delimiter, start);
  }

  result.push_back(std::stoi(str.substr(start, end)));
}

void splitStringIntoStrings(const std::string& str, std::vector<std::string>& result, char delimiter = ',') {
  size_t start = 0;
  size_t end = str.find(delimiter);

  while (end != std::string::npos) {
    result.push_back(str.substr(start, end - start));
    start = end + 1;
    end = str.find(delimiter, start);
  }

  result.push_back(str.substr(start, end));
}

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("onConnect");
    deviceJustConnected = true;
    deviceConnected = true;
    shouldDisplayConnectScreen = false;

    // No need to display screen yet - that will happen on bluetooth data read
    // renderScreen();
    // pCharacteristic->setValue("Hello from ESP32!");
    // pCharacteristic->notify();
  };

  void onDisconnect(BLEServer* pServer) {
    Serial.println("onDisconnect");
    deviceConnected = false;
    pServer->startAdvertising();
    // renderScreen();
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
      Serial.println("Received Data: ");

      for (int i = 0; i < value.length(); i++) {
        Serial.print(value[i]);
      }

      Serial.println();

      std::vector<std::string> parts;
      splitStringIntoStrings(value, parts, '|');

      // [time, velocity, formattedMinsAgo, bgData.join(',')].join('|')

      timeStr = parts[0];
      velocity = std::stoi(parts[1]);
      minsAgo = parts[2];

      std::vector<int> tempValues;
      splitStringIntoInts(parts[3], tempValues);

      // Find the last non-zero value in the vector and set it to currentBg
      for (auto it = tempValues.rbegin(); it != tempValues.rend(); ++it) {
        if(*it != 0) {
          currentBg = *it;
          break;
        }
      }

      bgValues = tempValues;
      renderScreen();

      // delay(500);
      // velocity = 25;
      // renderScreen();
      // delay(500);
      // velocity = 8;
      // renderScreen();
      // delay(500);
      // velocity = 0;
      // renderScreen();
      // delay(500);
      // velocity = -8;
      // renderScreen();
      // delay(500);
      // velocity = -25;
      // renderScreen();
    }
  }
};

void setupBLE() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  BLEDevice::init(BLE_SERVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  // BLEAdvertising *pAdvertising = pServer->getAdvertising();
  // pAdvertising->start();
  pServer->startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

// void playSleepSong() {
//   M5.Beep.tone(400);
//   delay(180);
//   M5.Beep.tone(200);
//   delay(180);
//   M5.Beep.tone(100);
//   delay(180);
//   M5.Beep.mute();
// }

// void playWakeSong() {
//   M5.Beep.tone(100);
//   delay(180);
//   M5.Beep.tone(200);
//   delay(180);
//   M5.Beep.tone(400);
//   delay(180);
//   M5.Beep.mute();
// }

void fadeIn() {
  displayOn = true;
  M5.Axp.WakeUpDisplayAfterLightSleep();
  M5.Axp.ScreenBreath(4);
  delay(25);
  M5.Axp.ScreenBreath(8);
  delay(25);
  M5.Axp.ScreenBreath(12);
  delay(25);
  M5.Axp.ScreenBreath(16);
  delay(25);
  M5.Axp.ScreenBreath(20);
  timeAwake = 0;
}

void fadeOut() {
  displayOn = false;
  M5.Axp.ScreenBreath(16);
  delay(25);
  M5.Axp.ScreenBreath(12);
  delay(25);
  M5.Axp.ScreenBreath(8);
  delay(25);
  M5.Axp.ScreenBreath(4);
  delay(25);
  M5.Axp.SetSleep();
  // M5.Axp.LightSleep(3000); // this doesn't seem to work.
}

void setup() {
  M5.begin();
  M5.Axp.ScreenBreath(screenBrightness);
  renderScreen();
  setupBLE();
}

void loop() {
  M5.update();

  // Toggle display when pressing the A button:
  if (M5.BtnA.wasReleased()) {
    if (displayOn == true) {
      fadeOut();
      // playSleepSong();
    } else if (displayOn == false) {
      fadeIn();
      // playWakeSong();
    }
  }

  // Switch screen rotation when pressing the B button:
  if (M5.BtnB.wasReleased()) {
    screenRotation = screenRotation == 1 ? 3 : 1;
    renderScreen();
  }

  delay(100); // TODO: determine optimal delay time

  // TODO: use millis() instead of delay() to avoid blocking the loop
  timeAwake += 100;
  timeSinceLastRender += 100;

  if (timeAwake > (1000 * 15) && displayOn == true) {
    fadeOut();
  }

  // Re-render the screen once a minute
  if (timeSinceLastRender > (1000 * 60 * 1)) {
    renderScreen();
    timeSinceLastRender = 0;
  }
}
