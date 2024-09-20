
#include "components.h"

void setup() {
Serial.begin(115200);

pinMode(interrupt, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(interrupt), buttonPressed, RISING);

// inicializace komunikace se senzorem zrychleni a jeho nastaveni do mericiho modu
Wire.begin();
Wire.beginTransmission(ADXL345);
Wire.write(0x2D);
Wire.write(8);
Wire.endTransmission();
delay(10);

display.begin(SSD1306_SWITCHCAPVCC, 0x3C);      
display.display();
delay(1000);
openingScreen();
refreshTmp = micros();
}

void loop() {
  Serial.print("start : ");
  Serial.println(millis());
  
  for(int i = 0; i < 2; i++){
    Xsum -= trimData(readAccel(ADXL345,0x32) + OFFSET_X);
    Ysum += trimData(readAccel(ADXL345,0x34) + OFFSET_Y);
    
    if(abs(Xsum) > THRESHOLD_SUM){                        //pokud je hodnota Xsum vetsi nez THRESHOLD_SUM posuneme kurzor o +1 nebo -1 (závisí na znamenku Xsum) ve smeru osy x funkce move() provede posunuti pripadne vyhodnoti kolizi
     move(+1 | (Xsum >> (sizeof(int) * 8 - 1)),0);
     Xsum = 0;
    }
    if(abs(Ysum) > THRESHOLD_SUM){
      move(0,+1 | (Ysum >> (sizeof(int) * 8 - 1)));
      Ysum = 0;
    }
    delay(SUM_DELAY);
 }

  if(Xpos > 112 && Ypos > 48)
    finnishScreen();
  while( micros()-refreshTmp < REFRESH_TIME){
  }
  refreshTmp = micros();
  display.display();
  
}

void drawLevel(void){               //funkce na vykresleni urovne podle dat z array level
  display.clearDisplay();
  for(int i = 0; i < 64; i++){
    for(int j = 0; j < 128; j++){
      //if(pgm_read_byte_near(level+(i*128+j)))
      if(getPixelValue(j,i))
        display.drawPixel(j, i, WHITE);
  }
}

}

void drawDot(int x, int y){                 //kresli kurzor
  display.drawPixel(x, y, WHITE);
  display.drawPixel(x+1, y, WHITE);
  display.drawPixel(x-1, y, WHITE);
  display.drawPixel(x, y+1, WHITE);
  display.drawPixel(x, y-1, WHITE);
  
}
void undrawDot(int x, int y){               //vymaze kurzor
  display.drawPixel(x, y, BLACK);
  display.drawPixel(x+1, y, BLACK);
  display.drawPixel(x-1, y, BLACK);
  display.drawPixel(x, y+1, BLACK);
  display.drawPixel(x, y-1, BLACK);
  
}
int readAccel(int deviceAddress, int regAddress){     //funkce vraci hodnotu zrychleni ze senzoru, regAddress vyzaduje adresu registru (X osa - 0x32, Y osa - 0x34, Z osa - 0x36 )
  Wire.beginTransmission(deviceAddress);
  Wire.write(regAddress);
  Wire.endTransmission(false);
  Wire.requestFrom(deviceAddress, 2, true);
  return ( Wire.read()| Wire.read() << 8);

}

int trimData(int a){
  if(a < -TRIM1 || a > TRIM1)
    return -TRIM1;
  //if(a >= -TRIM2 || a <= TRIM2 )
   // return 0;
  return a;
}
void move(int x, int y){
    byte fail = 1;
  //  if(!pgm_read_byte_near(level+((2*y+Ypos)*128+Xpos)) && !pgm_read_byte_near(level+((2*y+Ypos)*128+Xpos+1)) && !pgm_read_byte_near(level+((2*y+Ypos)*128+Xpos-1))){
    if(!getPixelValue(Xpos,2*y+Ypos) && !getPixelValue(Xpos+1,2*y+Ypos) && !getPixelValue(Xpos-1,2*y+Ypos)){                                                                //kontrola kolize na ose y
      undrawDot(Xpos, Ypos);
      Ypos += y;
      drawDot(Xpos, Ypos);
      if(y)
      fail = 0;
    }
  //  if(!pgm_read_byte_near(level+((Ypos)*128+Xpos+2*x)) && !pgm_read_byte_near(level+((Ypos+1)*128+Xpos+2*x)) && !pgm_read_byte_near(level+((Ypos-1)*128+Xpos+2*x))){
    if(!getPixelValue(Xpos+2*x,Ypos) && !getPixelValue(Xpos+2*x,Ypos+1) && !getPixelValue(Xpos+2*x,Ypos-1)){                                                                //kontrola kolize na ose x
      undrawDot(Xpos, Ypos);
      Xpos += x;
      drawDot(Xpos, Ypos);
      if(x)
      fail = 0;
    }
    if(fail && collision)
    gameOver();
    return;
}

