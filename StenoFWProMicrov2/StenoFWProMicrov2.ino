/**
 * StenoFW is a firmware for Stenoboard keyboards.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2014 Emanuele Caruso. See LICENSE.txt for details.
 * 
 * StenoFW for Pro Micro v2
 * Modifications by Lenno Liu:
 * 
 * Summary:
 * Modified pins to work on the Pro Micro, and shifted them around.
 * Added Fn3 pin on Row 2 Column 5
 * 
 * 
 * 
 * Swapped Column Pin 2 with LED pin 3.
 * Swapped Row Pins {11, 12, 13} with {16, 14, 15}
 * Shifted all pins down to have all columns on the left, all rows on the right.
 * Added [3, 4], [5] For RH asterisk differentiation
 * Added pins [21, 20, 19] for NKRO, Gemini, TXBolt LED 
 * 
 * Summarization of pins and outputs:
 * Pin 2: LED Backlight
 * Pin 3: N/A (unused)
 * Pin 4: Column 5
 * Pin 5: Column 4
 * Pin 6: Column 3
 * Pin 7: Column 2
 * Pin 8: Column 1
 * Pin 9: Column 0
 * 
 * Pin 10: Row 4
 * Pin 16: Row 3
 * Pin 14: Row 2
 * Pin 15: Row 1
 * Pin 18: Row 0
 * Pin 19: TXBolt LED
 * Pin 20: Gemini LED
 * Pin 21: NKRO LED
 * 
 * To do:
 * Line 428: Does Fn3 key work?
 * Define new Fn1 function for toggling Plover.
 * To do this: When Fn1 is pressed alone, input a string.
 * In plover, when string is input, toggle.
 * Or do something else, such as input a key combination. In plover, when key combination is pressed, toggle.
 */

#define ROWS 5
#define COLS 6

/* The following matrix is shown here for reference only.
char keys[ROWS][COLS] = {
  {'S', 'T', 'P', 'H', '*', Fn1},
  {'S', 'K', 'W', 'R', '*', Fn2},
  {'a', 'o', 'e', 'u', '#'},
  {'f', 'p', 'l', 't', 'd'},
  {'r', 'b', 'g', 's', 'z'}

  New Matrix: (Top asterisks for left hand, bottom asterisks for right, Fn3)
  {'S', 'T', 'P', 'H', '*', Fn1},
  {'S', 'K', 'W', 'R', '*', Fn2},
  {'a', 'o', 'e', 'u', '#', Fn3},
  {'f', 'p', 'l', 't', 'd', '*'},
  {'r', 'b', 'g', 's', 'z', '*'}
*/

// Configuration variables
const int rowPins[ROWS] = {18, 15, 14, 16, 10}; 
const int colPins[COLS] = {9, 8, 7, 6, 5, 4}; //const might not make this compile
const int ledPin = 2;
const long debounceMillis = 20;

const int NKROPin = 21;
const int GEMINIPin = 20;
const int TXBOLTPin = 19;

// Keyboard state variables
boolean isStrokeInProgress = false;
boolean currentChord[ROWS][COLS];
boolean currentKeyReadings[ROWS][COLS];
boolean debouncingKeys[ROWS][COLS];
unsigned long debouncingMicros[ROWS][COLS];

// Other state variables
int ledIntensity = 1; // Min 0 - Max 255

// Protocol state
#define GEMINI 0
#define TXBOLT 1
#define NKRO 2
int protocol = NKRO;


// This is called when the keyboard is connected
void setup() {
   Keyboard.begin();
  Serial.begin(9600);
  for (int i = 0; i < COLS; i++)
    pinMode(colPins[i], INPUT_PULLUP); //initialized each INPUT pin
  for (int i = 0; i < ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH); //init's each output pin to high
  }
  pinMode(ledPin, OUTPUT); 
  analogWrite(ledPin, ledIntensity);
  
  digitalWrite(NKROPin, HIGH);
  
  clearBooleanMatrixes();
}

