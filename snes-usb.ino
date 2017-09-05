// NES/SNES controller to USB adapter
// based on info from:
// https://tresi.github.io/nes/
// https://www.gamefaqs.com/snes/916396-super-nintendo/faqs/5395

// pinout for my extension cable:
// (starting from rectangular side)
// 
// 1: green (5v)
// 2: yellow (clock)
// 3: white (latch)
// 4: red (data)
// 7: black (ground)

#include <HID-Project.h>

const int LATCH_PIN = 2; // orange
const int CLOCK_PIN = 3; // red
const int DATA_PIN = 4; // yellow

// how often should we poll the controller (16ms = 60Hz)
const unsigned long POLL_INTERVAL_MS = 16;

// raw button data
const int BUTTON_COUNT = 16;
bool buttonPressed[BUTTON_COUNT];

// NES buttons in data order are:
// A, B, Select, Start, Up, Down, Left, Right
struct NesControllerData
{
  bool button_a;
  bool button_b;
  bool button_select;
  bool button_start;
  bool button_up;
  bool button_down;
  bool button_left;
  bool button_right;
};

// SNES buttons in data order are:
// B, Y, Select, Start, Up, Down, Left, Right
// A, X, L, R, ?, ?, ?, ?
struct SnesControllerData
{
  bool button_b;
  bool button_y;
  bool button_select;
  bool button_start;
  bool button_up;
  bool button_down;
  bool button_left;
  bool button_right;
  bool button_a;
  bool button_x;
  bool button_l;
  bool button_r;
  bool dummy1;
  bool dummy2;
  bool dummy3;
  bool dummy4;
};

// map from bitset of dpad buttons to Gamepad API dpad value.
// assuming the bits are down, right, up, left.
// for invalid combinations, choose a meaningful fallback.
const int8_t DPAD_MAPPINGS[] =
{
  GAMEPAD_DPAD_CENTERED,   // 0000 = none
  GAMEPAD_DPAD_DOWN,       // 0001 = down
  GAMEPAD_DPAD_RIGHT,      // 0010 = right
  GAMEPAD_DPAD_DOWN_RIGHT, // 0011 = down+right
  GAMEPAD_DPAD_UP,         // 0100 = up
  GAMEPAD_DPAD_DOWN,       // 0101 = down+up = invalid
  GAMEPAD_DPAD_UP_RIGHT,   // 0110 = right+up
  GAMEPAD_DPAD_RIGHT,      // 0111 = right+up+down = invalid
  GAMEPAD_DPAD_LEFT,       // 1000 = left
  GAMEPAD_DPAD_DOWN_LEFT,  // 1001 = down+left
  GAMEPAD_DPAD_RIGHT,      // 1010 = right+left = invalid
  GAMEPAD_DPAD_DOWN,       // 1011 = right+left+down = invalid
  GAMEPAD_DPAD_UP_LEFT,    // 1100 = up+left
  GAMEPAD_DPAD_LEFT,       // 1101 = down+up+left = invalid
  GAMEPAD_DPAD_UP,         // 1110 = right+up+left = invalid
  GAMEPAD_DPAD_CENTERED    // 1111 = all buttons = invalid
};

int8_t getDpadValue(bool down, bool right, bool up, bool left)
{
  byte index = byte(down) |
               (byte(right) << 1) |
               (byte(up) << 2) |
               (byte(left) << 3);

  return DPAD_MAPPINGS[index];
}

void readController()
{
  // send initial signal to store button states
  digitalWrite(LATCH_PIN, HIGH);
  delayMicroseconds(12);
  digitalWrite(LATCH_PIN, LOW);

  // read all buttons
  for (int i = 0; i < BUTTON_COUNT; ++i)
  {
    buttonPressed[i] = (digitalRead(DATA_PIN) == LOW);

    // send clock pulse to switch to next button
    delayMicroseconds(6);
    digitalWrite(CLOCK_PIN, HIGH);
    delayMicroseconds(6);
    digitalWrite(CLOCK_PIN, LOW);
  }
}

void updateGamepad()
{
  SnesControllerData &data = *(SnesControllerData*) &buttonPressed;
  
  Gamepad.releaseAll();
  Gamepad.dPad1(getDpadValue(data.button_down, data.button_right, data.button_up, data.button_left));

  // TODO: mapping constants
  if (data.button_b) Gamepad.press(1);
  if (data.button_y) Gamepad.press(2);
  if (data.button_select) Gamepad.press(3);
  if (data.button_start) Gamepad.press(4);
  if (data.button_a) Gamepad.press(5);
  if (data.button_x) Gamepad.press(6);
  if (data.button_l) Gamepad.press(7);
  if (data.button_r) Gamepad.press(8);

  Gamepad.write();
}

void setup()
{
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, INPUT);

  Gamepad.begin();
}

void loop()
{
  unsigned long start = millis();

  readController();
  updateGamepad();

  unsigned long duration = millis() - start;
  if (duration < POLL_INTERVAL_MS)
    delay(POLL_INTERVAL_MS - duration);
}
