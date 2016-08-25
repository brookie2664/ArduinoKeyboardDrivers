/**
 * PlanckProMicro is firmware written for Stenoboard keyboards that use the Planck layout.
 * Please see http://ortholinearkeyboards.com/planck for more info.
 * Note: To save on the number of pins needed and to incorporate LED control, the matrix
 * is an 8x6 matrix instead of a 4x12 matrix.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * Based off of StenoFW by Emanuele Caruso, 2014. Written by Lenno Liu, 2015.
 * 
 * I/O Pins (Pro Micro):
 *             MICRO USB
 * 2: Column 0            21: Row 0
 * 3: stenoledPin         20: Row 1
 * 4: Column 1            19: Row 2
 * 5: Column 2            18: Row 3
 * 6: Column 3            15: Row 4
 * 7: Column 4            14: Row 5
 * 8: Column 5            16: Row 6
 * 9: keyboardledPin      10: Row 7
 * 
 * https://www.arduino.cc/en/Reference/AnalogWrite explains why 3 and 9 are the LED pins.
 * 
 * Define string for toggling Plover, and add that to plover toggle on/off function here.
 * Don't forget to add that to Plover as well.
 * 
 * Reference Matrix: (Actual matrixes must be found below)
 *{'S', 'T', 'P', 'H', '*', '*'},
 *{'S', 'K', 'W', 'R', '*', '*'},
 *{' ', ' ', ' ', ' ', Fn1, Fn2},
 *{' ', ' ', '#', 'a', 'o', '#'},
 *
 *{'*', 'F', 'P', 'L', 'T', 'D'},
 *{'*', 'R', 'B', 'G', 'S', 'Z'},
 *{Fn3, ' ', ' ', ' ', ' ', ' '},
 *{'e', 'u', '#', led, LED, ' '}
*/

#define ROWS 8
#define COLS 6

//Define pin numbers
const int rowpins[ROWS] = {21, 20, 19, 18, 15, 14, 16, 10};
const int colpins[COLS] = {2, 4, 5, 6, 7, 8};
const int stenoledpin = 3;
const int keyboardledpin = 9;

//Debounce Time (default 20 milliseconds)
const long debouncetime = 20; //in milliseconds

//Keyboard matrixes, key press.
boolean anykeyispressed = false;
boolean stroke = false;
boolean currentchord[ROWS][COLS];
boolean keyreadings[ROWS][COLS];
boolean debouncingkeypress[ROWS][COLS];
boolean presskey[ROWS][COLS];
boolean debouncingkeyrelease[ROWS][COLS];
boolean releasekey[ROWS][COLS];
unsigned long pressdebouncetime[ROWS][COLS];
unsigned long releasedebouncetime[ROWS][COLS];

//Functionality initialized as plover mode.
#define plover 0
#define qwerty 1
#define dvorak 2
int functionality = plover;

//Other
int ledbrightness = 1; // 0 - 255
#define NKRO 0
#define GEMINI 1
#define TXBOLT 2
int protocol = NKRO;

void plovertoggleon() {
  //Keyboard.press(); // define plover toggle on chord.
  //delay(100);
  //Keyboard.releaseAll();
}
void plovertoggleoff() {
  //Keyboard.press(); // define plover toggle off chord.
  //delay(100);
  //Keyboard.releaseAll();
}

void setup() {
  Keyboard.begin();
  Serial.begin(9600);
  for (int i = 0; i < COLS; i++) {
    pinMode(colpins[i], INPUT_PULLUP);
  }
  for (int i = 0; i < ROWS; i++) {
    pinMode(rowpins[i], OUTPUT);
    digitalWrite(rowpins[i], HIGH);
  }
  clearmatrixes();
  pinMode(stenoledpin, OUTPUT);
  pinMode(keyboardledpin, OUTPUT);
  digitalWrite(stenoledpin, HIGH);
  digitalWrite(keyboardledpin, HIGH);
  delay(300);
  digitalWrite(stenoledpin, LOW);
  digitalWrite(keyboardledpin, LOW);
  delay(300);
  plovermode();
}