// Read key states and handle all chord events
void loop() {
  readKeys();
  
  boolean isAnyKeyPressed = true;
  
  // If stroke is not in progress, check debouncing keys
  if (!isStrokeInProgress) {
    checkAlreadyDebouncingKeys();
    if (!isStrokeInProgress) checkNewDebouncingKeys();
  }
  
  // If any key was pressed, record all pressed keys
  if (isStrokeInProgress) {
    isAnyKeyPressed = recordCurrentKeys();
  }
  
  // If all keys have been released, send the chord and reset global state
  if (!isAnyKeyPressed) {
    sendChord();
    clearBooleanMatrixes();
    isStrokeInProgress = false;
  }
}

// Record all pressed keys into current chord. Return false if no key is currently pressed
boolean recordCurrentKeys() {
  boolean isAnyKeyPressed = false;
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (currentKeyReadings[i][j] == true) {
        currentChord[i][j] = true;
        isAnyKeyPressed = true;
      }
    }
  }
  return isAnyKeyPressed;
}

// If a key is pressed, add it to debouncing keys and record the time
void checkNewDebouncingKeys() {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (currentKeyReadings[i][j] == true && debouncingKeys[i][j] == false) {
        debouncingKeys[i][j] = true;
        debouncingMicros[i][j] = micros();
      }
    }
  }
}

// Check already debouncing keys. If a key debounces, start chord recording.
void checkAlreadyDebouncingKeys() {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (debouncingKeys[i][j] == true && currentKeyReadings[i][j] == false) {
        debouncingKeys[i][j] = false;
        continue;
      }
      if (debouncingKeys[i][j] == true && micros() - debouncingMicros[i][j] / 1000 > debounceMillis) {
        isStrokeInProgress = true;
        currentChord[i][j] = true;
        return;
      }
    }
  }
}

// Set all values of all boolean matrixes to false
void clearBooleanMatrixes() {
  clearBooleanMatrix(currentChord, false);
  clearBooleanMatrix(currentKeyReadings, false);
  clearBooleanMatrix(debouncingKeys, false);
}

// Set all values of the passed matrix to the given value
void clearBooleanMatrix(boolean booleanMatrix[][COLS], boolean value) {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      booleanMatrix[i][j] = value;
    }
  }
}

// Read all keys
void readKeys() {
  for (int i = 0; i < ROWS; i++) {
    digitalWrite(rowPins[i], LOW);
    for (int j = 0; j < COLS; j++)
      currentKeyReadings[i][j] = digitalRead(colPins[j]) == LOW ? true : false;
    digitalWrite(rowPins[i], HIGH);
  }
}

// Send current chord using NKRO Keyboard emulation
void sendChordNkro() {
  /* QWERTY mapping
  * Note:
  * Modified to add Y and H
  */ 
  char qwertyMapping[ROWS][COLS] = {
    {'q', 'w', 'e', 'r', 't', ' '},
    {'a', 's', 'd', 'f', 'g', ' '},
    {'c', 'v', 'n', 'm', '3', ' '},
    {'u', 'i', 'o', 'p', '[', 'y'},
    {'j', 'k', 'l', ';', '\'', 'h'}
  };
  int keyCounter = 0;
  char qwertyKeys[ROWS * COLS];
  boolean firstKeyPressed = false;
  
  // Calculate qwerty keys array using qwertyMappings[][]
  for (int i = 0; i < ROWS; i++)
    for (int j = 0; j < COLS; j++)
      if (currentChord[i][j]) {
        qwertyKeys[keyCounter] = qwertyMapping[i][j];
        keyCounter++;
      }
  // Emulate keyboard key presses
  for (int i = 0; i < keyCounter; i++) {
    if (qwertyKeys[i] != ' ') {
      Keyboard.press(qwertyKeys[i]);
      if (!firstKeyPressed) firstKeyPressed = true;
      else Keyboard.release(qwertyKeys[i]);
    }
  }
  Keyboard.releaseAll();
}
 
