
#include <HID-Project.h>
#include <Keypad.h>
#include <Encoder.h>
#include <TimerMs.h>
#include "FastLED.h"          // библиотека для работы с лентой

#define LED_COUNT 17          // число светодиодов в кольце/ленте
#define LED_DT 10             // пин, куда подключен DIN ленты

int ledMode = 2;
int prevledMode, maxledMode = 10;
// цвета мячиков для режима
byte ballColors[3][3] = {
  {0xff, 0, 0},
  {0xff, 0xff, 0xff},
  {0   , 0   , 0xff},
};

// ---------------СЛУЖЕБНЫЕ ПЕРЕМЕННЫЕ-----------------
int BOTTOM_INDEX = 0;        // светодиод начала отсчёта
int TOP_INDEX = int(LED_COUNT / 2);
int EVENODD = LED_COUNT % 2;
struct CRGB leds[LED_COUNT];
int ledsX[LED_COUNT][3];     //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, MARCH, ETC)

int thisdelay = 20;          //-FX LOOPS DELAY VAR
int thisstep = 10;           //-FX LOOPS DELAY VAR
int thishue = 0;             //-FX LOOPS DELAY VAR
int thissat = 255;           //-FX LOOPS DELAY VAR

int thisindex = 0;
int thisRED = 0;
int thisGRN = 0;
int thisBLU = 0;

int idex = 0;                //-LED INDEX (0 to LED_COUNT-1
int ihue = 0;                //-HUE (0-255)
int ibright = 0;             //-BRIGHTNESS (0-255)
int isat = 0;                //-SATURATION (0-255)
int bouncedirection = 0;     //-SWITCH FOR COLOR BOUNCE (0-1)
float tcount = 0.0;          //-INC VAR FOR SIN LOOPS
int lcount = 0;              //-ANOTHER COUNTING VAR
// ---------------СЛУЖЕБНЫЕ ПЕРЕМЕННЫЕ-----------------

#include "GyverOLED.h"
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;

// This library is for interfacing with the 3x4 Matrix
// Can be installed from the library manager, search for "keypad"
// and install the one by Mark Stanley and Alexander Brevig
// https://playground.arduino.cc/Code/Keypad/

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

const int MuteButton = 16;

int modePushCounter = 0;       // counter for the number of button presses

int buttonState = 0;           // current state of the button
int lastButtonState = 0;       // previous state of the button

char keys[ROWS][COLS] = {
  {'M', '<', 'P', '>'},  //  the keyboard hardware is  a 3x4 grid... 
  {'L', '7', '8', '9'},
  {'N', '4', '5', '6'},
  {'0', '1', '2', '3'},  // these values need  to be single char, so...
};
 byte rowPins[ROWS] = {1, 0, 4, 5 };    //connect to the row pinouts of the keypad
 byte colPins[COLS] = {6, 7, 8, 9 };  //connect to the column pinouts of the keypad
 Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
 Encoder RotaryEncoder(14, 15);
 long positionEncoder  = -999;

int horzPin = A1;  // Analog output of horizontal joystick pin
int vertPin = A0;  // Analog output of vertical joystick pin
int selPin = A2;  // select button pin of joystick

int vertZero, horzZero;  // Stores the initial value of each axis, usually around 512
int vertValue, horzValue;  // Stores current analog output of each axis
const int sensitivity = 200;  // Higher sensitivity value = slower mouse, should be <= about 500
int mouseClickFlag = 0;

//int invertMouse = 1;        //Invert joystick based on orientation
int invertMouse = -1;         //Noninverted joystick based on orientation

TimerMs effekt_timer(20, 1, 1);
TimerMs msg_timer(1000, 1, 1);
TimerMs enc_timer(1, 1, 1);

const int numStates = 4; // Кількість рівнів клавіатури 
int currentState = 0;    // Поточний рівень

int dopvar=1;
int msg=0;