void loop() {
  readkeys();
  debouncekeys();
  checkdebounces();
  macros();
  if (functionality != plover) {
    sendsignal();
  }
  if (functionality == plover && stroke == true) {
    //anykeyispressed = true;
    anykeyispressed = checkkeys;
    if (anykeyispressed == false) {
      sendchord();
      clearmatrixes();
      stroke = false; 
    }
  }
}
void readkeys() {
  for (int i = 0; i < ROWS; i++) {
    digitalWrite(rowpins[i], LOW);
    for (int j = 0; j < COLS; j++)
      if (digitalRead(colpins[j]) == LOW) keyreadings[i][j] = true;
      else keyreadings[i][j] = false;
    digitalWrite(rowpins[i], HIGH);
  }
}
void debouncekeys() {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (keyreadings[i][j] == true && presskey[i][j] == true) {
        debouncingkeyrelease[i][j] = false;
        continue;
      }
      if (keyreadings[i][j] == true && debouncingkeypress[i][j] == false) {
        debouncingkeypress[i][j] = true;
        pressdebouncetime[i][j] = micros();
      }
      if (keyreadings[i][j] == false && debouncingkeypress[i][j] == true) {
        debouncingkeypress[i][j] = false;
        continue;
      }
      if (keyreadings[i][j] == false && presskey[i][j] == true) {
        debouncingkeyrelease[i][j] = true;
        releasedebouncetime[i][j] = micros();
      }
    }
  }
}
void checkdebounces() {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (debouncingkeypress[i][j] == true && micros() - pressdebouncetime[i][j] / 1000 > debouncetime) {
        presskey[i][j] = true;
        currentchord[i][j] = true;
        stroke = true; //Do I need to do something special to this, to carry it out of this function?
      }
      if (debouncingkeyrelease[i][j] == true && micros() - releasedebouncetime[i][j] / 1000 > debouncetime) {
        releasekey[i][j] = true;
        presskey[i][j] = false;
        debouncingkeypress[i][j] = false;
        debouncingkeyrelease[i][j] = false;
      }
    }
  }
}

void macros() { //Macros for special key combinations. Feel free to modify and add.
  if (presskey[0][0] && presskey[3][0] && presskey[4][5] && presskey[7][5]){ //Corners. Toggles between plover/qwerty/dvorak.
    Keyboard.releaseAll();
    togglefunctionality();
    clearmatrixes();
    delay(300); //to prevent multiple toggles during a single keystroke?
    return;
  }
  if ((functionality != plover) && presskey[4][5] && presskey[6][5]) { //shift backspace
    Keyboard.press('?'); //needs testing.
    return;
  }
  if ((functionality != plover) && presskey[4][5] && releasekey[6][5]) { //releasing shift in shift backspace
    Keyboard.release('?');
    return;
  }
  if ((functionality != plover) && releasekey[4][5] && presskey[6][5]) { //releasing backspace in shift backspace
    Keyboard.release('?');
    return;
  }
  if ((functionality != plover) && presskey[7][4]) {
    increaseledbrightness();
    delay(15); // are these delays necessary?
    return;
  }
  if ((functionality != plover) && presskey[7][3]) {
    decreaseledbrightness();
    delay(15);
    return;
  }
  if (functionality == plover && presskey[2][4] && presskey[2][5] && presskey[6][0]) {
    toggleprotocol();
    delay(500);
    return;
  }
}

