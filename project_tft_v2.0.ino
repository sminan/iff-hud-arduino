
/* This version reacts to real variables through CAN-BUS                                    */
/* Implementation with Arduino MEGA, new distance fade function and sign has just 4 stages  */

//Includes
#include <TFT_ILI93XX.h>
#include <SPI.h>
#include <SD.h>
#include <MsTimer2.h>
#include <avr/pgmspace.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include "functions.h"     //my functions
#include "graphics.h"     //my functions

//Definitions
#define SCLK 52
#define MISO 50
#define MOSI 51
#define DC_RIGHT 48
#define DC_LEFT 49
#define RST_RIGHT 46
#define RST_LEFT 47
#define CS_SD 44
#define CS_RIGHT 42
#define CS_LEFT 43
#define CS_CAN 9

#define TSTIME_1 0
#define TSTIME_2 0
#define BUFFPXL 64
#define FILES_SIGN 4
#define FILES_SYM 2
#define BAUDRATE 115200
#define BUFSIZE 2
#define YPOS_FUNC 256

#define SLAVE 0x01

// Color definitions
#define _BLACK   0x0000
#define _BLUE    0x001F
#define _RED     0xF800
#define _GREEN   0x07E0
#define _CYAN    0x07FF
#define _MAGENTA 0xF81F
#define _YELLOW  0xFFE0
#define _WHITE   0xFFFF

struct SIGN
{
  uint8_t type = 0;
  uint8_t x0 = 0;
  uint8_t x1 = 0;
  uint8_t y0 = 0;
  uint8_t y1 = 0;
  uint8_t w = 64;
  uint8_t h = 64;
};

//TFT Object initialization with hardware SPI
TFT_ILI93XX tft_left = TFT_ILI93XX(CS_LEFT, DC_LEFT, RST_LEFT);
TFT_ILI93XX tft_right = TFT_ILI93XX(CS_RIGHT, DC_RIGHT, RST_RIGHT);
MCP_CAN CAN(CS_CAN);                                    // Set CS pin
SIGN sign;

//Global Variables
uint8_t i = 0 ;
uint8_t j = 0 ;
int ts_1 = TSTIME_1;
char aux[8];
uint8_t car_speed = 0;
int aux1 = 0;
uint8_t sector_on = 0;
uint8_t sector_off = 0;
uint8_t can_tx[BUFSIZE];
uint8_t can_rx[BUFSIZE];
uint8_t difx = 0;
uint8_t dify = 0;
int pos = 0;
unsigned long context[2];
uint8_t flag = 0;

//Files

const char dist_0[] PROGMEM = "m050.bin";
const char dist_1[] PROGMEM = "m100.bin";
const char dist_2[] PROGMEM = "m150.bin";
const char dist_3[] PROGMEM = "m200.bin";

const char* const files_dist[] PROGMEM = {dist_0, dist_1, dist_2, dist_3};

const char sign_0[] PROGMEM = "k000.bin";         //No limit
const char sign_1[] PROGMEM = "k005.bin";
const char sign_2[] PROGMEM = "k010.bin";
const char sign_3[] PROGMEM = "k020.bin";
const char sign_4[] PROGMEM = "k030.bin";
const char sign_5[] PROGMEM = "k040.bin";
const char sign_6[] PROGMEM = "k050.bin";
const char sign_7[] PROGMEM = "k060.bin";
const char sign_8[] PROGMEM = "k070.bin";
const char sign_9[] PROGMEM = "k080.bin";
const char sign_10[] PROGMEM = "k090.bin";
const char sign_11[] PROGMEM = "k100.bin";
const char sign_12[] PROGMEM = "k110.bin";
const char sign_13[] PROGMEM = "k120.bin";
const char sign_14[] PROGMEM = "k130.bin";
const char sign_15[] PROGMEM = "k140.bin";
const char sign_16[] PROGMEM = "k150.bin";
const char sign_17[] PROGMEM = "k160.bin";

