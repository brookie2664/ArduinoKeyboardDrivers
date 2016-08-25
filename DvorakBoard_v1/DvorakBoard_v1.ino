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
 * Based off of StenoFW by Emanuele Caruso, 2014. Modified by Lenno Liu, 2016.
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
 * 
 * Reference Matrix: (Actual matrixes must be found below)
 *{'Tab ', '\'', ',', '.', 'p', 'y'},
 *{'Caps', 'a', 'o', 'e', 'u', 'i'},
 *{'Shft', ';', 'q', 'j', 'k', 'x'},
 *{'Ctrl', 'Win', 'Alt', 'Esc', 'Lower', 'Sp'},
 *
 *{'f', 'g', 'c', 'r', 'l', 'bksp'},
 *{'d', 'h', 't', 'n', 's', '-'},
 *{'b', 'm', 'w', 'v', 'z', 'shft'},
 *{'sp', '<', 'v', '^', '>', 'enter'}
*/

#define ROWS 8
#define COLS 6
#include <Keyboard.h>

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

//Other
int ledbrightness = 1; // 0 - 255


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
}

void loop() {
  readkeys();
  debouncekeys();
  checkdebounces();
  macros();
  sendsignal();
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
  if (presskey[4][5] && presskey[6][5]) { //shift backspace
    Keyboard.press('?'); //needs testing.
    return;
  }
  if (presskey[4][5] && releasekey[6][5]) { //releasing shift in shift backspace
    Keyboard.release('?');
    return;
  }
  if (releasekey[4][5] && presskey[6][5]) { //releasing backspace in shift backspace
    Keyboard.release('?');
    return;
  }
  /*if (presskey[7][4]) {
    increaseledbrightness();
    delay(15); // are these delays necessary?
    return;
  }
  if (presskey[7][3]) {
    decreaseledbrightness();
    delay(15);
    return;
  }*/
}

// Matrixes.
char keymapping[ROWS][COLS]; 
const char dvorakmapping[ROWS][COLS] = {
  {179, '\'', ',', '.', 'p', 'y'}, // tab
  {193, 'a', 'o', 'e', 'u', 'i'},  // capslock
  {129, ';', 'q', 'j', 'k', 'x'},  // l_shift
  {128, 131, 130, 177, ' ',  32},  // l_control, win, alt, esc, lower, space
  {'f', 'g', 'c', 'r', 'l', 178},  // Ascii says 8, leonardo says 178 (backspace)
  {'d', 'h', 't', 'n', 's', '-'},  //
  {'b', 'm', 'w', 'v', 'z', 133},  // r_shift
  { 32, 216, 217, 218, 215, 176}   // space, (raise) left, down, up, right (enter?)
};
const char lowereddvorak[ROWS][COLS] = {
  {'`', '1', '2', '3', '4', '5'},  // tab -> `
  {193, ' ', ' ', ' ', ' ', ' '}, 
  {129, ' ', ' ', ' ', ' ', ' '}, 
  {128, 131, 130, 177, ' ',  32}, 
  {'6', '7', '8', '9', '0', 178}, 
  {' ', ' ', '[', ']', ' ', ' '},   
  {' ', ' ', '/', '=', '\\', 133},
  { 32, 216, 217, 218, 215, 176}  // enter
}; 
/*const char raiseddvorak[ROWS][COLS] = {
  {'`', '1', '2', '3', '4', '5'}, 
  {193, ' ', ' ', ' ', ' ', ' '}, 
  {129, ' ', ' ', ' ', ' ', ' '}, 
  {128, 131, 130, 177, ' ',  32}, 
  {'6', '7', '8', '9', '0', 178}, 
  {' ', ' ', ' ', '[', ']', 176}, 
  {' ', ' ', ' ', '/', '=', '\\'},
  { 32, ' ', 216, 217, 218, 215}   
}; */ 
void sendsignal() {
  for (int i = 0; i < ROWS; i++){
    for (int j = 0; j < COLS; j++){
      //if (presskey[7][1] == true) {
        //keymapping[i][j] = raiseddvorak[i][j]; // raised dvorak
      //} //Put 'else' in front of 'if' if reimplementing raised dvorak
      if (presskey[3][4] == true) {
        keymapping[i][j] = lowereddvorak[i][j]; // lowered dvorak
      }
      else {
        keymapping[i][j] = dvorakmapping[i][j]; // otherwise use dvorak
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


//LED brightness control

//Protocol toggle.

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