void one_color_all(int cred, int cgrn, int cblu) {       //-SET ALL LEDS TO ONE COLOR
  for (int i = 0 ; i < LED_COUNT; i++ ) {
    leds[i].setRGB( cred, cgrn, cblu);
  }
}

void one_color_allHSV(int ahue) {    //-SET ALL LEDS TO ONE COLOR (HSV)
  for (int i = 0 ; i < LED_COUNT; i++ ) {
    leds[i] = CHSV(ahue, thissat, 255);
  }
}


void setup() {
  Serial.begin(9600); // initialize serial communication:
  pinMode(MuteButton, INPUT_PULLUP);
  pinMode(horzPin, INPUT);  // Set both analog pins as inputs
  pinMode(vertPin, INPUT);
  pinMode(selPin, INPUT);  // set button select pin as input
  digitalWrite(selPin, HIGH);  // Pull button select pin high
  vertZero = analogRead(vertPin);  // get the initial values
  horzZero = analogRead(horzPin);  // Joystick should be in neutral position when reading these
  Mouse.begin(); //Init mouse emulation
  Keyboard.begin();
  Consumer.begin();
  LEDS.setBrightness(60);  // ограничить максимальную яркость

  LEDS.addLeds<WS2811, LED_DT, GRB>(leds, LED_COUNT);  // настрйоки для нашей ленты (ленты на WS2811, WS2812, WS2812B)
  one_color_all(0, 0, 0);          // погасить все светодиоды
  LEDS.show();
  oled.init();              // инициализация
  // ускорим вывод, ВЫЗЫВАТЬ ПОСЛЕ oled.init()!!!
  Wire.setClock(400000L);   // макс. 800'000
  oled.clear();
  oled.setScale(3);
  oled.setCursor(5, 3);
  oled.println("MINIPAD");
  delay(1000); 
  effekt_timer.setPeriodMode();
  msg_timer.setPeriodMode();
  Serial.println("Start MicroPad");
  lcdMsgMode();

}


void loop() {
  encoder();
  checkMute();
  mouse_update();
  if (effekt_timer.tick())led_effect();
  if (msg_timer.tick()) if (msg) {lcdMsgMode();}

  char key = keypad.getKey();

  if(key) {
   switch(key){
    //Обробка мультимедійної клавіші "Назад"
    case '<':
     Consumer.write(MEDIA_PREV); 
     lcdMsg("PREV");
    break;
    //Обробка мультимедійної клавіші "Відтворити/паузв"
    case 'P':
      Consumer.write(MEDIA_PLAY_PAUSE);
      lcdMsg("PLAY_PAUSE");
    break;
    //Обробка мультимедійної клавіші "Наступна"
    case '>':
      Consumer.write(MEDIA_NEXT);
      lcdMsg("NEXT");
    break;
    //Обробка натискання на клавішу "Підсвітка"
    case 'L':
      ledMode++;
      if(ledMode>maxledMode) ledMode=1;
      change_mode(ledMode);
      lcdMsgLED();
    break;
    //Обробка натискання на клавішу "Радіація"
    case 'N':
      if (ledMode<10) {prevledMode=ledMode;ledMode=10;oled.setContrast(0);} else {ledMode=prevledMode;oled.setContrast(255);}
      change_mode(ledMode);
    break;
    case 'M':
     ChangeState();
    break;
    
    default:
        switch(currentState){
          case 0:
            Layout1(key);
            break;
          case 1:
            Layout2(key);
            break;
          case 2:
            Layout3(key);
            break;
          case 3: 
            Layout4(key);
            break;
        }
     delay(100); Keyboard.releaseAll();
   }
  }
}

//Відображення повідомлення про поточний рівень клавіатури
void lcdMsgMode(){
  oled.clear();
  oled.setScale(3);
  oled.setCursorXY(5, 20);
  oled.print("LEVEL:");
  oled.print(currentState+1);
  msg=0;
}

//Відображення повідомлення про режим підсвітки
void lcdMsgLED(){
  oled.clear();
  oled.setScale(2);
  oled.setCursorXY(1, 24);
  oled.print("LED MODE:");
  oled.print(ledMode);
  msg=1;
  msg_timer.setTime(1000);
  msg_timer.start();
}