// Matrixes.
char keymapping[ROWS][COLS]; 
const char qwertymapping[ROWS][COLS] = {
  {179, 'q', 'w', 'e', 'r', 't'},  // tab
  {193, 'a', 's', 'd', 'f', 'g'},  // capslock
  {129, 'z', 'x', 'c', 'v', 'b'},  // shift
  {128, 131, 130, 177, ' ',  32},  // l_control, win, alt, esc, 'lower', space
  {'y', 'u', 'i', 'o', 'p', 178},  // ascii table says 8, leonardo table says 178 (backspace)
  {'h', 'j', 'k', 'l', ';', '\''}, // to get enter, there needs to be a layer raise. Enter.
  {'n', 'm', ',', '.', '/', 133},  //
  { 32, ' ', 216, 217, 218, 215}   // space, 'raise', left, down, up, right
}; 
const char loweredqwerty[ROWS][COLS] = {
  {'`', '1', '2', '3', '4', '5'},  // tab -> `
  {193, ' ', ' ', ' ', ' ', ' '},
  {129, ' ', ' ', ' ', ' ', ' '}, 
  {128, 131, 130, 177, ' ',  32}, 
  {'6', '7', '8', '9', '0', 178}, 
  {' ', ' ', ' ', '-', '=', 176},  //enter
  {' ', ' ', ' ', '[', ']', '\\'}, // shift -> \
  { 32, ' ', 216, 217, 218, 215}   
}; 
const char raisedqwerty[ROWS][COLS] = {
  {'~', '!', '@', '#', '$', '%'}, 
  {194, 195, 196, 197, 198, 199},  //F1 - F6
  {200, 201, 202, 203, 204, 205},  //F7 - F12
  {128, 131, 130, 177, ' ',  32},  
  {'^', '&', '*', '(', ')', 178}, 
  {209, 210, 211, '_', '+', 176},  //Ins, Home, PgUp, enter
  {212, 213, 214, '{', '}', '|'},  //Del, End, PgDn, shift -> |
  { 32, ' ', 216, 217, 218, 215} 
}; 
const char dvorakmapping[ROWS][COLS] = {
  {179, '\'', ',', '.', 'p', 'y'},
  {193, 'a', 'o', 'e', 'u', 'i'}, 
  {129, ';', 'q', 'j', 'k', 'x'},  
  {128, 131, 130, 177, ' ',  32},
  {'f', 'g', 'c', 'r', 'l', 178}, 
  {'d', 'h', 't', 'n', 's', '-'}, 
  {'b', 'm', 'w', 'v', 'z', 133},
  { 32, ' ', 216, 217, 218, 215}  
};
const char lowereddvorak[ROWS][COLS] = {
  {'`', '1', '2', '3', '4', '5'}, 
  {193, ' ', ' ', ' ', ' ', ' '}, 
  {129, ' ', ' ', ' ', ' ', ' '}, 
  {128, 131, 130, 177, ' ',  32}, 
  {'6', '7', '8', '9', '0', 178}, 
  {' ', ' ', ' ', '[', ']', 176}, 
  {' ', ' ', ' ', '/', '=', '\\'},
  { 32, ' ', 216, 217, 218, 215}  
}; 
const char raiseddvorak[ROWS][COLS] = {
  {'~', '!', '@', '#', '$', '%'}, 
  {194, 195, 196, 197, 198, 199}, 
  {200, 201, 202, 203, 204, 205},
  {128, 131, 130, 177, ' ',  32},
  {'^', '&', '*', '(', ')', 178}, 
  {209, 210, 211, '{', '}', 176}, 
  {212, 213, 214, '?', '+', '|'},
  { 32, ' ', 216, 217, 218, 215}  
}; 
void sendsignal() {
  for (int i = 0; i < ROWS; i++){
    for (int j = 0; j < COLS; j++){
      if (presskey[7][1] == true && functionality == qwerty) {
       keymapping[i][j] = raisedqwerty[i][j]; //raised qwerty
      }
      else if (presskey[3][4] == true && functionality == qwerty) {
        keymapping[i][j] = loweredqwerty[i][j]; //lowered qwerty
      }  
      else if (presskey[7][1] == true && functionality == dvorak) {
        keymapping[i][j] = raiseddvorak[i][j]; // raised dvorak
      }
      else if (presskey[3][4] == true && functionality == dvorak) {
        keymapping[i][j] = lowereddvorak[i][j]; // lowered dvorak
      }
      else if (functionality == dvorak) {
        keymapping[i][j] = dvorakmapping[i][j]; // dvorak
      }
      else {
        keymapping[i][j] = qwertymapping[i][j]; // otherwise use qwerty
      }
    }
  }
  for (int i = 0; i < ROWS; i++){
    for (int j = 0; j < COLS; j++){
      if (presskey[i][j]) {
        Keyboard.press(keymapping[i][j]);
      }
      if (releasekey[i][j]) {
        Keyboard.release(keymapping[i][j]);
        releasekey[i][j] = false;
      }
    }
  }
}


void clearmatrixes() {
  clearmatrix(currentchord, false);
  clearmatrix(keyreadings, false);
  clearmatrix(debouncingkeypress, false);
  clearmatrix(presskey, false);
  clearmatrix(debouncingkeyrelease, false);
  clearmatrix(releasekey, false);
}
void clearmatrix(boolean matrix[][COLS], boolean value) {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      matrix[i][j] = value;
    }
  }
}


void togglefunctionality() {
  if (functionality == plover) {
    dvorakmode();
    return;
  }
  else if (functionality == dvorak) {
    qwertymode();
    return;
  }
  else
    plovermode();
}
void plovermode() {
  functionality = plover;
  plovertoggleon();
  analogWrite(stenoledpin, ledbrightness);
  analogWrite(keyboardledpin, 0);
}
void dvorakmode() {
  functionality = dvorak;
  plovertoggleoff();
  analogWrite(stenoledpin, 0);
  analogWrite(keyboardledpin, ledbrightness);
}
void qwertymode() {
  functionality = qwerty;
  plovertoggleoff();
  analogWrite(stenoledpin, ledbrightness);
  analogWrite(keyboardledpin, ledbrightness);
}

