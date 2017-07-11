#include <UTFTGLUE.h>
#include <Adafruit_GFX.h>


#define LCD_CS A3
#define LCD_RS A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

UTFTGLUE tft(LCD_RS, LCD_WR, LCD_CS, LCD_RESET, LCD_RD);

#define txtLINE0   25
#define txtLINE1   45
#define txtLINE2   65
#define txtLINE3   85

const int LCD_WIDTH = 320;  //Panjang/Lebar LCD dalam pixel
const int LCD_HEIGHT = 240; //Tinggi LCD dalam pixel
const int SAMPLES = 270; 
const int DOTS_DIV = 30;//270/30=9div/sample

const int ad_sw = A14;                   // Analog 3 pin for switches
const int ad_ch0 = A10;                   // Analog 6 pin for channel 0
const int ad_ch1 = A11;                   // Analog 5 pin for channel 1
const unsigned long VREF[] = {150, 300, 750, 1500, 3000}; // reference voltage 5.0V ->  150 :   1V/div range (100mV/dot)
                                        // It means 5.0 * DOTS_DIV = 150. Use 4.9 if reference voltage is 4.9[V]
                                        //                        -> 300 : 0.5V/div
                                        //                        -> 750 : 0.2V/div
                                        //                        ->1500 : 100mV/div
                                        //                       -> 3000 :  50mV/div
const int MILLIVOL_per_dot[] = {33, 17, 6, 3, 2}; // mV/dot
const int MODE_ON = 0;
const int MODE_INV = 1;
const int MODE_OFF = 2;
const char *Modes[] = {"NORMAL", "INVERT", "OFF"};
const int TRIG_AUTO = 0;
const int TRIG_NORM = 1;
const int TRIG_SCAN = 2;
const int TRIG_ONE  = 3;
const char *TRIG_Modes[] = {"Auto", "Norm", "Scan", "One"};
const int TRIG_E_UP = 0;
const int TRIG_E_DN = 1;
#define RATE_MIN 0
#define RATE_MAX 13
const char *Rates[] = {"F1-1", "F1-2 ", "F2  ", "5ms", "10ms", "20ms", "50ms", "0.1s", "0.2s", "0.5s", "1s", "2s", "5s", "10s"};
#define RANGE_MIN 0
#define RANGE_MAX 4
const char *Ranges[] = {"1V", "0.5V", "0.2V", "0.1V", "50mV"};
unsigned long startMillis;
byte data[4][SAMPLES];                   // keep twice of the number of channels to make it a double buffer
byte sample=0;                           // index for double buffer
unsigned int ain;

// Declare variables and set defaults here
// Note: only ch1 is available with Aitendo's parallel 320x240 TFT LCD  
byte range0 = RANGE_MIN, ch0_mode = MODE_OFF;  // CH0
short ch0_off = 204;
byte range1 = RANGE_MIN, ch1_mode = MODE_ON;  // CH1
short ch1_off = 204;
byte rate = 3;                                // sampling rate
byte trig_mode = TRIG_AUTO, trig_lv = 30, trig_edge = TRIG_E_UP, trig_ch = 1; // trigger settings
byte Start = 1;  // Start sampling
byte menu = 0;  // Default menu
uint16_t identifier;


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Define colors here