void lcdMsg(String smsg){
  int xx;
  xx = (smsg.length()/2)*12;
  oled.clear();
  oled.setScale(2);
  oled.setCursorXY((OLED_WIDTH/2)-xx, 24);
  oled.print(smsg);
  msg=1;
  msg_timer.setTime(1000);
  msg_timer.start();
}

// Зміна рівня клавіатури
void ChangeState(){
  currentState++;
  if (currentState == numStates){
    currentState = 0;
  }
  lcdMsgMode();
}

// Перший рівень клавіатури (дубляж цифрової клавіатури)
void Layout1(char button){
   //int a;
   //a=int(button);
   if ((button>='0')&&(button<='9')) lcdMsg(String(button)); 
  switch(button){
    case '0':
      Keyboard.print('0');
      break;
    case '1':
      Keyboard.print('1');
      break;
    case '2':
      Keyboard.print('2');
      break;
    case '3':
      Keyboard.print('3');
      break;
    case '4':
      Keyboard.print('4');
      break;
    case '5':
      Keyboard.print('5');
      break;
    case '6':
      Keyboard.print('6');
      break;
    case '7':
      Keyboard.print('7');
      break;
    case '8':
      Keyboard.print('8');
      break;
    case '9':
      Keyboard.print('9');
      break;
  };
}

// Другий рівень клавіатури
void Layout2(char button){
  switch(button){
    case '0':
      Keyboard.press(KEY_LEFT_CTRL);  
      Keyboard.print('c');
      lcdMsg("CTRL+C"); //Скопировать
      break;
    case '1':
      Keyboard.press(KEY_LEFT_CTRL);  
      Keyboard.print('v');
      lcdMsg("CTRL+V");  //Вставить
      break;
    case '2':
      Keyboard.press(KEY_LEFT_CTRL);  
      Keyboard.print('x');
      lcdMsg("CTRL+X"); //Вырезать
      break;
    case '3':
      Keyboard.press(KEY_ESC);
      lcdMsg("ESC"); 
      break; //Ескейп
    case '4':
      Keyboard.press(KEY_PRINT);
      lcdMsg("PRINT SCR"); //Вызов скриншотилки
      break;
    case '5':
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.print('v');
      lcdMsg("Win+V"); //Буфер
      break;
    case '6':
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.print('v');
      lcdMsg("CTRL+ALT+V"); //Вставить текст без форматирования
      break;
    case '7':
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.print('s');
      lcdMsg("A+S+S"); //Вызов контекстной строки
      break;
    case '8':
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.print('T');
      lcdMsg("Win+S+T"); //Копирование текста
      break;
    case '9': // згорнути/розгорнути вікно
      if (dopvar){
       Keyboard.press(KEY_LEFT_GUI);
       Keyboard.print('m');
       lcdMsg("DESKTOP"); 
       dopvar=0;
      } else
      {
       Keyboard.press(KEY_LEFT_GUI);
       Keyboard.press(KEY_LEFT_SHIFT);
       Keyboard.print('m');
       lcdMsg("DESKTOP");  
       dopvar=1;
      }

      break;
  };
}

// Третій рівень клавіатури
void Layout3(char button){
  switch(button){
     case '0':
      Keyboard.press(KEY_LEFT_CTRL);  
      Keyboard.print('c');
      lcdMsg("CTRL+C"); //Скопировать
      break;
    case '1':
      Keyboard.press(KEY_LEFT_CTRL);  
      Keyboard.print('v');
      lcdMsg("CTRL+V");  //Вставить
      break;
    case '2':
      Keyboard.press(KEY_LEFT_CTRL);  
      Keyboard.print('d');
      lcdMsg("Dublicate"); //Дублировать
      break;
    case '3':
      Keyboard.press(KEY_LEFT_CTRL);  
      Keyboard.print('y');
      lcdMsg("CTRL+Y"); //Удалить строчку
    case '4':
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.print('+');
      lcdMsg("Expand"); //Удалить строчку
      break;
    case '5':
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.print('-');
      lcdMsg("Collapse"); //Удалить строчку
      break;
    case '6':
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.print('/');
      lcdMsg("Comment"); //Коментарии
      break;
    case '7':
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.print('F');
      lcdMsg("Find"); //Поиск
      break;
    case '8':
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.press(KEY_F10);
      lcdMsg("Run"); //Запуск сервера
      break;
    case '9':
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(KEY_F12);
      lcdMsg("Tio"); //Клиент ТИО
      break;
  };
}