// Send current chord over serial using the Gemini protocol. 
// Add currentChord[3][5] and currentChord[4][5] (Y and H) 
void sendChordGemini() {
  // Initialize chord bytes
  byte chordBytes[] = {B10000000, B0, B0, B0, B0, B0};
  
  // Byte 0
  if (currentChord[2][4]) {
    chordBytes[0] = B10000001;
  }
  
  // Byte 1
  if (currentChord[0][0] || currentChord[1][0]) {
    chordBytes[1] += B01000000;
  }
  if (currentChord[0][1]) {
    chordBytes[1] += B00010000;
  }
  if (currentChord[1][1]) {
    chordBytes[1] += B00001000;
  }
  if (currentChord[0][2]) {
    chordBytes[1] += B00000100;
  }
  if (currentChord[1][2]) {
    chordBytes[1] += B00000010;
  }
  if (currentChord[0][3]) {
    chordBytes[1] += B00000001;
  }
  
  // Byte 2
  if (currentChord[1][3]) {
    chordBytes[2] += B01000000;
  }
  if (currentChord[2][0]) {
    chordBytes[2] += B00100000;
  }
  if (currentChord[2][1]) {
    chordBytes[2] += B00010000;
  }
  if (currentChord[0][4] || currentChord[1][4] || currentChord[3][5] || currentChord[4][5]) {
    chordBytes[2] += B00001000;
  }
  
  // Byte 3
  if (currentChord[2][2]) {
    chordBytes[3] += B00001000;
  }
  if (currentChord[2][3]) {
    chordBytes[3] += B00000100;
  }
  if (currentChord[3][0]) {
    chordBytes[3] += B00000010;
  }
  if (currentChord[4][0]) {
    chordBytes[3] += B00000001;
  }
  
  // Byte 4
  if (currentChord[3][1]) {
    chordBytes[4] += B01000000;
  }
  if (currentChord[4][1]) {
    chordBytes[4] += B00100000;
  }
  if (currentChord[3][2]) {
    chordBytes[4] += B00010000;
  }
  if (currentChord[4][2]) {
    chordBytes[4] += B00001000;
  }
  if (currentChord[3][3]) {
    chordBytes[4] += B00000100;
  }
  if (currentChord[4][3]) {
    chordBytes[4] += B00000010;
  }
  if (currentChord[3][4]) {
    chordBytes[4] += B00000001;
  }

  // Byte 5
  if (currentChord[4][4]) {
    chordBytes[5] += B00000001;
  }

  // Send chord bytes over serial: Unknown if modifications mess this up.
  for (int i = 0; i < 6; i++) {
    Serial.write(chordBytes[i]);
  }
}

void sendChordTxBolt() {
  byte chordBytes[] = {B0, B0, B0, B0, B0};
  int index = 0;
  
  // TX Bolt uses a variable length packet. Only those bytes that have active
  // keys are sent. The header bytes indicate which keys are being sent. They
  // must be sent in order. It is a good idea to send a zero after every packet.
  // 00XXXXXX 01XXXXXX 10XXXXXX 110XXXXX
  //   HWPKTS   UE*OAR   GLBPRF    #ZDST
  
  // byte 1
  // S-
  if (currentChord[0][0] || currentChord[1][0]) chordBytes[index] |= B00000001;
  // T-
  if (currentChord[0][1]) chordBytes[index] |= B00000010;  
  // K-
  if (currentChord[1][1]) chordBytes[index] |= B00000100;
  // P-
  if (currentChord[0][2]) chordBytes[index] |= B00001000;
  // W-
  if (currentChord[1][2]) chordBytes[index] |= B00010000;
  // H-
  if (currentChord[0][3]) chordBytes[index] |= B00100000;
  // Increment the index if the current byte has any keys set.
  if (chordBytes[index]) index++;
  
  // byte 2
  // R-
  if (currentChord[1][3]) chordBytes[index] |= B01000001;
  // A
  if (currentChord[2][0]) chordBytes[index] |= B01000010;
  // O
  if (currentChord[2][1]) chordBytes[index] |= B01000100;
  // * added YH
  if (currentChord[0][4] || currentChord[1][4] || currentChord[3][5] || currentChord[4][5]) chordBytes[index] |= B01001000;
  // E
  if (currentChord[2][2]) chordBytes[index] |= B01010000;
  // U
  if (currentChord[2][3]) chordBytes[index] |= B01100000;
  // Increment the index if the current byte has any keys set.
  if (chordBytes[index]) index++;
  
  // byte 3
  // -F
  if (currentChord[3][0]) chordBytes[index] |= B10000001;
  // -R
  if (currentChord[4][0]) chordBytes[index] |= B10000010;
  // -P
  if (currentChord[3][1]) chordBytes[index] |= B10000100;
  // -B
  if (currentChord[4][1]) chordBytes[index] |= B10001000;
  // -L
  if (currentChord[3][2]) chordBytes[index] |= B10010000;
  // -G
  if (currentChord[4][2]) chordBytes[index] |= B10100000;
  // Increment the index if the current byte has any keys set.
  if (chordBytes[index]) index++;
  
  // byte 4
  // -T
  if (currentChord[3][3]) chordBytes[index] |= B11000001;
  // -S
  if (currentChord[4][3]) chordBytes[index] |= B11000010;
  // -D
  if (currentChord[3][4]) chordBytes[index] |= B11000100;
  // -Z
  if (currentChord[4][4]) chordBytes[index] |= B11001000;
  // #
  if (currentChord[2][4]) chordBytes[index] |= B11010000;
  // Increment the index if the current byte has any keys set.
  if (chordBytes[index]) index++;
  
  // Now we have index bytes followed by a zero byte where 0 < index <= 4.
  index++; // Increment index to include the trailing zero byte.
  for (int i = 0; i < index; i++) {
    Serial.write(chordBytes[i]);
  }
}

