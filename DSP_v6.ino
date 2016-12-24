#include <avr/pgmspace.h>
#include <SPI.h> 
#include <LiquidCrystal.h>
//#include <LiquidCrystal_I2C.h>
#include <SD.h>
//#include <SoftwareSerial.h>

//#define SerialOutput
#define grblSerial Serial1

//#include <ClickEncoder.h>
//#include <TimerOne.h>

//#include <RTClib.h>
//RTC_DS1307 rtc;
//LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const uint8_t LCD_CHARS = 20, LCD_LINES = 4, SDcardChipSelectPin = 10;
char clearLine[] = "____________________";
#define RX_BUFFER_SIZE 128
#define Queue_len 10

const byte LT[8] PROGMEM = { B00111, B01111, B11111, B11111, B11111, B11111, B11111, B11111};
const byte UB[8] PROGMEM = { B11111, B11111, B11111, B00000, B00000, B00000, B00000, B00000};
const byte RT[8] PROGMEM = { B11100, B11110, B11111, B11111, B11111, B11111, B11111, B11111};
const byte LL[8] PROGMEM = { B11111, B11111, B11111, B11111, B11111, B11111, B01111, B00111};
const byte LB[8] PROGMEM = { B00000, B00000, B00000, B00000, B00000, B11111, B11111, B11111};
const byte LR[8] PROGMEM = { B11111, B11111, B11111, B11111, B11111, B11111, B11110, B11100};
const byte UMB[8] PROGMEM ={ B11111, B11111, B11111, B00000, B00000, B00000, B11111, B11111};
const byte LMB[8] PROGMEM ={ B11111, B00000, B00000, B00000, B00000, B11111, B11111, B11111};
const byte retArrow[8] PROGMEM = { 0x1,0x1,0x5,0x9,0x1f,0x8,0x4};
const byte escArrow[8] PROGMEM = { 0x4,0x8,0x1F,0x09,0x05,0x1,0x1F,0x0};