// Четвертий рівень клавіатури
void Layout4(char button){
  switch(button){
    case '0':
      Keyboard.print('0');
      break;
    case '1':
      Keyboard.print('1');
      break;
    case '2':
      Keyboard.print('2');
      break;
    case '3':
      Keyboard.print('3');
      break;
    case '4':
      Keyboard.print('4');
      break;
    case '5':
      Keyboard.print('5');
      break;
    case '6':
      Keyboard.print('6');
      break;
    case '7':
      Keyboard.print('7');
      break;
    case '8':
      Keyboard.print('8');
      break;
    case '9':
      Keyboard.print('9');
      break;
  };
}

//Обробка робрти енкодера 
void encoder(){
  long newPos = RotaryEncoder.read()/4;
  // Збільшити гучність
  if (newPos != positionEncoder && newPos > positionEncoder) {
    positionEncoder = newPos;
    //Serial.println("volume up");
    Consumer.write(MEDIA_VOLUME_UP);
    //lcdMsg("VOL. UP");
    }
  // Зменшити гучність
  if (newPos != positionEncoder && newPos < positionEncoder) {
    positionEncoder = newPos;
    //Serial.println("volume down");
    Consumer.write(MEDIA_VOLUME_DOWN);                      
    //lcdMsg("VOL. DOWN");
    }
}

// Обробка натиску на енкодер
void checkMute(){
  buttonState = digitalRead(MuteButton);
  if (buttonState != lastButtonState) {
    if (buttonState == LOW) { 
     lcdMsg("VOL. MUTE");
     //Serial.println("volume mute"); 
     Consumer.write(MEDIA_VOLUME_MUTE);//Вимкнути звук
    } 
    delay(50);
  }
  lastButtonState = buttonState;  // save the current state as the last state, for next time through the loop
}

// Обробка джойстика для емуляції "миші"
void mouse_update()
{
  vertValue = analogRead(vertPin) - vertZero;  // read vertical offset
  horzValue = analogRead(horzPin) - horzZero;  // read horizontal offset

  if (vertValue != 0)
    Mouse.move(0, (1 * (vertValue / sensitivity)), 0); // move mouse on y axis
  if (horzValue != 0)
    Mouse.move((invertMouse * (horzValue / sensitivity)), 0, 0); // move mouse on x axis

  if ((digitalRead(selPin) == 0) && (!mouseClickFlag))  // if the joystick button is pressed
  {
    mouseClickFlag = 1;
    Mouse.press(MOUSE_LEFT);  // click the left button down
  }
  else if ((digitalRead(selPin)) && (mouseClickFlag)) // if the joystick button is not pressed
  {
    mouseClickFlag = 0;
    Mouse.release(MOUSE_LEFT);  // release the left button
  }
}

// Обробка вибору режиму підсвітки
void change_mode(int newmode) {
  thissat = 255;
  switch (newmode) {
    case 1: thisdelay = 20; break;                      //---STRIP RAINBOW FADE
    case 2: thisdelay = 20; thisstep = 10; break;       //---RAINBOW LOOP
    case 3: thisdelay = 20; break;                      //---RANDOM BURST
    case 4: thisdelay = 15; thishue = 0; break;        //---PULSE COLOR SATURATION
    case 6: thisdelay = 50; thisstep = 15; break;      //---VERITCAL RAINBOW
    case 7: thisdelay = 25; thishue = 0; break;        //---RGB PROPELLER
    case 8: thisdelay = 5; break;                      //---NEW RAINBOW LOOP
    case 9: thisdelay = 45; break;                     // Fire
  }
  bouncedirection = 0;
  one_color_all(0, 0, 0);
  ledMode = newmode;
  effekt_timer.setTime(thisdelay);
}

