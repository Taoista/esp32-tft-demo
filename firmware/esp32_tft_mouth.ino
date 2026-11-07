#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// GMT020-02-8P (ST7789V, 240x320), matching the module wiring diagram:
// SCK=18, MOSI=23, CS=5, DC=19, RST=22.
constexpr int TFT_CS = 5;
constexpr int TFT_DC = 19;
constexpr int TFT_RST = 22;
constexpr uint16_t BG = ST77XX_BLACK;
constexpr uint16_t LIP = 0xF98C;    // warm pink
constexpr uint16_t TONGUE = 0xF986;

Adafruit_ST7789 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);
uint8_t mouthLevel = 0;
char serialLine[16] = {};
uint8_t serialLength = 0;
constexpr int IMAGE_ROWS_PER_CHUNK = 8;
constexpr int IMAGE_MAX_WIDTH = 320;
uint16_t imageChunk[IMAGE_MAX_WIDTH * IMAGE_ROWS_PER_CHUNK];
bool imageMode = false;

void drawClosedMouth(int cx, int cy) {
  // A relaxed, closed mouth: a short horizontal slit with slightly raised ends.
  for (int x = cx - 55; x <= cx + 55; ++x) {
    float u = (x - cx) / 55.0f;
    int y = cy + (int)(8.0f * u * u);
    tft.drawPixel(x, y, LIP);
    tft.drawPixel(x, y + 1, LIP);
    tft.drawPixel(x, y + 2, LIP);
  }
}

void drawTalkingMouth(uint8_t level, int cx, int cy) {
  const int rx = 55;
  const int ry = 7 + (int)(level * 0.16f);

  tft.fillEllipse(cx, cy, rx, ry, LIP);
  tft.fillEllipse(cx, cy, rx - 4, max(2, ry - 4), 0x1805); // dark mouth interior
  tft.drawEllipse(cx, cy, rx, ry, LIP);

  if (level > 45 && ry > 12) {
    int toothHeight = min(12, ry / 2);
    tft.fillRoundRect(cx - 31, cy - ry + 4, 62, toothHeight, 3, ST77XX_WHITE);
    int tongueHeight = min(10, ry / 3);
    tft.fillRoundRect(cx - 18, cy + ry - tongueHeight - 4, 36, tongueHeight, 5, TONGUE);
  }
}

void drawMouth() {
  const int cx = tft.width() / 2;
  const int cy = tft.height() / 2;
  // ST7789 retains pixels, so clear only the mouth bounds instead of
  // repainting the whole panel on every voice update.
  tft.fillRect(cx - 60, cy - 55, 120, 110, BG);
  if (mouthLevel < 12) drawClosedMouth(cx, cy);
  else drawTalkingMouth(mouthLevel, cx, cy);
}

void readLevels() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialLength) {
        serialLine[serialLength] = '\0';
        if (strncmp(serialLine, "IMG ", 4) == 0) {
          int rotation = constrain(atoi(serialLine + 4), 0, 1);
          tft.setRotation(rotation);
          imageMode = true;
          int width = tft.width();
          int height = tft.height();
          Serial.println("IMG_READY");
          bool complete = true;
          for (int y = 0; complete && y < height; y += IMAGE_ROWS_PER_CHUNK) {
            int rows = min(IMAGE_ROWS_PER_CHUNK, height - y);
            size_t chunkBytes = (size_t)width * rows * sizeof(uint16_t);
            size_t received = Serial.readBytes((uint8_t *)imageChunk, chunkBytes);
            if (received != chunkBytes) {
              complete = false;
              break;
            }
            tft.drawRGBBitmap(0, y, imageChunk, width, rows);
            Serial.printf("ACK %d\n", y + rows);
          }
          if (complete) {
            Serial.println("DONE");
          } else {
            Serial.println("IMAGE_ERROR");
            imageMode = false;
          }
        } else if (strcmp(serialLine, "PING") == 0) {
          Serial.println("PONG");
        } else if (strcmp(serialLine, "TEST") == 0) {
          int halfWidth = tft.width() / 2;
          int halfHeight = tft.height() / 2;
          tft.fillRect(0, 0, halfWidth, halfHeight, ST77XX_RED);
          tft.fillRect(halfWidth, 0, tft.width() - halfWidth, halfHeight, ST77XX_GREEN);
          tft.fillRect(0, halfHeight, halfWidth, tft.height() - halfHeight, ST77XX_BLUE);
          tft.fillRect(halfWidth, halfHeight, tft.width() - halfWidth,
                       tft.height() - halfHeight, ST77XX_WHITE);
          imageMode = true;
          Serial.println("TEST_OK");
        } else if (strcmp(serialLine, "MOUTH") == 0) {
          imageMode = false;
          mouthLevel = 0;
          tft.fillScreen(BG);
          drawMouth();
        } else if (strcmp(serialLine, "ROT0") == 0 || strcmp(serialLine, "ROT1") == 0) {
          tft.setRotation(serialLine[3] - '0');
          if (!imageMode) {
            tft.fillScreen(BG);
            drawMouth();
          }
          Serial.println("ROT_OK");
        } else {
          uint8_t nextLevel = (uint8_t)constrain(atoi(serialLine), 0, 255);
          bool crossedRestThreshold = (nextLevel < 12) != (mouthLevel < 12);
          int levelDelta = abs((int)nextLevel - (int)mouthLevel);
          if (!imageMode && (crossedRestThreshold || levelDelta >= 5)) {
            mouthLevel = nextLevel;
            drawMouth();
          } else {
            mouthLevel = nextLevel;
          }
        }
        serialLength = 0;
      }
    } else if (c >= 32 && c <= 126 && serialLength < sizeof(serialLine) - 1) {
      serialLine[serialLength++] = c;
    } else {
      serialLength = 0;
    }
  }
}

void setup() {
  Serial.begin(460800);
  Serial.setTimeout(10000);
  SPI.begin(18, -1, 23, TFT_CS);
  tft.init(240, 320);
  tft.setRotation(0);
  tft.fillScreen(BG);
  drawMouth();
  Serial.println("READY");
}

void loop() {
  readLevels();
}