void gameOver(){                  //obrazovka ukazan kdyz se kurzor dotkne prekazky, neprobehne pokud je collision nastaveno na 0
  pause = 1;
  display.fillRect(4, 23, 120, 18, WHITE);
  display.setTextSize(2);     
  display.setTextColor(BLACK);
  display.setCursor(6, 25);
  display.write("GAME OVER!");
  display.display();
  while(pause){
  }
  countdown();
  return;
}
void buttonPressed(){     // funkce volana interruptem na pinu 2
  if(pause)
    pause = 0;
/*
  if(!pause && !menu)
    menu = 1;
*/
}
void countdown(){       // odpocet nastavi pocatecni pozici kurzoru a spusti casovac a hru
  Xpos = 5;
  Ypos = 5;
  display.setTextSize(3);
  display.setTextColor(BLACK);

  for(int i = 3; i > 0; i--){
    drawLevel();
    display.fillRect(52, 16, 24, 32, WHITE);     
    display.setCursor(56, 20);
    display.print(i);
    display.display();
    delay(960);         // zpozdeni mezi cisly v odpoctu 960 ms protoze display.display() trva cca 40 ms
  }
  
  display.clearDisplay();
  drawLevel();
  drawDot(Xpos, Ypos);
  display.display();
  timeStart = millis();
  return;
}

void finnishScreen(){     // obrazovka po dokonceni hry zastavi casovac a zobrazi dosazeny cas
  pause = 1;
  timeEnd = millis();
  display.fillRect(14, 18, 100, 27, WHITE);
  display.setTextSize(2);
  display.setTextColor(BLACK);
  display.setCursor(16, 20);
  display.write("FINISHED");
  display.setTextSize(1);
  display.setCursor(16, 36);
  display.write("Time: ");
  display.print((timeEnd-timeStart)/1000);
  display.write(" s");
  display.display();
  while(pause){
  }
  countdown();
  return;
}
void openingScreen(){                     //uvodni obrazovka pri zapnuti zarizeni
  pause = 1;
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(8, 20);
  display.write("TILT GAME");
  display.display();
  delay(500);
  display.setTextSize(1);
  display.setCursor(16, 36);
  display.write("Press any button");
  display.setCursor(16, 45);
  display.write("to start :)");
  display.display();
   while(pause){
  }
  countdown();
  return;
}
//v teto verzi menu neni
/*
void menuScreen(){
  pause = 1;
  display.clearDisplay();
  for(int i = 0; i < 4; i++){
    display.drawRect(10, 10+i*12 , 100, 12, WHITE);
    display.setCursor(18, 12+i*12);
    display.print(menuItems[i]);
  }
  display.display();
   while(pause){
  }
  countdown();
  menu = 0;
}
*/  
bool getPixelValue(int x, int y){                     // funkce dostane hodnotu pixelu na souradnicich x a y z levelu ulozeneho jako indexovana 1-bitova bitmapa (128x64)
  y = 63 - y;
  byte a = pgm_read_byte_near(level1+((y*128+x)/8));
  byte b = 0;
  // "ozrcadleni" bytu a
  if (a & 0x01) b |= 0x80;
  if (a & 0x02) b |= 0x40;
  if (a & 0x04) b |= 0x20;
  if (a & 0x08) b |= 0x10;
  if (a & 0x10) b |= 0x08;
  if (a & 0x20) b |= 0x04;
  if (a & 0x40) b |= 0x02;
  if (a & 0x80) b |= 0x01;
  
  return !(b >> ((x+y*128)%8) & 0x01);
}
