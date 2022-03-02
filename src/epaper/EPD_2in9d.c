/*****************************************************************************
* | File      	:   EPD_2in9d.c
* | Author      :   Waveshare team
* | Function    :   2.9inch e-paper d
* | Info        :
*----------------
* |	This version:   V2.0
* | Date        :   2019-06-12
* | Info        :
* -----------------------------------------------------------------------------
* V3.0(2019-06-12):
* 1.Change:
*    lut_vcomDC[]  => EPD_2IN9D_lut_vcomDC[]
*    lut_ww[] => EPD_2IN9D_lut_ww[]
*    lut_bw[] => EPD_2IN9D_lut_bw[]
*    lut_wb[] => EPD_2IN9D_lut_wb[]
*    lut_bb[] => EPD_2IN9D_lut_bb[]
*    lut_vcom1[] => EPD_2IN9D_lut_vcom1[]
*    lut_ww1[] => EPD_2IN9D_lut_ww1[]
*    lut_bw1[] => EPD_2IN9D_lut_bw1[]
*    lut_wb1[] => EPD_2IN9D_lut_wb1[]
*    lut_bb1[] => EPD_2IN9D_lut_bb1[]
*    EPD_Reset() => EPD_2IN9D_Reset()
*    EPD_SendCommand() => EPD_2IN9D_SendCommand()
*    EPD_SendData() => EPD_2IN9D_SendData()
*    EPD_WaitUntilIdle() => EPD_2IN9D_ReadBusy()
*    EPD_SetFullReg() => EPD_2IN9D_SetFullReg()
*    EPD_SetPartReg() => EPD_2IN9D_SetPartReg()
*    EPD_TurnOnDisplay() => EPD_2IN9D_TurnOnDisplay()
*    EPD_Init() => EPD_2IN9D_Init()
*    EPD_Clear() => EPD_2IN9D_Clear()
*    EPD_Display() => EPD_2IN9D_Display()
*    EPD_Sleep() => EPD_2IN9D_Sleep()
*
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_2in9d.h"
#include "Debug.h"
#include "font5x8.h"
#include "ImageData.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(golioth_epaper, LOG_LEVEL_DBG);

/**
 * partial screen update LUT
**/
const unsigned char EPD_2IN9D_lut_vcom1[] = {
    0x00, 0x19, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    ,0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_ww1[] = {
    0x00, 0x19, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_bw1[] = {
    0x80, 0x19, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_wb1[] = {
    0x40, 0x19, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_bb1[] = {
    0x00, 0x19, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_2IN9D_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(20);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(2);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(20);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(2);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(20);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(2);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(20);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_2IN9D_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_2IN9D_SendData(UBYTE Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_2IN9D_ReadBusy(void)
{
    Debug("e-Paper busy\r\n");
    UBYTE busy;
    do {
        EPD_2IN9D_SendCommand(0x71);
        busy = DEV_Digital_Read(EPD_BUSY_PIN);
        busy =!(busy & 0x01);
		DEV_Delay_ms(20);
    } while(busy);
    DEV_Delay_ms(20);
    Debug("e-Paper busy release\r\n");
}

/******************************************************************************
function :	LUT download
parameter:
******************************************************************************/
static void EPD_2IN9D_SetPartReg(void)
{
    EPD_2IN9D_SendCommand(0x01);	//POWER SETTING
    EPD_2IN9D_SendData(0x03);
    EPD_2IN9D_SendData(0x00);
    EPD_2IN9D_SendData(0x2b);
    EPD_2IN9D_SendData(0x2b);
    EPD_2IN9D_SendData(0x03);

    EPD_2IN9D_SendCommand(0x06);	//boost soft start
    EPD_2IN9D_SendData(0x17);     //A
    EPD_2IN9D_SendData(0x17);     //B
    EPD_2IN9D_SendData(0x17);     //C

    EPD_2IN9D_SendCommand(0x04);
    EPD_2IN9D_ReadBusy();

    EPD_2IN9D_SendCommand(0x00);	//panel setting
    EPD_2IN9D_SendData(0xbf);     //LUT from OTP，128x296

    EPD_2IN9D_SendCommand(0x30);	//PLL setting
    EPD_2IN9D_SendData(0x3a);     // 3a 100HZ   29 150Hz 39 200HZ	31 171HZ

    EPD_2IN9D_SendCommand(0x61);	//resolution setting
    EPD_2IN9D_SendData(EPD_2IN9D_WIDTH);
    EPD_2IN9D_SendData((EPD_2IN9D_HEIGHT >> 8) & 0xff);
    EPD_2IN9D_SendData(EPD_2IN9D_HEIGHT & 0xff);

    EPD_2IN9D_SendCommand(0x82);	//vcom_DC setting
    EPD_2IN9D_SendData(0x12);
	
    EPD_2IN9D_SendCommand(0X50);
    EPD_2IN9D_SendData(0x97);
	
    unsigned int count;
    EPD_2IN9D_SendCommand(0x20);
    for(count=0; count<44; count++) {
        EPD_2IN9D_SendData(EPD_2IN9D_lut_vcom1[count]);
    }

    EPD_2IN9D_SendCommand(0x21);
    for(count=0; count<42; count++) {
        EPD_2IN9D_SendData(EPD_2IN9D_lut_ww1[count]);
    }

    EPD_2IN9D_SendCommand(0x22);
    for(count=0; count<42; count++) {
        EPD_2IN9D_SendData(EPD_2IN9D_lut_bw1[count]);
    }

    EPD_2IN9D_SendCommand(0x23);
    for(count=0; count<42; count++) {
        EPD_2IN9D_SendData(EPD_2IN9D_lut_wb1[count]);
    }

    EPD_2IN9D_SendCommand(0x24);
    for(count=0; count<42; count++) {
        EPD_2IN9D_SendData(EPD_2IN9D_lut_bb1[count]);
    }
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void EPD_2IN9D_TurnOnDisplay(void)
{
    EPD_2IN9D_SendCommand(0x12);		 //DISPLAY REFRESH
    DEV_Delay_ms(10);     //!!!The delay here is necessary, 200uS at least!!!

    EPD_2IN9D_ReadBusy();
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_2IN9D_Init(void)
{
    EPD_2IN9D_Reset();
	
	EPD_2IN9D_SendCommand(0x04);  
	EPD_2IN9D_ReadBusy();//waiting for the electronic paper IC to release the idle signal

	EPD_2IN9D_SendCommand(0x00);			//panel setting
	EPD_2IN9D_SendData(0x1f);		//LUT from OTP，KW-BF   KWR-AF	BWROTP 0f	BWOTP 1f

	EPD_2IN9D_SendCommand(0x61);			//resolution setting
	EPD_2IN9D_SendData (0x80);        	 
	EPD_2IN9D_SendData (0x01);		
	EPD_2IN9D_SendData (0x28);	

	EPD_2IN9D_SendCommand(0X50);			//VCOM AND DATA INTERVAL SETTING			
	EPD_2IN9D_SendData(0x97);		//WBmode:VBDF 17|D7 VBDW 97 VBDB 57		WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7

}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_2IN9D_Clear(void)
{
    UWORD Width, Height;
    Width = (EPD_2IN9D_WIDTH % 8 == 0)? (EPD_2IN9D_WIDTH / 8 ): (EPD_2IN9D_WIDTH / 8 + 1);
    Height = EPD_2IN9D_HEIGHT;

    EPD_2IN9D_SendCommand(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_2IN9D_SendData(0x00);
        }
    }

    EPD_2IN9D_SendCommand(0x13);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_2IN9D_SendData(0xFF);
        }
    }

    EPD_2IN9D_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_2IN9D_Display(UBYTE *Image)
{
    UWORD Width, Height;
    Width = (EPD_2IN9D_WIDTH % 8 == 0)? (EPD_2IN9D_WIDTH / 8 ): (EPD_2IN9D_WIDTH / 8 + 1);
    Height = EPD_2IN9D_HEIGHT;

    EPD_2IN9D_SendCommand(0x10);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_2IN9D_SendData(0x00);
        }
    }
    // Dev_Delay_ms(10);

    EPD_2IN9D_SendCommand(0x13);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_2IN9D_SendData(Image[i + j * Width]);
        }
    }
    // Dev_Delay_ms(10);

    EPD_2IN9D_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_2IN9D_DisplayPart(UBYTE *Image)
{
    /* Set partial Windows */
    EPD_2IN9D_SetPartReg();
    EPD_2IN9D_SendCommand(0x91);		//This command makes the display enter partial mode
    EPD_2IN9D_SendCommand(0x90);		//resolution setting
    EPD_2IN9D_SendData(0);           //x-start
    EPD_2IN9D_SendData(EPD_2IN9D_WIDTH - 1);       //x-end

    EPD_2IN9D_SendData(0);
    EPD_2IN9D_SendData(0);     //y-start
    EPD_2IN9D_SendData(EPD_2IN9D_HEIGHT / 256);
    EPD_2IN9D_SendData(EPD_2IN9D_HEIGHT % 256 - 1);  //y-end
    EPD_2IN9D_SendData(0x28);

    UWORD Width;
    Width = (EPD_2IN9D_WIDTH % 8 == 0)? (EPD_2IN9D_WIDTH / 8 ): (EPD_2IN9D_WIDTH / 8 + 1);
    
    /* send data */
    EPD_2IN9D_SendCommand(0x13);
    for (UWORD j = 0; j < EPD_2IN9D_HEIGHT; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_2IN9D_SendData(Image[i + j * Width]);
        }
    }

    /* Set partial refresh */    
    EPD_2IN9D_TurnOnDisplay();
}

uint8_t flip_invert(uint8_t column) {
    // Flip endianness and invert
    uint8_t ret_column = 0;
    for (uint8_t i=0; i<8; i++)
    {
        if (~column & (1<<i))
        {
            ret_column |= (1<<(7-i));
        }
    }
    return ret_column;
}

/**
 * @brief Use partial refresh to show string on one line of the display
 * 
 * Display is considered protrait-mode 296x128. This leaves 16 lines that are
 * 8-bits tall, and 296 columns
 * 
 * @param str           string to be written to display
 * @param str_len       numer of characters in string
 * @param line          0..16
 */
void epaper_WriteLine(uint8_t *str, uint8_t str_len, uint8_t line)
{
    if (line > 15) return;
    /* Set partial Windows */
    EPD_2IN9D_Init();
    EPD_2IN9D_SetPartReg();
    EPD_2IN9D_SendCommand(0x91);		//This command makes the display enter partial mode
    EPD_2IN9D_SendCommand(0x90);		//resolution setting
    EPD_2IN9D_SendData(line*8);           //x-start
    EPD_2IN9D_SendData((line*8)+8 - 1);       //x-end

    EPD_2IN9D_SendData(0);
    EPD_2IN9D_SendData(0);     //y-start
    EPD_2IN9D_SendData(296 / 256);
    EPD_2IN9D_SendData(296 % 256 - 1);  //y-end
    EPD_2IN9D_SendData(0x28);
    
    /* send data */
    EPD_2IN9D_SendCommand(0x13);

    //FIXME: column centering a looping only works for full-width
    uint8_t send_col;
    uint8_t letter;
    uint8_t column = 0;
    uint8_t str_idx = 48;
    EPD_2IN9D_SendData(0xff); //Unused column
    for (UWORD j = 0; j < 294; j++) {
        for (UWORD i = 0; i < 1; i++) {
            if (column == 0 || str_idx >= str_len)
            {
                send_col = 0xff;
            }
            else
            {
                if (str[str_idx] < 32 || str[str_idx] > 127)
                {
                    //Out of bounds, print a space
                    letter = 0;
                }
                else
                {
                    letter = str[str_idx] - 32;
                }
                send_col = flip_invert(font5x8[(5*letter)+(5-column)]);
            }
            EPD_2IN9D_SendData(send_col);
            if (++column > 5)
            {
                column = 0;
                --str_idx;
            }
        }
    }
    EPD_2IN9D_SendData(0xff); //Unused column

    /* Set partial refresh */    
    EPD_2IN9D_TurnOnDisplay();
    EPD_2IN9D_Sleep();
}

void double_flip_invert(uint8_t orig_column, uint8_t return_cols[2]) {
    // Double the pixesl, the flip endianness and invert
    uint8_t upper_column = 0;
    uint8_t lower_column = 0;
    for (uint8_t i=0; i<4; i++)
    {
        if (orig_column & (1<<(i+4))) upper_column |= 0b11 << (i*2);
        if (orig_column & (1<<(i))) lower_column |= 0b11 << (i*2);
    }
    return_cols[0] = flip_invert(upper_column);
    return_cols[1] = flip_invert(lower_column);
}

/**
 * @brief Use partial refresh to show string on one double-sized line of the
 * display
 *
 * @param str           string to be written to display
 * @param str_len       numer of characters in string
 * @param line          0..16 
 */
void epaper_WriteDoubleLine(uint8_t *str, uint8_t str_len, uint8_t line)
{
    if (line > 7) return;
    EPD_2IN9D_Init();
    /* Set partial Windows */
    EPD_2IN9D_Init();
    EPD_2IN9D_SetPartReg();
    EPD_2IN9D_SendCommand(0x91);		//This command makes the display enter partial mode
    EPD_2IN9D_SendCommand(0x90);		//resolution setting
    EPD_2IN9D_SendData(line*16);           //x-start
    EPD_2IN9D_SendData((line*16)+16 - 1);       //x-end

    EPD_2IN9D_SendData(0);
    EPD_2IN9D_SendData(0);     //y-start
    EPD_2IN9D_SendData(296 / 256);
    EPD_2IN9D_SendData(296 % 256 - 1);  //y-end
    EPD_2IN9D_SendData(0x28);
    
    /* send data */
    EPD_2IN9D_SendCommand(0x13);

    //FIXME: column centering a looping only works for full-width
    uint8_t send_col[2] = {0};
    uint8_t letter;
    uint8_t column = 0;
    uint8_t str_idx = 23;
    for (uint8_t i=0; i<8; i++) EPD_2IN9D_SendData(0xff); //Unused columns
    for (UWORD j = 0; j < 144; j++) {
        for (UWORD i = 0; i < 1; i++) {
            if (column == 0 || str_idx >= str_len)
            {
                send_col[0] = 0xff;
                send_col[1] = 0xff;
            }
            else
            {
                if (str[str_idx] < 32 || str[str_idx] > 127)
                {
                    //Out of bounds, print a space
                    letter = 0;
                }
                else
                {
                    letter = str[str_idx] - 32;
                }
                uint8_t letter_column = font5x8[(5*letter)+(5-column)];
                double_flip_invert(letter_column, send_col);
            }
            EPD_2IN9D_SendData(send_col[1]);
            EPD_2IN9D_SendData(send_col[0]);
            EPD_2IN9D_SendData(send_col[1]);
            EPD_2IN9D_SendData(send_col[0]);
            if (++column > 5)
            {
                column = 0;
                --str_idx;
            }
        }
    }
    for (uint8_t i=0; i<8; i++) EPD_2IN9D_SendData(0xff); //Unused columns

    /* Set partial refresh */    
    EPD_2IN9D_TurnOnDisplay();
    EPD_2IN9D_Sleep();
}

/**
 * @brief Clear the displays
 *
 * This handles waking up the display, performing a full clear, and putting it
 * back to sleep
 *
 */
void epaper_FullClear(void) {
    EPD_2IN9D_Init();
    EPD_2IN9D_Clear();
    EPD_2IN9D_Sleep();
}

void epaper_init(void) {
    LOG_INF("Setup ePaper pins");
  	DEV_Module_Init();

    LOG_INF("ePaper Init and Clear");
    EPD_2IN9D_Init();
    EPD_2IN9D_Clear();
	k_sleep(K_MSEC(500));

	LOG_INF("Show Golioth logo");
	EPD_2IN9D_Display((void *)gImage_2in9); /* cast because function is not expecting a CONST array) */
    EPD_2IN9D_Sleep(); /* always sleep the ePaper display to avoid damaging it */
}


/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_2IN9D_Sleep(void)
{
    EPD_2IN9D_SendCommand(0X50);
    EPD_2IN9D_SendData(0xf7);
    EPD_2IN9D_SendCommand(0X02);  	//power off
    EPD_2IN9D_ReadBusy();
    EPD_2IN9D_SendCommand(0X07);  	//deep sleep
    EPD_2IN9D_SendData(0xA5);
}
