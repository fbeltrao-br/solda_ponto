#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoOblique9pt7b.h>
#include <Wire.h>  // Enable I2C communication
// 1.3" I2C 12864 OLED screen module (Sino Wealth SH1106G driver) settings
const uint16_t SCREEN_WIDTH = 128, SCREEN_HEIGHT = 64;
#include <Adafruit_SH110X.h>
// KY-040 rotary encoder pins (S1, S2, KEY) or (CLK, DT, SW)
const uint8_t S1 = 2, S2 = 3, KEY = 4;
// RAMPS 1.4 mechanical endstop, Fotek 40A Solid State Relay, 3.3V 12mm active buzzer pins
const uint8_t TRIGGER = 5, SSR = 6, BUZZER = 7;  
const unsigned int FREQUENCY = 2500;
const unsigned long DURATION = 20;
// read_encoder() variables
uint32_t _lastIncReadTime = micros(), _lastDecReadTime = micros();
const uint16_t _pauseLength = 25000;
const uint8_t _fastIncrement = 3;
int16_t counter = 25;  // Starting value
const int16_t MIN_POWER = 10, MAX_POWER = 120;
// trigger() variables
uint8_t triggerState = digitalRead(TRIGGER);
bool blockTrigger = 0;
// edit() variables
uint8_t buttonState = digitalRead(KEY), lastButtonState = buttonState, editMode = 0;
// screen edit() mode variables
const uint32_t blinkInterval = 100UL;
uint32_t previousBlinkTimer = 0;
bool toggleBlink = 0;
// 32x32 warning icon
static const unsigned char PROGMEM warning_icon[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xc0,0x00,0x00,0x03,0xc0,0x00,
  0x00,0x0c,0x30,0x00,0x00,0x0c,0x30,0x00,0x00,0x0c,0x30,0x00,0x00,0x0c,0x30,0x00,
  0x00,0x30,0x0c,0x00,0x00,0x30,0x0c,0x00,0x00,0xc3,0xc3,0x00,0x00,0xc3,0xc3,0x00,
  0x00,0xc3,0xc3,0x00,0x00,0xc3,0xc3,0x00,0x03,0x03,0xc0,0xc0,0x03,0x03,0xc0,0xc0,
  0x03,0x03,0xc0,0xc0,0x03,0x03,0xc0,0xc0,0x0c,0x03,0xc0,0x30,0x0c,0x03,0xc0,0x30,
  0x30,0x00,0x00,0x0c,0x30,0x00,0x00,0x0c,0x30,0x03,0xc0,0x0c,0x30,0x03,0xc0,0x0c,
  0xc0,0x03,0xc0,0x03,0xc0,0x03,0xc0,0x03,0xc0,0x00,0x00,0x03,0xc0,0x00,0x00,0x03,
  0x3f,0xff,0xff,0xfc,0x3f,0xff,0xff,0xfc,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// Screen initialization
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT);

void setup() {
  attachInterrupt(digitalPinToInterrupt(S1), readEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(S2), readEncoder, CHANGE);
  pinMode(S1, INPUT_PULLUP);
  pinMode(S2, INPUT_PULLUP);
  pinMode(KEY, INPUT_PULLUP);
  pinMode(TRIGGER, INPUT_PULLUP);
  pinMode(SSR, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  delay(250);  // wait for the OLED to power up to avoid buffer corruption
  display.begin();
  display.clearDisplay();
  display.setContrast(0);
}

void loop() {
  shortDetection();
  drawScreen();
  pulsePin();
  editKey();
}

// Código do encoder KY-040 modificado. Original em: https://github.com/mo-thunderz/RotaryEncoder
void readEncoder() {
  if (blockTrigger == 1 || editMode == 0) return;

  // Encoder interrupt routine for both pins. Updates counter
  // if they are valid and have rotated a full indent
  static uint8_t old_AB = 3;                                                 // Lookup table index
  static int8_t encval = 0;                                                  // Encoder value
  static const int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  // Lookup table

  old_AB <<= 2;  // Remember previous state

  if (digitalRead(S1)) old_AB |= 0x02;  // Add current state of pin A
  if (digitalRead(S2)) old_AB |= 0x01;  // Add current state of pin B

  encval += enc_states[(old_AB & 0x0f)];

  // Update counter if encoder has rotated a full indent, that is at least 4 steps
  if (encval > 3) {  // Four steps forward
    int8_t changevalue = 1;
    if ((micros() - _lastIncReadTime) < _pauseLength) {
      changevalue = _fastIncrement * changevalue;
    }
    _lastIncReadTime = micros();
    counter = counter + changevalue;  // Update counter
    encval = 0;
    if (counter <= MAX_POWER) tone(BUZZER, FREQUENCY, DURATION);
  } else if (encval < -3) {  // Four steps backward
    int8_t changevalue = -1;
    if ((micros() - _lastDecReadTime) < _pauseLength) {
      changevalue = _fastIncrement * changevalue;
    }
    _lastDecReadTime = micros();
    counter = counter + changevalue;  // Update counter
    encval = 0;
    if (counter >= MIN_POWER) tone(BUZZER, FREQUENCY, DURATION);
  }
  counter = constrain(counter, MIN_POWER, MAX_POWER);
}

const void fullScreenWarning() {
  static const int16_t icon_w = 32, icon_h = 32;
  static const int16_t xpos = (SCREEN_WIDTH / 2) - (icon_w / 2);
  static const int16_t ypos = (SCREEN_HEIGHT / 2) - (icon_h / 2);
  display.clearDisplay();
  display.drawBitmap(xpos, ypos, warning_icon, 32, 32, 1);
  display.display();
}

void shortDetection() {
  while (digitalRead(TRIGGER) == LOW) {
    blockTrigger = 1;
    fullScreenWarning();
  }
  blockTrigger = 0;
}

void showCenteredText(int16_t ypos, const String &text) {
  int16_t x, y;
  uint16_t text_h, text_w;
  display.getTextBounds(text, 0, 0, &x, &y, &text_w, &text_h);
  int16_t xpos = (SCREEN_WIDTH - text_w) / 2;
  display.setCursor(xpos, ypos);
  display.println(text);
}

void drawScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setFont(&FreeMonoOblique9pt7b);
  showCenteredText(27, "PULSO");
  showCenteredText(46, String(counter) + "ms");
  if (editMode == 1) {
    uint32_t currentBlinkTimer = millis();
    if (currentBlinkTimer - previousBlinkTimer >= blinkInterval) {
      previousBlinkTimer = currentBlinkTimer;
      toggleBlink = !toggleBlink;
    }
    if (toggleBlink) {
      display.setFont();
      if (counter > MIN_POWER) display.drawChar(7, 38, '<', SH110X_WHITE, SH110X_BLACK, 1);
      if (counter < MAX_POWER) display.drawChar(115, 38, '>', SH110X_WHITE, SH110X_BLACK, 1);
    }
  }
  display.display();
}

void pulsePin() {
  if (blockTrigger == 1 || editMode == 1) return;
  triggerState = digitalRead(TRIGGER);
  if (triggerState == LOW) {
    fullScreenWarning();
    digitalWrite(SSR, HIGH);
    delay(counter);
    digitalWrite(SSR, LOW);
  }
}

void editKey() {
  if (blockTrigger == 1) return;
  buttonState = digitalRead(KEY);
  if (lastButtonState == 0 && buttonState == 1) editMode = !editMode ? 1 : 0;
  lastButtonState = buttonState;
}