//LED brightness control
void increaseledbrightness() {
  if (ledbrightness == 0) ledbrightness +=1;
  else if (ledbrightness < 50) ledbrightness +=10;
  else ledbrightness +30;
  if (ledbrightness > 255) ledbrightness = 0;
  if (functionality != dvorak) {
  analogWrite(stenoledpin, ledbrightness);  
  }
  if (functionality != plover) {
    analogWrite(keyboardledpin, ledbrightness);
  }
}
void decreaseledbrightness() {
  if (ledbrightness == 0) ledbrightness = 255;
  else if (ledbrightness <50) ledbrightness -=10;
  else ledbrightness -=30;
  if (ledbrightness < 1) ledbrightness = 0;
  if (functionality != dvorak) {
  analogWrite(stenoledpin, ledbrightness);  
  }
  if (functionality != plover) {
    analogWrite(keyboardledpin, ledbrightness);
  }
}
//Protocol toggle.
void toggleprotocol() {
  if (protocol == NKRO) {
    protocol = GEMINI;
    return;
  }
  else if (protocol == GEMINI) {
    protocol = TXBOLT;
    return;
  }
  else {
  protocol = NKRO;
  }
}

boolean checkkeys() { //I had void instead...?
  boolean anykeyispressed = false; //does this work? Do I need boolean?
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (presskey[i][j]) { //I don't need currentchord[i][j] = true right after, do I?
        anykeyispressed = true;
      }
    }
  }
  return anykeyispressed;
}