// Відображення підсвітки
void led_effect(){
  switch (ledMode) {
    case 10: one_color_all(0, 0, 0);
             FastLED.show();
             break;                            // пазуа
    case  1: rainbow_fade(); break;            // плавная смена цветов всей ленты
    case  2: rainbow_loop(); break;            // крутящаяся радуга
    case  3: random_burst(); break;            // случайная смена цветов
    case  4: pulse_one_color_all_rev(); break; // пульсация со сменой цветов
    case  5: flame(); break;                   // эффект пламени
    case  6: rainbow_vertical(); break;        // радуга в вертикаьной плоскости (кольцо)
    case  7: rgb_propeller(); break;           // RGB пропеллер
    case  8: new_rainbow_loop(); break;        // крутая плавная вращающаяся радуга
    case  9: Fire(55, 120, thisdelay); break;  // линейный огонь
  }
}

//------------------------------------- UTILITY FXNS --------------------------------------
//---SET THE COLOR OF A SINGLE RGB LED
void set_color_led(int adex, int cred, int cgrn, int cblu) {
  leds[adex].setRGB( cred, cgrn, cblu);
}

//---FIND INDEX OF HORIZONAL OPPOSITE LED
int horizontal_index(int i) {
  //-ONLY WORKS WITH INDEX < TOPINDEX
  if (i == BOTTOM_INDEX) {
    return BOTTOM_INDEX;
  }
  if (i == TOP_INDEX && EVENODD == 1) {
    return TOP_INDEX + 1;
  }
  if (i == TOP_INDEX && EVENODD == 0) {
    return TOP_INDEX;
  }
  return LED_COUNT - i;
}

//---FIND INDEX OF ANTIPODAL OPPOSITE LED
int antipodal_index(int i) {
  int iN = i + TOP_INDEX;
  if (i >= TOP_INDEX) {
    iN = ( i + TOP_INDEX ) % LED_COUNT;
  }
  return iN;
}

//---FIND ADJACENT INDEX CLOCKWISE
int adjacent_cw(int i) {
  int r;
  if (i < LED_COUNT - 1) {
    r = i + 1;
  }
  else {
    r = 0;
  }
  return r;
}

//---FIND ADJACENT INDEX COUNTER-CLOCKWISE
int adjacent_ccw(int i) {
  int r;
  if (i > 0) {
    r = i - 1;
  }
  else {
    r = LED_COUNT - 1;
  }
  return r;
}

void copy_led_array() {
  for (int i = 0; i < LED_COUNT; i++ ) {
    ledsX[i][0] = leds[i].r;
    ledsX[i][1] = leds[i].g;
    ledsX[i][2] = leds[i].b;
  }
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
}

void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < LED_COUNT; i++ ) {
    setPixel(i, red, green, blue);
  }
  FastLED.show();
}

void rainbow_fade() {                         //-m2-FADE ALL LEDS THROUGH HSV RAINBOW
  ihue++;
  if (ihue > 255) {
    ihue = 0;
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(ihue, thissat, 255);
  }
  LEDS.show();
  //delay(thisdelay);
}

void rainbow_loop() {                        //-m3-LOOP HSV RAINBOW
  idex++;
  ihue = ihue + thisstep;
  if (idex >= LED_COUNT) {
    idex = 0;
  }
  if (ihue > 255) {
    ihue = 0;
  }
  leds[idex] = CHSV(ihue, thissat, 255);
  LEDS.show();
  //delay(thisdelay);
}

void random_burst() {                         //-m4-RANDOM INDEX/COLOR
  idex = random(0, LED_COUNT);
  ihue = random(0, 255);
  leds[idex] = CHSV(ihue, thissat, 255);
  LEDS.show();
  //delay(thisdelay);
}