const char* const files_sign[] PROGMEM = {sign_0, sign_1, sign_2, sign_3, sign_4, \
                                          sign_5, sign_6, sign_7, sign_8, sign_9, \
                                          sign_10, sign_11, sign_12, sign_13,     \
                                          sign_14, sign_15, sign_16, sign_17};

const char acc_0[] PROGMEM = "a000.bin";      //ACC Inactiv
const char acc_1[] PROGMEM = "a001.bin";      //ACC Activ

const char* const files_acc[] PROGMEM = {acc_0, acc_1};

const char nass_0[] PROGMEM = "n000.bin";      //ACC Activ

const char* const files_nass[] PROGMEM = {nass_0};

const char pedal_0[] PROGMEM = "p000.bin";      //Pedal
const char pedal_1[] PROGMEM = "p001.bin";      //Pedal

const char* const files_pedal[] PROGMEM = {pedal_0, pedal_0};
/* -------------------TIMESTICK---------------------*/
void timestick()
{
  if (ts_1 > 0) ts_1--;
  //  if (ts_2 > 0) ts_2--;
}

/* ------------------SETUP CODE---------------------*/
void setup()
{
  Serial.begin(BAUDRATE);
  if (SD.begin(CS_SD)) Serial.println("SD Card OK!");
  else
  {
    Serial.println("SD Card don't work...");
    return;
  }

  //  MsTimer2::set(1, timestick);
  //  MsTimer2::start();

  while (CAN_OK != CAN.begin(CAN_500KBPS))              // init can bus : baudrate = 500k
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(100);
  }
  Serial.println("CAN BUS Shield init ok!");

  tft_left.begin();
  tft_right.begin();
  tft_left.setRotation(0);
  tft_right.setRotation(0);

  //Erase the screen
  tft_left.fillScreen(_BLACK);
  tft_right.fillScreen(_BLACK);

  context[0] = 0;
  context[1] = 0;

  //Draw road line. Four times makes 4px thickness
  for (i = 0 ; i < 4 ; i++)
  {
    tft_left.drawLine(50 - i, 0, 240 - 50 - i, 250, _WHITE);
    tft_right.drawLine(240 - 50 + i, 0, 50 + i , 250, _WHITE);
  }

  for (i = 0 ; i < 4 ; i++)
  {
    strcpy_P(aux, (char*)pgm_read_word(&(files_dist[i])));
    draw_line(tft_left, tft_right, _WHITE, i);
    drawBMP_flip(tft_right, aux, 200 - (140 * 50 * (i + 1)) / 250, 50 * (i + 1), 64, 24);
  }

  //Draw end line. Four times makes 4px thickness
  draw_line(tft_left, tft_right, _WHITE, 4);

  //Wait a second
  delay(1000);

  strcpy_P(aux, (char*)pgm_read_word(&(files_acc[0])));
  drawBMP_flip(tft_right, aux, 176, YPOS_FUNC, 64, 64);          //240-64=176

  can_rx[0] = 0xFF;

  /*  strcpy_P(aux, (char*)pgm_read_word(&(files_pedal[0])));
    drawBMP_flip(tft_right, aux, 48, YPOS_FUNC, 64, 64);           //240-3*64=48

    strcpy_P(aux, (char*)pgm_read_word(&(files_pedal[1])));
    drawBMP_flip(tft_left, aux, 176, YPOS_FUNC, 64, 64);
  */
}