void sendchord() {
  if (protocol == GEMINI) {
    sendChordGemini();
    return;
  }
  else if (protocol == TXBOLT) {
    sendChordTXBolt();
    return;
  }
  else {
    sendChordNKRO();
  }
}
void sendChordNKRO() {
  const char NKROmapping[ROWS][COLS] = {
    {'q', 'w', 'e', 'r', 't', 'y'}, // {'y', 'u', 'i', 'o', 'p', '['},
    {'a', 's', 'd', 'f', 'g', 'h'}, // {'h', 'j', 'k', 'l', ';', '\''},
    {' ', ' ', ' ', ' ', ' ', ' '}, // {' ', ' ', ' ', ' ', ' ', ' '},
    {' ', ' ', '1', 'c', 'v', '2'}, // {'n', 'm', ' ', ' ', ' ', ' '}
    {'y', 'u', 'i', 'o', 'p', '['},
    {'h', 'j', 'k', 'l', ';', '\''},
    {' ', ' ', ' ', ' ', ' ', ' '},
    {'n', 'm', '3', ' ', ' ', ' '}    
  };
  int keyCounter = 0;
  char qwertyKeys[ROWS * COLS];
  boolean firstKeyPressed = false;
  
  // Calculate qwerty keys array using qwertyMappings[][]
  for (int i = 0; i < ROWS; i++)
    for (int j = 0; j < COLS; j++)
      if (currentchord[i][j]) {
        qwertyKeys[keyCounter] = NKROmapping[i][j];
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

//Gemini and TXBolt protocol sending.
void sendChordGemini() {
  // Initialize chord bytes
  byte chordBytes[] = {B10000000, B0, B0, B0, B0, B0};
  
  // Byte 0: #
  if (currentchord[3][5] || currentchord[3][2] || currentchord[7][2]) {
    chordBytes[0] = B10000001;
  }
  
  // Byte 1 
  if (currentchord[0][0] || currentchord[1][0]) {
    chordBytes[1] += B01000000;
  }
  if (currentchord[0][1]) {
    chordBytes[1] += B00010000;
  }
  if (currentchord[1][1]) {
    chordBytes[1] += B00001000;
  }
  if (currentchord[0][2]) {
    chordBytes[1] += B00000100;
  }
  if (currentchord[1][2]) {
    chordBytes[1] += B00000010;
  }
  if (currentchord[0][3]) {
    chordBytes[1] += B00000001;
  }
  
  // Byte 2
  if (currentchord[1][3]) {
    chordBytes[2] += B01000000;
  }
  if (currentchord[3][3]) {
    chordBytes[2] += B00100000;
  }
  if (currentchord[3][4]) {
    chordBytes[2] += B00010000;
  }
  if (currentchord[0][4] || currentchord[1][4] || currentchord[0][5] || currentchord[1][5] || currentchord[4][0] || currentchord[5][0]) {
    chordBytes[2] += B00001000;
  } //Asterisks
  
  // Byte 3
  if (currentchord[7][0]) {
    chordBytes[3] += B00001000;
  }
  if (currentchord[7][1]) {
    chordBytes[3] += B00000100;
  }
  if (currentchord[4][1]) {
    chordBytes[3] += B00000010;
  }
  if (currentchord[5][1]) {
    chordBytes[3] += B00000001;
  }
  
  // Byte 4
  if (currentchord[4][2]) {
    chordBytes[4] += B01000000;
  }
  if (currentchord[5][2]) {
    chordBytes[4] += B00100000;
  }
  if (currentchord[4][3]) {
    chordBytes[4] += B00010000;
  }
  if (currentchord[5][3]) {
    chordBytes[4] += B00001000;
  }
  if (currentchord[4][4]) {
    chordBytes[4] += B00000100;
  }
  if (currentchord[5][4]) {
    chordBytes[4] += B00000010;
  }
  if (currentchord[4][5]) {
    chordBytes[4] += B00000001;
  }

  // Byte 5
  if (currentchord[5][5]) {
    chordBytes[5] += B00000001;
  }

  // Send chord bytes over serial: Unknown if modifications mess this up.
  for (int i = 0; i < 6; i++) {
    Serial.write(chordBytes[i]);
  }
}

void sendChordTXBolt() {
  byte chordBytes[] = {B0, B0, B0, B0, B0};
  int index = 0;
  
  // TX Bolt uses a variable length packet. Only those bytes that have active
  // keys are sent. The header bytes indicate which keys are being sent. They
  // must be sent in order. It is a good idea to send a zero after every packet.
  // 00XXXXXX 01XXXXXX 10XXXXXX 110XXXXX
  //   HWPKTS   UE*OAR   GLBPRF    #ZDST
  
  // byte 1
  // S-
  if (currentchord[0][0] || currentchord[1][0]) chordBytes[index] |= B00000001;
  // T-
  if (currentchord[0][1]) chordBytes[index] |= B00000010;  
  // K-
  if (currentchord[1][1]) chordBytes[index] |= B00000100;
  // P-
  if (currentchord[0][2]) chordBytes[index] |= B00001000;
  // W-
  if (currentchord[1][2]) chordBytes[index] |= B00010000;
  // H-
  if (currentchord[0][3]) chordBytes[index] |= B00100000;
  // Increment the index if the current byte has any keys set.
  if (chordBytes[index]) index++;
  
  // byte 2
  // R-
  if (currentchord[1][3]) chordBytes[index] |= B01000001;
  // A
  if (currentchord[3][3]) chordBytes[index] |= B01000010;
  // O
  if (currentchord[3][4]) chordBytes[index] |= B01000100;
  // * added YH
  if (currentchord[0][4] || currentchord[1][4] || currentchord[0][5] || currentchord[1][5] || currentchord[4][0] || currentchord[5][0]) chordBytes[index] |= B01001000;
  // E
  if (currentchord[7][0]) chordBytes[index] |= B01010000;
  // U
  if (currentchord[7][1]) chordBytes[index] |= B01100000;
  // Increment the index if the current byte has any keys set.
  if (chordBytes[index]) index++;
  
  // byte 3
  // -F
  if (currentchord[4][1]) chordBytes[index] |= B10000001;
  // -R
  if (currentchord[5][1]) chordBytes[index] |= B10000010;
  // -P
  if (currentchord[4][2]) chordBytes[index] |= B10000100;
  // -B
  if (currentchord[5][2]) chordBytes[index] |= B10001000;
  // -L
  if (currentchord[4][3]) chordBytes[index] |= B10010000;
  // -G
  if (currentchord[5][3]) chordBytes[index] |= B10100000;
  // Increment the index if the current byte has any keys set.
  if (chordBytes[index]) index++;
  
  // byte 4
  // -T
  if (currentchord[4][4]) chordBytes[index] |= B11000001;
  // -S
  if (currentchord[5][4]) chordBytes[index] |= B11000010;
  // -D
  if (currentchord[4][5]) chordBytes[index] |= B11000100;
  // -Z
  if (currentchord[5][5]) chordBytes[index] |= B11001000;
  // #
  if (currentchord[3][5] || currentchord[3][2] || currentchord[7][2]) chordBytes[index] |= B11010000;
  // Increment the index if the current byte has any keys set.
  if (chordBytes[index]) index++;
  // Now we have index bytes followed by a zero byte where 0 < index <= 4.
  index++; // Increment index to include the trailing zero byte.
  for (int i = 0; i < index; i++) {
    Serial.write(chordBytes[i]);
  }
}

