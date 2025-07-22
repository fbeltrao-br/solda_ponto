// Macro for direct port manipulation
#define dwon(port, pin) (port |= _BV(pin))
#define dwoff(port, pin) (port &= ~(_BV(pin)))
#include <Wire.h>  // Enable I2C communication
// 1.3" I2C 12864 OLED screen module (SH1106G) settings
const uint8_t i2c_Address = 0x3c;
const uint16_t SCREEN_WIDTH = 128;
const uint16_t SCREEN_HEIGHT = 64;
const int16_t OLED_RESET = -1;
#include <Adafruit_SH110X.h>
// KY-040 rotary encoder pins
const uint8_t ENC_A = 2;
const uint8_t ENC_B = 3;
const uint8_t ENC_KEY = 4;
// Other pins and configs
const uint8_t TRIGGER = 5;  // RAMPS 1.4 mechanical endstop
const uint8_t SSR = 6;  // Fotek 40A Solid State Relay
const uint8_t BUZZER = 7;  // 3.3V 12mm active buzzer
const unsigned int FREQUENCY = 2500;
const unsigned long DURATION = 20;
// read_encoder() variables
uint32_t _lastIncReadTime = micros();
uint32_t _lastDecReadTime = micros();
const uint16_t _pauseLength = 25000;
const uint8_t _fastIncrement = 3;
volatile int16_t counter = 25;  // Starting value
const int16_t MIN_POWER = 10;
const int16_t MAX_POWER = 100;
// trigger() variables
uint8_t triggerState = digitalRead(TRIGGER);
uint8_t lastTriggerState = triggerState;
bool blockTrigger = 0;
// edit() variables
uint8_t buttonState = digitalRead(ENC_KEY);
uint8_t lastButtonState = buttonState;
uint8_t editMode = 0;
// screen edit() mode variables
const uint32_t blinkInterval = 100;
uint32_t previousBlinkTimer = 0;
bool toggleBlink = 0;

// Screen initialization
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  attachInterrupt(digitalPinToInterrupt(ENC_A), read_encoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), read_encoder, CHANGE);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_KEY, INPUT_PULLUP);
  pinMode(SSR, OUTPUT);
  pinMode(TRIGGER, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  delay(250);  // wait for the OLED to power up to avoid buffer corruption
  display.begin(i2c_Address, true);
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
void read_encoder() {
  if (blockTrigger == 1 || editMode == 0) return;

  // Encoder interrupt routine for both pins. Updates counter
  // if they are valid and have rotated a full indent
  static uint8_t old_AB = 3;                                                 // Lookup table index
  static int8_t encval = 0;                                                  // Encoder value
  static const int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  // Lookup table

  old_AB <<= 2;  // Remember previous state

  if (digitalRead(ENC_A)) old_AB |= 0x02;  // Add current state of pin A
  if (digitalRead(ENC_B)) old_AB |= 0x01;  // Add current state of pin B

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

void showFullScreenText(const String &text) {
  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);
  uint16_t x, y;
  uint16_t text_h, text_w;
  display.getTextBounds(text, 0, 0, &x, &y, &text_w, &text_h);
  uint16_t xpos = (SCREEN_WIDTH / 2) - (text_w / 2);
  uint16_t ypos = (SCREEN_HEIGHT / 2) - (text_h / 2);
  display.setCursor(xpos, ypos);
  display.println(text);
  display.display();
}

void shortDetection() {
  while (digitalRead(TRIGGER) == LOW) {
    dwoff(PORTD, SSR);
    blockTrigger = 1;
    display.setTextSize(5);
    display.setTextColor(SH110X_WHITE);
    showFullScreenText("!");
  }
  blockTrigger = 0;
}

void showCenteredText(uint16_t ypos, const String &text) {
  uint16_t x, y;
  uint16_t text_h, text_w;
  display.getTextBounds(text, 0, 0, &x, &y, &text_w, &text_h);
  uint16_t xpos = (SCREEN_WIDTH - text_w) / 2;
  display.setCursor(xpos, ypos);
  display.println(text);
}

void drawScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  showCenteredText(10, String("PULSO"));
  display.setTextSize(3);
  display.setTextColor(SH110X_WHITE);
  showCenteredText(32, String(counter) + ("ms"));
  if (editMode == 1) {
    uint32_t currentBlinkTimer = millis();
    if (currentBlinkTimer - previousBlinkTimer >= blinkInterval) {
      previousBlinkTimer = currentBlinkTimer;
      toggleBlink = !toggleBlink;
      if (toggleBlink) {
        if (counter > MIN_POWER) {
          display.setTextSize(1);
          display.setTextColor(SH110X_WHITE);
          display.setCursor(7, 40);
          display.print("<");
        }
        if (counter < MAX_POWER) {
          display.setTextSize(1);
          display.setTextColor(SH110X_WHITE);
          display.setCursor(115, 40);
          display.print(">");
        }
      }
    }
  }
  display.display();
}

void pulsePin() {
  if (blockTrigger == 1 || editMode == 1) return;
  triggerState = digitalRead(TRIGGER);
  if (triggerState != lastTriggerState) {
    if (triggerState == LOW) {
      blockTrigger = 1;
      display.setTextSize(5);
      display.setTextColor(SH110X_WHITE);
      showFullScreenText("!");
      dwon(PORTD, SSR);
      delay(counter);
      dwoff(PORTD, SSR);
    } else {
      blockTrigger = 0;
      dwoff(PORTD, SSR);
    }
    lastTriggerState = triggerState;
  }
}

void editKey() {
  if (blockTrigger == 1) return;
  buttonState = digitalRead(ENC_KEY);
  if (lastButtonState == 0 && buttonState == 1) editMode = !editMode ? 1 : 0;
  lastButtonState = buttonState;
}