// Send the chord using the current protocol. If there are fn keys
// pressed, delegate to the corresponding function instead.
// In future versions, there should also be a way to handle fn keys presses before
// they are released, eg. for mouse emulation functionality or custom key presses.
void sendChord() {
  // If fn keys have been pressed, delegate to corresponding method and return
  if (currentChord[0][5] && currentChord[1][5]) {
    fn1fn2();
    return;
  } else if (currentChord[0][5]) {
    fn1();
    return;
  } else if (currentChord[1][5]) {
    fn2();
    return;
  } else if (currentChord[2][5]) {
    fn3();
    return;
  } 
  
  if (protocol == NKRO) {
    sendChordNkro();
  } else if (protocol == GEMINI) {
    sendChordGemini();
  } else {
    sendChordTxBolt();
  }
}

// Fn1 functions
//
// This function is called when "fn1" key has been pressed, but not "fn2".
// Tip: maybe it is better to avoid using "fn1" key alone in order to avoid
// accidental activation?
//
// Current functions:
//    PH-PB   ->   Set NKRO Keyboard emulation mode
//    PH-G   ->   Set Gemini PR protocol mode
//    PH-B   ->   Set TX Bolt protocol mode
// Switched protocol toggle to Fn1-Fn2.
// To add: Plover toggle.
void fn1() {

}
// Fn2 functions
//
// This function is called when "fn2" key has been pressed, but not "fn1".
// Tip: maybe it is better to avoid using "fn2" key alone in order to avoid
// accidental activation?
//
// Current functions: none.
// Added function: Increase LED brightness.
void fn2() {
  if (ledIntensity == 0) ledIntensity +=1;
      else if(ledIntensity < 50) ledIntensity += 10;
      else ledIntensity += 30;
      if (ledIntensity > 255) ledIntensity = 255;
      analogWrite(ledPin, ledIntensity);
}

// Added Fn3 pin on [2][5]
// Added function: Decrease LED brightness.
void fn3() {
  if(ledIntensity == 0) ledIntensity = 0;
      else if(ledIntensity < 50) ledIntensity -= 10;
      else ledIntensity -= 30;
      if (ledIntensity < 1) ledIntensity = 0;
      analogWrite(ledPin, ledIntensity);
}
// Fn1-Fn2 functions
//
// This function is called when both "fn1" and "fn2" keys have been pressed.
//
// Current functions:
//   HR-P   ->   LED intensity up
//   HR-F   ->   LED intensity down
// Added Fn3, changed LED intensity to Fn2 and Fn3
//
// Changed Protocol Toggle to Fn1-Fn2
void fn1fn2() {
  switch(protocol)
  {
    case NKRO:
      protocol = GEMINI;
      digitalWrite(NKROPin, LOW);
      digitalWrite(GEMINIPin, HIGH);
      break;
    case GEMINI:
      protocol = TXBOLT;
      digitalWrite(GEMINIPin, LOW);
      digitalWrite(TXBOLTPin, HIGH);
      break;
    case TXBOLT:
      default:
      protocol = NKRO;
      digitalWrite(GEMINIPin, LOW);
      digitalWrite(TXBOLTPin, LOW);
      digitalWrite(NKROPin, HIGH);
  }
}
