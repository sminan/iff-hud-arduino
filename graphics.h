

/* Library to resume graphics functions */
#include <TFT_ILI93XX.h>
#include <avr/pgmspace.h>
#include <SD.h>

// Color definitions
#define _BLACK   0x0000
#define _BLUE    0x001F
#define _RED     0xF800
#define _GREEN   0x07E0
#define _CYAN    0x07FF
#define _MAGENTA 0xF81F
#define _YELLOW  0xFFE0
#define _WHITE   0xFFFF
#define _KEYCOLOR   0xF81F
#define BUFSZ 8 //more is slower. Tested with 64 without "k" variable and ran slower

void draw_line(TFT_ILI93XX tft_left, TFT_ILI93XX tft_right, int color, uint8_t line)
{
  uint8_t i;

  line = line + 1;
  
  for (i = 0 ; i < 4 ; i++)
  {
    tft_left.drawLine(50+(140*50*line)/250, 50*line+i, 240, 50*line+i, color);
    tft_right.drawLine(190-(140*50*line)/250, 50*line+i, 0, 50*line+i, color);
  }
}

void drawBMP_flip(TFT_ILI93XX tft, char* str, int x, int y, int w, int h)
{
  uint16_t buf[BUFSZ];
  uint8_t i;
  uint8_t j;
  uint8_t k;
  
  File file;
  file = SD.open(str,FILE_READ);
  
  if((x >= tft.width()) || (y >= tft.height())) return;
  
  for ( i = 0 ; i < h ; i++)
  { 
    for ( j = 0 ; j < w ; j = j+8)
    {
      file.read(buf,sizeof(buf));
      for ( k = 0 ; k < 8 ; k++)
      {
        if ( buf[k] != _KEYCOLOR )
        {
          tft.drawPixel( x + j + k, y + i, buf[k]);  
        }
      }
    }
  }
  file.close();  
}

void draw_fade(TFT_ILI93XX tft_left, TFT_ILI93XX tft_right, uint16_t color, uint8_t y0, uint8_t y1)
{
  uint8_t i;
  uint8_t j;
  uint8_t dif;

  dif = y0 - y1;

  if (y1 > 0 ) 
  {
    y1 = y1 - 1;
    for ( i = 0 ; i < 10 ; i++ )
    {
      tft_left.drawLine(50+(140*(y1 - i ))/250 + 1, y1-i, 240, y1-i, color);
      tft_right.drawLine(190-(140*(y1 - i))/250 - 1, y1-i, 0, y1-i, color);
      if (color) color = color - 64;
    }
  }
  
  if ( y0 >= 0 )
  {
    for ( i = 0 ; i < dif + 1  ; i++ )
    {
      tft_left.drawLine(  50 +(140*(y0-i))/250 + 1, y0-i, 240, y0-i, _BLACK);
      tft_right.drawLine( 190-(140*(y0-i))/250 - 1, y0-i, 0  , y0-i, _BLACK);
    }
  }  
  draw_line(tft_left, tft_right, _WHITE, 3);
  draw_line(tft_left, tft_right, _WHITE, 2);
  draw_line(tft_left, tft_right, _WHITE, 1);
  draw_line(tft_left, tft_right, _WHITE, 0);
}


int drawBMP_flip_cont(TFT_ILI93XX tft, char* str, int x, int y, int w, int h, unsigned long* context)
{
  uint16_t buf[BUFSZ];
  uint8_t i;
  uint8_t j;
  uint8_t k;
  uint8_t l;
  
  File file;
  file = SD.open(str,FILE_READ);
  file.seek(context[1]);
  
  if((x >= tft.width()) || (y >= tft.height())) return;
  
  for ( i = context[0], l = 0 ; i < h && l < 16 ; i++, l++)  //The maximal value for "l" define in how many steps the sign will be shown (for 16 -> 4)
  { 
    for ( j = 0 ; j < w ; j = j+8)
    {
      file.read(buf,sizeof(buf));
      for ( k = 0 ; k < 8 ; k++)
      {
        if ( buf[k] != _KEYCOLOR )
        {
          tft.drawPixel( x + j + k, y + i, buf[k]);  
        }
      }
    }
  }
  if ( i == h )
  {
    context[0] = 0;
    context[1] = 0;
    return 0;
  }
  else
  {
    context[0] = i;
    context[1] = file.position();
    file.close();
    return y;
  }

  
}