/* -------------------LOOP CODE---------------------*/
void loop()
{

  can_tx[0] = 1;
  can_tx[1] = 0;
  CAN.sendMsgBuf(SLAVE, 0, sizeof(can_tx), can_tx);

  if (CAN_MSGAVAIL == CAN.checkReceive())           // check if data coming
  {
    receive_data(can_rx, CAN);
  }

  sign.type = can_rx[0];

  if (sign.type != 0xFF)
  {
    if (sign.type >= 0 && sign.type <= 0x12)
    {
      sign.x0 = sign.x1;
      sign.y0 = sign.y1;

      aux1 = (140 * can_rx[1]) / 250 - 22;

      if ( aux1 >= 0 ) sign.x1 = aux1;
      else sign.x1 = 0;

      sign.y1 = can_rx[1];

      if (sign.x0 > sign.x1) difx = sign.x0 - sign.x1;
      else if (sign.x0 < sign.x1) difx = 64;
      else if (sign.x0 = sign.x1) difx = 0;

      if (sign.y0 > sign.y1) dify = sign.y0 - sign.y1;
      else if (sign.y0 < sign.y1) dify = 64;
      else if (sign.y0 = sign.y1) dify = 0;

      strcpy_P(aux, (char*)pgm_read_word(&(files_sign[sign.type])));
      draw_fade(tft_left, tft_right, _GREEN, sign.y0, sign.y1);
      if ( sign.y1 <= 200 && sign.y0 == 0 )
      {
        //drawBMP_flip(tft_left, aux, (140*200)/250 - 22, 200, 64, 64);
        pos = 200;
      }
      else if ( sign.y1 <= 150 && sign.y0 > 150 )
      {
        tft_left.fillRect((140 * 200) / 250 - 22, 200, sign.w, sign.h, _BLACK);
        //drawBMP_flip(tft_left, aux, (140*150)/250 - 22, 150, 64, 64);
        pos = 150;
      }
      else if ( sign.y1 <= 100 && sign.y0 > 100 )
      {
        tft_left.fillRect((140 * 150) / 250 - 22, 150, sign.w, sign.h, _BLACK);
        //drawBMP_flip(tft_left, aux, (140*100)/250 - 22, 100, 64, 64);
        pos = 100;
      }
      else if ( sign.y1 <= 50 && sign.y0 > 50 )
      {
        tft_left.fillRect((140 * 100) / 250 - 22, 100, sign.w, sign.h, _BLACK);
        //drawBMP_flip(tft_left, aux, (140*50)/250 - 22, 50, 64, 64);
        pos = 50;
      }
      if (pos > 0) pos = drawBMP_flip_cont(tft_left, aux, (140 * pos) / 250 - 22, pos, 64, 64, context);
    }
    if (sign.type == 0x80)
    {
      strcpy_P(aux, (char*)pgm_read_word(&(files_nass[0])));
      drawBMP_flip(tft_left, aux, 0, YPOS_FUNC, 64, 64);
    }
    if (sign.type == 0x20)
    {
      strcpy_P(aux, (char*)pgm_read_word(&(files_acc[0])));
      drawBMP_flip(tft_right, aux, 176, YPOS_FUNC, 64, 64);          //240-64=176
    }
    else if (sign.type == 0x21)
    {
      strcpy_P(aux, (char*)pgm_read_word(&(files_acc[1])));
      drawBMP_flip(tft_right, aux, 176, YPOS_FUNC, 64, 64);          //240-64=176
    }
  }
  else
  {
    if (sign.y1 <= 50 ) tft_left.fillRect(sign.x1 + (140 * 50) / 250 - 22, 50, sign.w, sign.h, _BLACK);
    else if (sign.y1 <= 100 ) tft_left.fillRect(sign.x1 + (140 * 50) / 250 - 22, 100, sign.w, sign.h, _BLACK);
    else if (sign.y1 <= 150 ) tft_left.fillRect(sign.x1 + (140 * 50) / 250 - 22, 150, sign.w, sign.h, _BLACK);
    else if (sign.y1 <= 200 ) tft_left.fillRect(sign.x1 + (140 * 50) / 250 - 22, 200, sign.w, sign.h, _BLACK);
    //if (sign.y0 > 0)
    draw_fade(tft_left, tft_right, _BLACK, sign.y0, sign.y1);
    sign.x0 = 0;
    sign.x1 = 0;
    sign.y0 = 0;
    sign.y1 = 0;
  }
}


