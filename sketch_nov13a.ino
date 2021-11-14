/*    Sketch for Prototyping Keyboard V1.2
 *    by Cameron Coward 1/30/21
 *    
 *    Tested on Arduino Uno. Requires custom PCB
 *    and a 74HC595 shift register.
 *    
 *    More info: https://www.hackster.io/cameroncoward/64-key-prototyping-keyboard-matrix-for-arduino-4c9531
 */
#include <arduino-timer.h>

const int rowData = 2;  // shift register Data pin for rows
const int rowLatch = 3; // shift register Latch pin for rows
const int rowClock = 4; // shift register Clock pin for rows

const int columns[] = { A0, A1, A2, A3, A4, A5, 5, 6 };

const int enter_key   = 10;
const int arrow_l_key = 0;
const int arrow_r_key = 0;
const int arrow_u_key = 0;
const int arrow_d_key = 0;
const int f1_key      = 0;
const int f2_key      = 0;
const int shift_l_key = 0;
const int shift_r_key = 0;
const int tab_key     = 9;
const int capsKey     = 1;
const int esc_key     = 27;
const int ctrl_key    = 0;
const int unused_key  = 0;
const int space_key   = ' ';

// shiftRow is the required shift register byte for each row, rowState will contain pressed keys for each row
const byte shiftRow[] = {
  B11111110,
  B11111101,
  B11111011,
  B11110111,
  B11101111,
  B11011111,
  B10111111,
  B01111111
};
byte rowState[8] = { B00000000 };
byte prevRowState[8] = { B00000000 };

const char keys[] = {
  '1', 'v', '5', ',', '9', '`',  arrow_r_key, f2_key,
  'q', 'f', 't', 'k', 'o', ']',  arrow_d_key, f1_key,
  'a', 'r', 'g', 'i', 'l', '\\', arrow_u_key, ctrl_key,
  'z', '4', 'b', '8', '.', ' ',  arrow_l_key, shift_r_key,
  '3', 'x', '7', 'n', '-', '/',  space_key,   shift_l_key,
  'e', 's', 'u', 'h', '=', ';',  tab_key,     capsKey,
  'd', 'w', 'j', 'y', '[', 'p',  unused_key,  enter_key,
  'c', '2', 'm', '6', '\'', '0', esc_key,     unused_key
};

// ASCII codes for keys with shift pressed AND caps is active
const char capsShiftKeys[] = {
  '!', 'v', '%', '<', '(', '~', arrow_r_key, f2_key,
  'q', 'f', 't', 'k', 'o', '}', arrow_d_key, f1_key,
  'a', 'r', 'g', 'i', 'l', '|', arrow_u_key, ctrl_key,
  'z', '$', 'b', '*', '>', ' ', arrow_l_key, shift_r_key,
  '#', 'x', '&', 'n', '_', '?', space_key,   shift_l_key,
  'e', 's', 'u', 'h', '+', ':', tab_key,     capsKey,
  'd', 'w', 'j', 'y', '{', 'p', unused_key,  enter_key,
  'c', '@', 'm', '^', '"', ')', esc_key,     unused_key
};

// ASCII codes for keys with shift pressed.
const char shiftKeys[] = {
  '!', 'V', '%', '<', '(', '~', arrow_r_key, f2_key,
  'Q', 'F', 'T', 'K', 'O', '}', arrow_d_key, f1_key,
  'A', 'R', 'G', 'I', 'L', '|', arrow_u_key, ctrl_key,
  'Z', '$', 'B', '*', '>', ' ', arrow_l_key, shift_r_key,
  '#', 'X', '&', 'N', '_', '?', space_key,   shift_l_key,
  'E', 'S', 'U', 'H', '+', ':', tab_key,     capsKey,
  'D', 'W', 'J', 'Y', '{', 'P', unused_key,  enter_key,
  'C', '@', 'M', '^', '"', ')', esc_key,     unused_key
};

// ASCII codes for keys with caps is active
const char capsKeys[] = {
  '1', 'V', '5', ',', '9', '`',  arrow_r_key, f2_key,
  'Q', 'F', 'T', 'K', 'O', ']',  arrow_d_key, f1_key,
  'A', 'R', 'G', 'I', 'L', '\\', arrow_u_key, ctrl_key,
  'Z', '4', 'B', '8', '.', ' ',  arrow_l_key, shift_r_key,
  '3', 'X', '7', 'N', '-', '/',  space_key,   shift_l_key,
  'E', 'S', 'U', 'H', '=', ';',  tab_key,     capsKey,
  'D', 'W', 'J', 'Y', '[', 'P',  unused_key,  enter_key,
  'C', '2', 'M', '6', '\'', '0', esc_key,     unused_key
};
 
bool caps = false;
bool shift = false;
bool capsShift = false;
bool ctrl = false;
bool f1 = false;
bool f2 = false;

auto timer = timer_create_default();

void setup() {
  Serial.begin(9600);

  // setup all column pin as inputs with internal pullup resistors
  for (int i = 0; i < 8; i++) {
    pinMode(columns[i], INPUT_PULLUP);
  }
  
  // the outputs needed to control the 74HC595 shift register
  pinMode(rowLatch, OUTPUT);
  pinMode(rowClock, OUTPUT);
  pinMode(rowData, OUTPUT);

  // make sure shift register starts at all HIGH
  updateShiftRegister(B11111111);

  timer.every(5, readKeyboard);         // 5ms
}

void loop() {
  timer.tick();
}

void readKeyboard() {
    scanKeyboard();
    checkPressedKeys();
}

void updateShiftRegister(byte row) {
  digitalWrite(rowLatch, LOW); // set latch to low so we can write an entire byte at once
  shiftOut(rowData, rowClock, MSBFIRST, row);  // write that byte
  digitalWrite(rowLatch, HIGH); // set latch back to high so it shift register will remain stable until next change
}

void scanKeyboard() {
  for (int row = 0; row < 8; row++) {
    updateShiftRegister(shiftRow[row]);

    for (int column = 0; column < 8; column++) {
      bitWrite(rowState[row], column, !digitalRead(columns[column]));
    }
  }
}

void checkPressedKeys() {
  shift     = bitRead(rowState[3], 7) | bitRead(rowState[4], 7);
  ctrl      = bitRead(rowState[2], 7);
  f1        = bitRead(rowState[1], 7);
  f2        = bitRead(rowState[0], 7);
  capsShift = shift && caps;
  
  for (int row = 7; row >= 0; row--) {                    // iterate through each row
    for (int column = 7; column >= 0; column--) {                  // iterate through each bit in that row
      
      bool newBit = bitRead(rowState[row], column);             // check the state of that bit
      bool prevBit = bitRead(prevRowState[row], column);         // check the previous state of that bit
      
      if (newBit == 1 && prevBit == 0) {                       // only allows button press if state has changed to true
          int charIndex = (row * 8) + column;               // calculate which position in char array to select
          int character;

          if (capsShift) {
            character = capsShiftKeys[charIndex];
          } else if(caps) {
            character = capsKeys[charIndex];
          } else if (shift) {
            character = shiftKeys[charIndex];
          } else {
            character = keys[charIndex];
          }

          if (character == capsKey) {
            caps = !caps;
          } else if(character > 0) {
            processChar(character);
          }
      }
      bitWrite(prevRowState[row], column, newBit);
    }
  }
}

void processChar(char receivedKey) {
  Serial.print(receivedKey);
}