enum BtnPins_t { BtnPinMin = 29, /*BtnPinUp, BtnPinDown,*/ BtnPinLeft, BtnPinRight, BtnPinEnter, BtnPinESC, BtnPinMax };
//enum BtnStats_t{ Btn_Released, Btn_Pressed }; // KeyStat; 
LiquidCrystal lcd(9,8,7,6,5,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//LiquidCrystal_I2C lcd (0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
//Serial1 grblSerial(A1, A0); // RX, TX
//Sd2Card mySDcard;

String FName; // FileName[8]+"."+FileExt[3]+\0 
uint8_t Queue[Queue_len];
char space = ' ';

void createCharFromProgmem(uint8_t num, const byte memBuf[8])
{byte buf8[8];
 for (uint8_t i=0; i<8; i++) buf8[i] = pgm_read_byte_near(memBuf+i);
 lcd.createChar(num,buf8);
}

void showSplashScreen (uint8_t interval = 4) {
  // assignes each segment a write number
  createCharFromProgmem(8,LT);
  createCharFromProgmem(1,UB);
  createCharFromProgmem(2,RT);
  createCharFromProgmem(3,LL);
  createCharFromProgmem(4,LB);
  createCharFromProgmem(5,LR);
  createCharFromProgmem(6,UMB);
  createCharFromProgmem(7,LMB);
//  lcd.createChar(1,UB); lcd.createChar(2,RT); lcd.createChar(3,LL); lcd.createChar(4,LB); 
//  lcd.createChar(5,LR); lcd.createChar(6,UMB); lcd.createChar(7,LMB);
  lcd.clear();

  customG(0 + interval*0,0);    // displays custom 0 on the LCD
  customR(0 + interval*1,0);
  customB(0 + interval*2,0);
  customL(0 + interval*3,0);
  customS(0 + interval*4,0);
  lcd.setCursor(0,2); lcd.print(F("Standalone G-code")); 
  lcd.setCursor(0,3); lcd.print(F("sender by yurayerz")); 
  delay(1000); for (int i = 0; i < 20; i++) {lcd.scrollDisplayLeft(); delay(50);}
  delay(1000);
}


//============================================================================
void setup() {
//  lcd.init(); //lcd.backlight();
  lcd.begin (LCD_CHARS, LCD_LINES); // initialize the lcd 
  lcd.noCursor(); lcd.noBlink();

  showSplashScreen();

  BtnPinSetup(); // init buttons pins

#ifdef SerialOutput
  Serial.begin(9600);
  Serial.println(freeRAM());// delay(3000);
#endif

  grblSerial.begin(9600); grblSerial.setTimeout(5000);
  lcd.clear(); 
  createCharFromProgmem(7,retArrow);
  createCharFromProgmem(6,escArrow);

//  lcd.createChar(7, retArrow); lcd.createChar(6, escArrow);

}//============================================================================

//============================================================================
void loop(){
  lcd.clear(); 
  if (!init_grbl()) return;
  if (!beginSDcard()) return;

  lcd.setCursor(0,3); lcd.print(F("Free RAM:")); lcd.print(freeRAM()); 
  delay(1000);

  lcd.clear(); 
  if (SelectFileName(&FName))
//    sendFileToGRBL2(FName.c_str());
    sendFileToGRBL(FName.c_str());
  else {
    lcd.print(F("** Cancel select! **"));  
    delay(2000);
  }
}
//============================================================================

uint8_t init_grbl() {
//===================
//Reset grbl
  lcd.home(); lcd.print(F("Reset GRBL..."));
  lcd.setCursor(0,1); 
  grblSerial.write('\030$X\015');
  delay(3000); // Wait for grbl to initialize and flush startup text in serial input
  String buff = "";
  if (grblSerial.available()) {
//    buff = grblSerial.readStringUntil('\n'); buff.trim();
    while (grblSerial.available()) {
      char ch = grblSerial.read();
      if (buff.length() < LCD_CHARS and ch >= ' ') buff += ch; 
    }
//    lcd.print(buff.substring(0,LCD_CHARS-1));
    lcd.print(buff);
    lcd.home(); lcd.print(F("Reset GRBL done!"));
    delay (1000);
    return 1;
  }  
  else {
    lcd.print(F("GRBL not respond!"));
    delay (1000);
    return 0;
  }
/*
    recBuf.remove(LCD_CHARS - 5); recBuf = "> " + recBuf + "...";
    lcd.setCursor(0,1); lcd.print(recBuf);
    while (grblSerial.available()) grblSerial.read();
    lcd.setCursor(0,2); lcd.print("Reset GRBL: [Ctrl+X]");
    lcd.setCursor(0,3); 
    delay(3000);
    while (grblSerial.available()) lcd.write(grblSerial.read());
    return 1;
*/
}

/*uint8_t Debug_print_Queue (char *msg) {
  grblSerial.print (msg);
  for (uint8_t y=0; y < Queue_len; y++) 
    {grblSerial.print(Queue[y]); Serial.print(" ");}
  Serial.print(" ="); Serial.print(send_buff_len());
  Serial.println();
}
*/

uint8_t totalByteInQueue() { // посчитать сумму элементов массива
//===========================
  uint8_t sum = 0; 
  for (uint8_t y=0; y < Queue_len-1; y++) sum += Queue[y];
  return sum;
}

uint8_t queueCnt() { // посчитать количество ненулевых элементов в массиве
//===================
  uint8_t cnt = 0;
  for (uint8_t y=0; y < Queue_len-1; y++) cnt += !(Queue[y]==0);
  return cnt;
}

uint8_t enqueue(uint8_t Q) {
//===========================
//  Serial.print("Enqueue="); Serial.print(Q);
  uint8_t y=0;
  if ((totalByteInQueue() + Q) > RX_BUFFER_SIZE) return 0; // enqueue SUM over
  while ( y < Queue_len && Queue[y] != 0 ) {y++;}; // искать первый непустой элемент
  if (y >= Queue_len) 
    return 0; // enqueue LEN over
    else {Queue[y]=Q; return 1;}; 
}

void dequeue() {
//=======================================
  for (uint8_t y=0; y < Queue_len-1; y++) Queue[y]=Queue[y+1];
  Queue[Queue_len] = 0;
//  Debug_print_Queue("-= Dequeue =- ");
}  

uint8_t beginSDcard() {
//=======================================
  Sd2Card mySDcard;
  uint8_t retrySD = 0, cancelSD = 0;
  do { //* repeat retry init SD until cancel or success 
    lcd.setCursor(0,2); lcd.print(F("Init SD-card...     "));
    lcd.setCursor(0,3); lcd.print(clearLine);
    // Note that even if it's not used as the CS pin, the hardware SS pin 
    // (10 on most Arduino boards) must be left as an output 
    // or the SD library functions will not work. 
    pinMode(SDcardChipSelectPin, OUTPUT);
    if (mySDcard.init(SPI_HALF_SPEED, 10, 11, 12, 13))
//    if (SD.begin(10, 11, 12, 13)) 
      { lcd.setCursor(0,2); lcd.print(F("Init SD-card done!  "));
        lcd.setCursor(0,3); lcd.print(clearLine);
        retrySD = 0;
      } 
      else {
          lcd.setCursor(0,2); lcd.print(F("Init SD-card failed!"));
          lcd.setCursor(0,3); lcd.print(F("\006-escape    \007-retry"));
          //BtnPinSetup();
          retrySD = 0; cancelSD = 0;
          do {
            cancelSD = BtnIsPressed (BtnPinESC); 
            retrySD = BtnIsPressed (BtnPinEnter);
          } while (!(retrySD||cancelSD));
          if (cancelSD) 
            {waitForBtnReleased(BtnPinESC); return 0;}
            else waitForBtnReleased(BtnPinEnter);
      };
  } while (retrySD);
  delay(1000);
  return 1;
}

uint8_t SelectFileName (String *FName) {// Select file to send to GRBL-controller
//=======================================
  *FName = "";
  // List directory SD-card
  //-----------------------
  uint8_t prevF, nextF, selectF, cancelF;
  lcd.clear();
//  digitalWrite(SDcardChipSelectPin, LOW);
//  digitalWrite(SDcardChipSelectPin, HIGH);
//  if (!mySDcard.init(SPI_HALF_SPEED, SDcardChipSelectPin)) return 0;
//  if (!SD.begin(SDcardChipSelectPin)) return 0;
  SD.begin(10, 11, 12, 13);
  File root = SD.open("/");
/*
  if (!root.openNextFile()) {
    lcd.print("Empty directory!");
    delay(1000);
    root.close(); 
    return 0;
  }
  else root.rewindDirectory();
*/
//  Serial.println(root.name());
  while(true) {
    File dir = root.openNextFile();
    if (!dir) root.rewindDirectory(); //no more files
    if (!dir.isDirectory()) {
      lcd.setCursor(0,0); lcd.print(F("Select file:"));
      lcd.setCursor(0,1); lcd.print(clearLine); 
      lcd.setCursor(0,1); lcd.print(F(">")); lcd.print(dir.name()); 
      lcd.setCursor(0,2); lcd.print(clearLine); 
      lcd.setCursor(0,2); lcd.print(space); lcd.print(dir.size()/1024); lcd.print(F("Kb")); 
      lcd.setCursor(0,3); lcd.print(F("\177list\176  \007sel \006cancel")); //lcd.print(char(126));
      prevF = 0; nextF = 0; selectF = 0; cancelF = 0;
      do {
          cancelF = BtnIsPressed (BtnPinESC); 
          selectF = BtnIsPressed (BtnPinEnter);
          nextF   = BtnIsPressed (BtnPinRight); 
          prevF   = BtnIsPressed (BtnPinLeft);
      } while (!(nextF||prevF||selectF||cancelF));
      if (cancelF) {waitForBtnReleased(BtnPinESC); dir.close(); root.close(); return 0;};
      if (selectF) {waitForBtnReleased(BtnPinEnter); *FName = dir.name(); dir.close(); root.close(); return 1;};
      if (prevF) {waitForBtnReleased(BtnPinLeft); root.rewindDirectory();};
      if (nextF) waitForBtnReleased(BtnPinRight); 
    }
    dir.close();
  }
};

void sendFileToGRBL(const char *FName) {
//======================================
  File dFile = SD.open(FName,FILE_READ);
  for (uint8_t y=0; y < Queue_len; y++) Queue[y]=0; // queue init 

  if (!dFile) {
    lcd.clear(); lcd.print(F("error opening ")); lcd.print(FName); delay(3000); return;
  } 

  //+ Step mode ask... - включить пошаговый режим?
  lcd.clear(); lcd.print(F("Step mode?")); 
  lcd.setCursor(0,1); lcd.print(F(">")); lcd.print(FName); 
  lcd.setCursor(0,3); lcd.print(F("\007 yes           \006 no"));
  delay(100); // magic?
  uint8_t oneStepMode = 0, cancelF =0;
  do {
    cancelF = BtnIsPressed (BtnPinESC); 
    oneStepMode = BtnIsPressed (BtnPinEnter);
  } while (!(oneStepMode||cancelF));
  if (cancelF) waitForBtnReleased(BtnPinESC);
  if (oneStepMode) waitForBtnReleased(BtnPinEnter);
  //- Step mode ask...
  
  String outBuf = "", respBuf = "", statBuf = "";
  int recCnt = 0, respCnt = 0;

//  grblSerial.write('\030'); delay(3000); // Wait for grbl to initialize and flush startup text in serial input
//  while (grblSerial.available()) grblSerial.read(); // flush input from GRBL
  
  uint8_t showEnable = 1;

// main loop ******************************************** 
  lcd.clear();
#ifdef SerialOutput
  Serial.println(space); Serial.println(FName); Serial.println(F("==========================="));
#endif
  do {
//  while (dFile.available()) {
    if (showEnable) {
      // отобразить состояние очереди записей отправленных в ГРБЛ
      lcd.setCursor(0,2); lcd.print(clearLine); lcd.setCursor(0,2); 
      lcd.print(FName); 
      lcd.print(F(" Q:")); lcd.print(totalByteInQueue()); lcd.print(F("/")); lcd.print(queueCnt());
      displayRunMode(oneStepMode);
    }
  
    // если буфер пуст, считать в него очередную строку =>
    // (буфер опустошается только после реальной отправки строки в ГРБЛ или комментарии/пустая строка)
    if (outBuf.length() == 0) {
      if (dFile.available()) { // есть еще строки в файле?
        outBuf = dFile.readStringUntil('\n'); // прочитать строку
        outBuf.trim(); recCnt++; showEnable = 1; // трим, увеличить счетчик строк, разрешить отображение

#ifdef SerialOutput
        Serial.print(recCnt); Serial.print(F(":")); Serial.println(outBuf);
#endif

      }; 
    };

    // отобразить содержимое буфера на экране
    if (showEnable) {
      lcd.home(); lcd.print(clearLine); //lcd.print(clearLine);
      lcd.home(); lcd.print(recCnt); 
//      lcd.print(":"); lcd.print(outBuf.length()); 
      lcd.print(F(":")); lcd.print(outBuf.substring(0,14));
      showEnable = 0;
    };

    if (outBuf.startsWith(F("(")) || outBuf.length()==0) { // если строка - комментарий "(" или пустая, то ...
      outBuf = ""; // ... опустошить, увеличить стетчик ответов, пробросить цикл
      respCnt++; 
      lcd.setCursor(0,1); lcd.print(respCnt); lcd.print(F(":Skipped..."));
      showEnable = 1;
//++      delay(1000);
//      continue;
    };
    
    uint8_t stepS = 0, runS = 0, pauseS = 0, retryS = 0;
    pauseS = BtnIsPressed (BtnPinESC); 
    runS   = BtnIsPressed (BtnPinEnter);
    stepS  = BtnIsPressed (BtnPinRight); 
    if (oneStepMode) { //... и если включен пошаговый режим, ждать ответа пользователя (нажатия кнопок)
      uint8_t stepS = 0, runS = 0, pauseS = 0;
      lcd.cursor(); lcd.blink();
      do {
        pauseS = BtnIsPressed (BtnPinESC); 
        runS   = BtnIsPressed (BtnPinEnter);
        stepS  = BtnIsPressed (BtnPinRight); 
        retryS = BtnIsPressed (BtnPinLeft); 
      } while (!(stepS||runS||pauseS||retryS));
      if (retryS) waitForBtnReleased(BtnPinLeft);
      if (stepS) waitForBtnReleased(BtnPinRight);
      if (runS) {waitForBtnReleased(BtnPinEnter); oneStepMode = 0; lcd.noCursor(); lcd.noBlink();};
      if (pauseS) {waitForBtnReleased(BtnPinESC); break;};
    }
    else { // не пошаговый режим...
      if (pauseS || runS || stepS || retryS) {
        if (retryS) waitForBtnReleased(BtnPinLeft);
        if (stepS) {waitForBtnReleased(BtnPinRight); oneStepMode = 1;};
        if (runS) {waitForBtnReleased(BtnPinEnter); oneStepMode = 0;};
        if (pauseS) {waitForBtnReleased(BtnPinESC); break;};
      }
    }

    // если постановка в очередь удачна (количество элементов и общая длина очереди позволяет), 
    // то отправить строку в ГРБЛ и очистить буфер (пустой буфер - признак успешной передачи)
    if (outBuf.length() != 0) {
      if (enqueue(outBuf.length()+2)) { //+++ +2!
        grblSerial.print(outBuf + '\n'); grblSerial.flush();
        delay(50);
        outBuf=""; showEnable = 1; 
      } /****/
    }
//    if (BtnIsPressed(BtnPinESC)) oneStepMode=1;

    //- read GRBL responce
    //====================
//++    delay(50);
    while (grblSerial.available()) {
      respBuf = grblSerial.readStringUntil('\n');
      respCnt++; respBuf.trim();

#ifdef SerialOutput
      Serial.print(respCnt); Serial.print(F(":")); Serial.println(respBuf);
#endif

      lcd.setCursor(0,1); lcd.print(clearLine);
      lcd.setCursor(0,1); lcd.print(respCnt); 
      lcd.print(':'); lcd.print(respBuf); 
      dequeue(); respBuf = ""; 
      showEnable = 1;

/*
      char ch = grblSerial.read();
      if (ch == '\n') {
        respCnt++; respBuf.trim();
        Serial.print(respCnt); Serial.print(F(":")); Serial.println(respBuf);
        lcd.setCursor(0,1); lcd.print(clearLine);
        lcd.setCursor(0,1); lcd.print(respCnt); 
        lcd.print(':'); lcd.print(respBuf); 
        dequeue(); respBuf = ""; 
        showEnable = 1;
//        delay(10);
      }
      else respBuf += ch;
*/  
    }
  } while ( dFile.available() || queueCnt() != 0 );
// main loop ******************************************** 
  if (dFile.available()) {
    lcd.setCursor(0,3); lcd.print(F("*** Cancelled!!! ***")); delay(3000);
  }
  else {
    lcd.setCursor(0,3); lcd.print(F("**** Job done! *****")); delay(3000);
  } 
  dFile.close();
  lcd.noCursor(); lcd.noBlink();
  delay(3000);
  lcd.clear(); 
}
//***********************************************************************

void displayRunMode(uint8_t oneStepMode) {
//==================
  if (oneStepMode) 
    { lcd.setCursor(0,3); lcd.print(F("Stp>\176Step \007Run \006Canc"));}
    else {
      lcd.setCursor(0,3); lcd.print(F("Run> \176Pause  \006Cancel"));};
}

void BtnPinSetup() {
//==================
  for (uint8_t i = BtnPinMin+1; i < BtnPinMax; i++) pinMode(i, INPUT_PULLUP);
}

uint8_t BtnIsPressed (uint8_t BtnPin) {
//=====================================
//  uint8_t b = (digitalRead(BtnPin));
  return !digitalRead(BtnPin);
}

void waitForBtnReleased(uint8_t BtnPin) {
//=======================================
  while (BtnIsPressed(BtnPin));
}
// custom LARGE FONT
//=======================================
void customB(uint8_t xx, uint8_t yy)
{ lcd.setCursor(xx, yy);   lcd.write(255); lcd.write(6); lcd.write(5);
  lcd.setCursor(xx, yy+1); lcd.write(255); lcd.write(7); lcd.write(2);
}
void customG(uint8_t xx, uint8_t yy)
{ lcd.setCursor(xx,yy);   lcd.write(8); lcd.write(1); lcd.write(1);
  lcd.setCursor(xx,yy+1); lcd.write(3); lcd.write(4); lcd.write(2);
}
void customL(uint8_t xx, uint8_t yy)
{ lcd.setCursor(xx,yy);   lcd.write(255);
  lcd.setCursor(xx,yy+1); lcd.write(255); lcd.write(4); lcd.write(4);
}
void customR(uint8_t xx, uint8_t yy)
{ lcd.setCursor(xx,yy);   lcd.write(255); lcd.write(6);   lcd.write(2);
  lcd.setCursor(xx,yy+1); lcd.write(255); lcd.write(254); lcd.write(2); 
}
void customS(uint8_t xx, uint8_t yy)
{ lcd.setCursor(xx,yy);   lcd.write(8); lcd.write(6); lcd.write(6);
  lcd.setCursor(xx,yy+1); lcd.write(7); lcd.write(7); lcd.write(5);
}
  
int freeRAM () 
{ extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
/*
  rtc.begin();
  if (! rtc.isrunning()) {lcd.println("RTC is NOT running!"); return;};
  DateTime now = rtc.now();
  lcd.print(now.year(), DEC);
  lcd.print('/'); lcd.print(now.month(), DEC);
  lcd.print('/'); lcd.print(now.day(), DEC);
  lcd.print(' '); 
  lcd.print(now.hour(), DEC);   lcd.print(':');
  lcd.print(now.minute(), DEC); lcd.print(':');
  lcd.print(now.second(), DEC); //lcd.println();
  lcd.noCursor();
*/