void pulse_one_color_all_rev() {           //-m11-PULSE SATURATION ON ALL LEDS TO ONE COLOR
  if (bouncedirection == 0) {
    isat++;
    if (isat >= 255) {
      bouncedirection = 1;
    }
  }
  if (bouncedirection == 1) {
    isat = isat - 1;
    if (isat <= 1) {
      bouncedirection = 0;
    }
  }
  for (int idex = 0 ; idex < LED_COUNT; idex++ ) {
    leds[idex] = CHSV(thishue, isat, 255);
  }
  LEDS.show();
  //delay(thisdelay);
}

void flame() {                                    //-m22-FLAMEISH EFFECT
  int idelay = random(0, 35);
  float hmin = 0.1; float hmax = 45.0;
  float hdif = hmax - hmin;
  int randtemp = random(0, 3);
  float hinc = (hdif / float(TOP_INDEX)) + randtemp;
  int ihue = hmin;
  for (int i = 0; i <= TOP_INDEX; i++ ) {
    ihue = ihue + hinc;
    leds[i] = CHSV(ihue, thissat, 255);
    int ih = horizontal_index(i);
    leds[ih] = CHSV(ihue, thissat, 255);
    leds[TOP_INDEX].r = 255; leds[TOP_INDEX].g = 255; leds[TOP_INDEX].b = 255;
    LEDS.show();
    effekt_timer.setTime(idelay);
  }
}

void rainbow_vertical() {                        //-m23-RAINBOW 'UP' THE LOOP
  idex++;
  if (idex > TOP_INDEX) {
    idex = 0;
  }
  ihue = ihue + thisstep;
  if (ihue > 255) {
    ihue = 0;
  }
  int idexA = idex;
  int idexB = horizontal_index(idexA);
  leds[idexA] = CHSV(ihue, thissat, 255);
  leds[idexB] = CHSV(ihue, thissat, 255);
  LEDS.show();
  //delay(thisdelay);
}

void rgb_propeller() {                           //-m27-RGB PROPELLER
  idex++;
  int ghue = (thishue + 80) % 255;
  int bhue = (thishue + 160) % 255;
  int N3  = int(LED_COUNT / 3);
  int N6  = int(LED_COUNT / 6);
  int N12 = int(LED_COUNT / 12);
  for (int i = 0; i < N3; i++ ) {
    int j0 = (idex + i + LED_COUNT - N12) % LED_COUNT;
    int j1 = (j0 + N3) % LED_COUNT;
    int j2 = (j1 + N3) % LED_COUNT;
    leds[j0] = CHSV(thishue, thissat, 255);
    leds[j1] = CHSV(ghue, thissat, 255);
    leds[j2] = CHSV(bhue, thissat, 255);
  }
  LEDS.show();
  //delay(thisdelay);
}

void new_rainbow_loop() {                      //-m88-RAINBOW FADE FROM FAST_SPI2
  ihue -= 1;
  fill_rainbow( leds, LED_COUNT, ihue );
  LEDS.show();
  //delay(thisdelay);
}

//---------------------------------линейный огонь-------------------------------------
void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[LED_COUNT];
  int cooldown;

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < LED_COUNT; i++) {
    cooldown = random(0, ((Cooling * 10) / LED_COUNT) + 2);

    if (cooldown > heat[i]) {
      heat[i] = 0;
    } else {
      heat[i] = heat[i] - cooldown;
    }
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = LED_COUNT - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if ( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160, 255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for ( int j = 0; j < LED_COUNT; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  FastLED.show();
  effekt_timer.setTime(SpeedDelay);
}

void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature / 255.0) * 191);

  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252

  // figure out which third of the spectrum we're in:
  if ( t192 > 0x80) {                    // hottest
    setPixel(Pixel, 255, 255, heatramp);
  } else if ( t192 > 0x40 ) {            // middle
    setPixel(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    setPixel(Pixel, heatramp, 0, 0);
  }
}