//color address harus diberikan saat menggunakan lib SPFD5408_Adafruit_TFTLCD.h
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define BGCOLOR   BLACK    //Warna Background
#define GRIDCOLOR GREEN    //Warna Grid
#define CH1COLOR  RED      //Warna Gelombang Channel 1
#define CH2COLOR  MAGENTA  //Warna Gelombang Channel 2

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  tft.begin(9600);
  identifier = tft.readID();
  tft.begin(identifier);
  
  tft.setRotation(1);
  tft.fillScreen(BGCOLOR);
  
  tft.setColor(CYAN);
  tft.setTextSize(2);
  tft.print("Spectrum Analyzer",60,100);
  delay(1000);

  tft.fillScreen(BGCOLOR);   
  Serial.begin(9600);
  
  DrawGrid();
  DrawText();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void CheckSW() 
{
  static unsigned short oain[2];
  static unsigned long Millis = 0, oMillis = 0;
  unsigned long ms;
  unsigned int ain=analogRead(ad_sw);

  return;
 
  ms = millis();
  if ((ms - Millis)<5)
   return;
  Millis = ms;
  
  if (!(abs(oain[0] - oain[1])>10 && abs(oain[1] - ain)<2)) 
  {
    oain[0] = oain[1];
    oain[1] = ain;
    return;
  }
  oain[0] = oain[1];
  oain[1] = ain;
  
  if (ain > 950 || (Millis - oMillis)<200)
    return;
  oMillis = Millis;

  //Serial.println(ain);
  
  int sw;
  for (sw = 0; sw < 10; sw ++) 
  {
    const int sw_lv[] = {889, 800, 700, 611, 514, 419, 338, 231, 132, 70};
    if (ain > sw_lv[sw])
      break;
  }
  //Serial.println(sw);
  
  switch (menu) 
  {
  case 1:
    menu1_sw(sw); 
    break;
  case 2:
    menu2_sw(sw); 
    break;
  case 0:
    menu0_sw(sw); 
    break; 
  default:
    menu0_sw(sw); 
    
  }
  
  DrawText();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void menu0_sw(int sw) 
{  
  switch (sw) 
  {
   case 0:
    // START/HOLD
    if (Start)
       Start = 0;
     else
       Start = 1;
    break;
   case 1:
    // CH0 RANGE -
    if (range0 < RANGE_MAX)
      range0 ++;
    break;
   case 2:
    // CH1 RANGE -
    if (range1 < RANGE_MAX)
      range1 ++;
    break;
   case 3:
    // RATE FAST
    if (rate > 0)
      rate --;
    break;
   case 4:
    // TRIG MODE
    if (trig_mode < TRIG_ONE)
      trig_mode ++;
    else
      trig_mode = 0;
    break;
   case 5:
    // SEND
    SendData();
    break;
   case 6:
    // TRIG MODE
    if (trig_mode > 0)
      trig_mode --;
    else
      trig_mode = TRIG_ONE;
    break;
   case 7:
    // RATE SLOW
    if (rate < RATE_MAX)
      rate ++;
    break;
   case 8:
    // CH1 RANGE +
    if (range1 > 0)
      range1 --;
    break;
   case 9:
    // CH0 RANGE +
    if (range0 > 0)
      range0 --;
    break;
   case 10:
   default:
    // MENU SW
    menu ++;
     break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void menu1_sw(int sw) 
{  
  switch (sw) 
  {
   case 0:
    // START/HOLD
    if (Start)
       Start = 0;
     else
       Start = 1;
    break;
   case 1:
    // CH0 offset +
    if (ch0_off < 1023)
      ch0_off += 1024/VREF[range0];
    break;
   case 2:
    // CH1 offset +
    if (ch1_off < 1023)
      ch1_off += 1024/VREF[range1];
    break;
   case 3:
    // trigger level +
    if (trig_lv < 60)
      trig_lv ++;
    break;
   case 4:
   case 6:
    // TRIG EDGE
    if (trig_edge == TRIG_E_UP)
      trig_edge = TRIG_E_DN;
    else
      trig_edge = TRIG_E_UP;
    break;
   case 5:
    // SEND
    SendData();
    break;
   case 7:
    // trigger level -
    if (trig_lv > 0)
      trig_lv --;
    break;
   case 8:
    // CH1 OFF -
    if (ch1_off > -1023)
      ch1_off -= 1024/VREF[range1];
    break;
   case 9:
    // CH0 OFF -
    if (ch0_off > -1023)
      ch0_off -= 1024/VREF[range0];
    break;
   case 10:
   default:
    // MENU SW
    menu ++;
     break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void menu2_sw(int sw) 
{  
  switch (sw) 
  {
   case 0:
    // START/HOLD
    if (Start)
       Start = 0;
     else
       Start = 1;
    break;
   case 1:
    if (ch0_mode < 2)
      ch0_mode ++;
    break;
   case 2:
    if (ch1_mode < 2)
      ch1_mode ++;
    break;
   case 3:
   case 7:
    // TRIG channel
    if (trig_ch == 0)
      trig_ch = 1;
    else
      trig_ch = 0;
    break;
   case 5:
    // SEND
    SendData();
    break;
   case 8:
    if (ch1_mode > 0)
      ch1_mode --;
    break;
   case 9:
    if (ch0_mode > 0)
      ch0_mode --;
    break;
   case 10:
    // MENU SW
    menu = 0;
     break;
   case 4:
   case 6:
   default:
    // none
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SendData() 
{
  Serial.print(Rates[rate]);
  Serial.println("/div (30 samples)");
  for (int i=0; i<SAMPLES; i ++) 
  {
      Serial.print(data[sample + 0][i]*MILLIVOL_per_dot[range0]);
      Serial.print(" ");
      Serial.println(data[sample + 1][i]*MILLIVOL_per_dot[range1]);
   } 
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void DrawGrid() 
{
    for (int x=0; x<=SAMPLES; x += 2) 
    { // Horizontal Line
      for (int y=0; y<=LCD_HEIGHT; y += DOTS_DIV) 
      {
        tft.setColor(GRIDCOLOR);
        tft.drawPixel(x, y);
        CheckSW();
      }
      if (LCD_HEIGHT == 240)
      { 
        tft.setColor(GRIDCOLOR);
        tft.drawPixel(x, LCD_HEIGHT-1);
      }
    }
    for (int x=0; x<=SAMPLES; x += DOTS_DIV ) 
    { // Vertical Line
      for (int y=0; y<=LCD_HEIGHT; y += 2) 
      {
        tft.setColor(GRIDCOLOR);
        tft.drawPixel(x, y);
        CheckSW();
      }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

void DrawText() 
{
  tft.setColor(WHITE);
  tft.setTextSize(1);
  tft.print("MODE",SAMPLES+13, 5);
  
  tft.setColor(CYAN);    
  tft.print(Ranges[range0],SAMPLES+3, 25);
  tft.print("/DIV",SAMPLES+20, 25);
  
  tft.print(Rates[rate],SAMPLES+3, 45);
  tft.print("/DIV",SAMPLES+20, 45);
  
  tft.setColor(MAGENTA);
  tft.print(TRIG_Modes[trig_mode],SAMPLES+3, 65);
  
  tft.setColor(BLUE);  
  tft.print(trig_edge == TRIG_E_UP ? "UP" : "DN",SAMPLES+3, 85); 
  
  tft.setColor(RED);
  tft.print(Modes[ch1_mode],SAMPLES+3, 105);
  
  tft.setColor(YELLOW); 
  tft.print(trig_ch == 0 ? "T:1" : "T:2",SAMPLES+3, 125);

  tft.setColor(WHITE);
  tft.setTextSize(1);
  
  tft.print("6.0",5,20); 
 
  tft.print("5.0",5,50); 
  
  tft.print("4.0",5,80); 
  
  tft.print("3.0",5,110); 
  
  tft.print("2.0",5,140); 
 
  tft.print("1.0",5,170); 
  
  tft.print("0",5,200); 
 
  tft.print("-1",5,230); 
 
  #if 0
   tft.setColor(BLACK);
   //tft.fillRect(101,txtLINE0,28,64);  // clear text area that will be drawn below 
    
    switch (menu) 
    {
      case 0:
        tft.print(Ranges[range0],SAMPLES + 1,txtLINE0);
        tft.print(Ranges[range1],SAMPLES + 1,txtLINE1); 
        tft.print(Rates[rate],SAMPLES + 1,txtLINE2); 
        tft.print(TRIG_Modes[trig_mode],SAMPLES + 1,txtLINE3); 
        break;
      case 1:
        tft.print("OF1",SAMPLES + 1,txtLINE0); 
        tft.print("OF2",SAMPLES + 1,txtLINE1);
        tft.print("Tlv",SAMPLES + 1,txtLINE2); 
        tft.print(trig_edge == TRIG_E_UP ? "UP" : "DN",SAMPLES + 1,txtLINE3); 
        break;
      case 2:
        tft.print(Modes[ch0_mode],SAMPLES + 1,txtLINE0); 
        tft.print(Modes[ch1_mode],SAMPLES + 1,txtLINE1);
        tft.print(trig_ch == 0 ? "T:1" : "T:2",SAMPLES + 1,txtLINE2); 
        break;
    }
  #endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void DrawGrid(int x) 
{
    if ((x % 2) == 0)
      for (int y=0; y<=LCD_HEIGHT; y += DOTS_DIV)
      { tft.setColor(GRIDCOLOR);
        tft.drawPixel(x, y);
      }
    if ((x % DOTS_DIV) == 0)
      for (int y=0; y<=LCD_HEIGHT; y += 2)
      { tft.setColor(GRIDCOLOR);
        tft.drawPixel(x, y);
      }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ClearAndDrawGraph() 
{
  int clear = 0;
  
  if (sample == 0)
    clear = 2;
   //#if 0
   for (int x=0; x<(SAMPLES-1); x++) 
   {
     tft.setColor(BGCOLOR);
     tft.drawLine(x, LCD_HEIGHT-data[clear+0][x], x+1, LCD_HEIGHT-data[clear+0][x+1]);
     tft.drawLine(x, LCD_HEIGHT-data[clear+1][x], x+1, LCD_HEIGHT-data[clear+1][x+1]);
     if (ch0_mode != MODE_OFF)
     { tft.setColor(CH1COLOR);
       tft.drawLine(x, LCD_HEIGHT-data[sample+0][x], x+1, LCD_HEIGHT-data[sample+0][x+1]);
     }
     if (ch1_mode != MODE_OFF)
     { tft.setColor(CH1COLOR);
       tft.drawLine(x, LCD_HEIGHT-data[sample+1][x], x+1, LCD_HEIGHT-data[sample+1][x+1]);
     }
     CheckSW();
   }  
  // #endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ClearAndDrawDot(int i) 
{
  int clear = 0;

  if (i <= 1)
    return;
  if (sample == 0)
    clear = 2;
  //#if 0 
  for (int x=0; x<(SAMPLES-1); x++) 
  {
  tft.setColor(BGCOLOR);
  tft.drawLine(i-1, LCD_HEIGHT-data[clear+0][i-1], i, LCD_HEIGHT-data[clear+0][i]);
  tft.drawLine(i-1, LCD_HEIGHT-data[clear+1][i-1], i, LCD_HEIGHT-data[clear+1][i]);
  if (ch0_mode != MODE_OFF)
  { 
    tft.setColor(CH1COLOR);
    tft.drawLine(i-1, LCD_HEIGHT-data[sample+0][i-1], i, LCD_HEIGHT-data[sample+0][i]);
  }
  if (ch1_mode != MODE_OFF)
  {
    tft.setColor(CH1COLOR);
    tft.drawLine(i-1, LCD_HEIGHT-data[sample+1][i-1], i, LCD_HEIGHT-data[sample+1][i]);
  }
  }
  //#endif
  DrawGrid(i);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void DrawGraph()
{
   for (int x=0; x<SAMPLES; x++) 
   {
     tft.setColor(CH1COLOR);
     tft.drawPixel(x, LCD_HEIGHT-data[sample+0][x]);
     tft.drawPixel(x, LCD_HEIGHT-data[sample+1][x]);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ClearGraph() 
{
  int clear = 0;
  
  if (sample == 0)
    clear = 2;
  for (int x=0; x<SAMPLES; x++) 
  {
     tft.setColor(BGCOLOR);
     tft.drawPixel(x, LCD_HEIGHT-data[clear+0][x]);
     tft.drawPixel(x, LCD_HEIGHT-data[clear+1][x]);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

inline unsigned long adRead(byte ch, byte mode, int off)
{
  unsigned long a = analogRead(ch);
  a = ((a+off)*VREF[ch == ad_ch0 ? range0 : range1]+512) >> 10;
  a = a>=(LCD_HEIGHT+1) ? LCD_HEIGHT : a;
  if (mode == MODE_INV)
    return LCD_HEIGHT - a;
  return a;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void  loop() 
{
  
  if (trig_mode != TRIG_SCAN) 
  {
      unsigned long st = millis();
      byte oad;
      if (trig_ch == 0)
        oad = adRead(ad_ch0, ch0_mode, ch0_off);
      else
        oad = adRead(ad_ch1, ch1_mode, ch1_off);
      for (;;) 
      {
        byte ad;
        if (trig_ch == 0)
          ad = adRead(ad_ch0, ch0_mode, ch0_off);
        else
          ad = adRead(ad_ch1, ch1_mode, ch1_off);

        if (trig_edge == TRIG_E_UP) 
        {
           if (ad >= trig_lv && ad > oad)
            break; 
        } 
        else 
        {
           if (ad <= trig_lv && ad < oad)
            break; 
        }
        oad = ad;

        CheckSW();
        if (trig_mode == TRIG_SCAN)
          break;
        if (trig_mode == TRIG_AUTO && (millis() - st) > 100)
          break; 
      }
  }
  
  // sample and draw depending on the sampling rate
    if (rate <= 5 && Start) 
    {
    // change the index for the double buffer
    if (sample == 0)
      sample = 2;
    else
      sample = 0;

    if (rate == 0) 
    { // full speed, channel 0 only
      unsigned long st = millis();
      for (int i=0; i<SAMPLES; i ++) 
      {
        data[sample+0][i] = adRead(ad_ch0, ch0_mode, ch0_off);
      }
      for (int i=0; i<SAMPLES; i ++)
        data[sample+1][i] = 0;
      // Serial.println(millis()-st);
    } 
    else if (rate == 1) 
    { // full speed, channel 1 only
      unsigned long st = millis();
      for (int i=0; i<SAMPLES; i ++) 
      {
        data[sample+1][i] = adRead(ad_ch1, ch1_mode, ch1_off);
      }
      for (int i=0; i<SAMPLES; i ++)
        data[sample+0][i] = 0;
      // Serial.println(millis()-st);
    } else if (rate == 2) 
    { // full speed, dual channel
      unsigned long st = millis();
      for (int i=0; i<SAMPLES; i ++) 
      {
        data[sample+0][i] = adRead(ad_ch0, ch0_mode, ch0_off);
        data[sample+1][i] = adRead(ad_ch1, ch1_mode, ch1_off);
      }
      // Serial.println(millis()-st);
    } 
    else if (rate >= 3 && rate <= 5) 
    { // .5ms, 1ms or 2ms sampling
      const unsigned long r_[] = {5000/DOTS_DIV, 10000/DOTS_DIV, 20000/DOTS_DIV};
      unsigned long st0 = millis();
      unsigned long st = micros();
      unsigned long r = r_[rate - 3];
      for (int i=0; i<SAMPLES; i ++) 
      {
        while((st - micros())<r) ;
        st += r;
        data[sample+0][i] = adRead(ad_ch0, ch0_mode, ch0_off);
        data[sample+1][i] = adRead(ad_ch1, ch1_mode, ch1_off);
      }
      // Serial.println(millis()-st0);
    }
    ClearAndDrawGraph();
    CheckSW();
    DrawGrid();
    DrawText();
    //SendData(); //kirim data ke serial
  
  
  } 
  else if (Start) 
  { // 5ms - 500ms sampling
  // copy currently showing data to another
    if (sample == 0) 
    {
      for (int i=0; i<SAMPLES; i ++) 
      {
        data[2][i] = data[0][i];
        data[3][i] = data[1][i];
      }
    } 
    else 
    {
      for (int i=0; i<SAMPLES; i ++) 
      {
        data[0][i] = data[2][i];
        data[1][i] = data[3][i];
      }      
    }

    const unsigned long r_[] = {50000/DOTS_DIV, 100000/DOTS_DIV, 200000/DOTS_DIV,
                      500000/DOTS_DIV, 1000000/DOTS_DIV, 2000000/DOTS_DIV, 
                      5000000/DOTS_DIV, 10000000/DOTS_DIV};
    unsigned long st0 = millis();
    unsigned long st = micros();
    for (int i=0; i<SAMPLES; i ++) 
    {
      while((st - micros())<r_[rate-6]) 
      {
        CheckSW();
        if (rate<6)
          break;
      }
      if (rate<6) 
      { // sampling rate has been changed
        tft.fillScreen(BGCOLOR);
        break;
      }
      st += r_[rate-6];
      if (st - micros()>r_[rate-6])
          st = micros(); // sampling rate has been changed to shorter interval
      if (!Start) {
         i --;
         continue;
      }
      data[sample+0][i] = adRead(ad_ch0, ch0_mode, ch0_off);
      data[sample+1][i] = adRead(ad_ch1, ch1_mode, ch1_off);
      ClearAndDrawDot(i);     
    }
    // Serial.println(millis()-st0);
    DrawGrid();
    DrawText();
    //SendData(); //kirim data ke serial
  } 
  else 
  {
    CheckSW();
  }
  if (trig_mode == TRIG_ONE)
    Start = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

