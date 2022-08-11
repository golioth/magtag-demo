/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* originally based on Wavesahre E-Paper Library: */

/*
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

#include "magtag_epaper.h"
#include "magtag_linewriter.h"
//#include "Debug.h"
#include "DEV_Config.h"
#include "font5x8.h"
#include "ImageData.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(golioth_epaper, LOG_LEVEL_DBG);

bool _display_asleep = true;

/**
 * partial screen update LUT
**/
const unsigned char EPD_2IN9D_lut_vcom1[] = {
    0x00, 0x00, 0x00, 0x25, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_ww1[] = {
    0x02, 0x00, 0x00, 0x25, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_bw1[] = {
    0x48, 0x00, 0x00, 0x25, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_wb1[] = {
    0x84, 0x00, 0x00, 0x25, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_bb1[] = {
    0x01, 0x00, 0x00, 0x25, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

bool EPD_2IN9D_IsAsleep(void) {
    return _display_asleep;
}

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
void EPD_2IN9D_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(10);
    _display_asleep = false;
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
void EPD_2IN9D_SendCommand(uint8_t Reg)
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
void EPD_2IN9D_SendData(uint8_t Data)
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
    uint8_t busy;
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
void EPD_2IN9D_SetPartReg(void)
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
    EPD_2IN9D_SendData(0x3C);     // 3a 100HZ   29 150Hz 39 200HZ	31 171HZ

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
void EPD_2IN9D_Refresh(void)
{
    EPD_2IN9D_SendCommand(0x12);		 //DISPLAY REFRESH
    DEV_Delay_ms(1);     //!!!The delay here is necessary, 200uS at least!!!

    EPD_2IN9D_ReadBusy();
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_2IN9D_Init(void)
{
    if (_display_asleep) { EPD_2IN9D_Reset(); }
    EPD_2IN9D_SendCommand(0x00);			//panel setting
    EPD_2IN9D_SendData(0x1f);		//LUT from OTP，KW-BF   KWR-AF	BWROTP 0f	BWOTP 1f

//     EPD_2IN9D_SendCommand(0x61);			//resolution setting
//     EPD_2IN9D_SendData (0x80);        	 
//     EPD_2IN9D_SendData (0x01);		
//     EPD_2IN9D_SendData (0x28);	

    EPD_2IN9D_SendCommand(0X50);			//VCOM AND DATA INTERVAL SETTING			
    EPD_2IN9D_SendData(0x97);		//WBmode:VBDF 17|D7 VBDW 97 VBDB 57		WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7
    
    EPD_2IN9D_SendCommand(0x04);
    EPD_2IN9D_ReadBusy();
}


void EPD_2IN9D_SendRepeatedBytePattern(uint8_t byte_pattern, uint16_t how_many) {
    for (uint16_t i = 0; i < how_many; i++) {
        EPD_2IN9D_SendData(byte_pattern);
    }
}

void EPD_2IN9D_SendPartialAddr(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    EPD_2IN9D_SendCommand(0x90);		//resolution setting
    EPD_2IN9D_SendData(x);           //x-start
    EPD_2IN9D_SendData(x+w - 1);       //x-end

    EPD_2IN9D_SendData(0);
    EPD_2IN9D_SendData(y);     //y-start
    EPD_2IN9D_SendData((y+h) / 256);
    EPD_2IN9D_SendData((y+h) % 256 - 1);  //y-end
    EPD_2IN9D_SendData(0x01);
}

void EPD_2IN9D_SendPartialLineAddr(uint8_t line) {
    EPD_2IN9D_SendCommand(0x90);		//resolution setting
    EPD_2IN9D_SendData(line*16);           //x-start
    EPD_2IN9D_SendData((line*16)+16 - 1);       //x-end

    EPD_2IN9D_SendData(0);
    EPD_2IN9D_SendData(0);     //y-start
    EPD_2IN9D_SendData(296 / 256);
    EPD_2IN9D_SendData(296 % 256 - 1);  //y-end
    EPD_2IN9D_SendData(0x01);
}

/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_2IN9D_Clear(void)
{
    uint16_t Width, Height;
    Width = (EPD_2IN9D_WIDTH % 8 == 0)? (EPD_2IN9D_WIDTH / 8 ): (EPD_2IN9D_WIDTH / 8 + 1);
    Height = EPD_2IN9D_HEIGHT;

    EPD_2IN9D_SendCommand(0x13);
    EPD_2IN9D_SendRepeatedBytePattern(0xFF, Width*Height);

    EPD_2IN9D_SendCommand(0x10);
    EPD_2IN9D_SendRepeatedBytePattern(0xFF, Width*Height);

    EPD_2IN9D_Refresh();

    EPD_2IN9D_SendCommand(0x13);
    EPD_2IN9D_SendRepeatedBytePattern(0xFF, Width*Height);
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_2IN9D_Display(uint8_t *Image)
{
    uint16_t Width, Height;
    Width = (EPD_2IN9D_WIDTH % 8 == 0)? (EPD_2IN9D_WIDTH / 8 ): (EPD_2IN9D_WIDTH / 8 + 1);
    Height = EPD_2IN9D_HEIGHT;

    EPD_2IN9D_SendCommand(0x13);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            EPD_2IN9D_SendData(Image[i + j * Width]);
        }
    }
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_2IN9D_DisplayPart(uint8_t *Image)
{
    /* Set partial Windows */
    EPD_2IN9D_SetPartReg();
    EPD_2IN9D_SendCommand(0x91);		//This command makes the display enter partial mode
    EPD_2IN9D_SendPartialAddr(0, 0, EPD_2IN9D_WIDTH, EPD_2IN9D_HEIGHT);
    EPD_2IN9D_Display(Image);
}

void EPD_2in9D_PartialClear(void) {
    EPD_2IN9D_SendCommand(0x91);		//This command makes the display enter partial mode
    EPD_2IN9D_SendCommand(0x90);		//resolution setting
    EPD_2IN9D_SendData(0);           //x-start
    EPD_2IN9D_SendData(EPD_2IN9D_WIDTH - 1);       //x-end

    EPD_2IN9D_SendData(0);
    EPD_2IN9D_SendData(0);     //y-start
    EPD_2IN9D_SendData(EPD_2IN9D_HEIGHT / 256);
    EPD_2IN9D_SendData(EPD_2IN9D_HEIGHT % 256 - 1);  //y-end
    EPD_2IN9D_SendData(0x01);

    uint16_t Width;
    Width = (EPD_2IN9D_WIDTH % 8 == 0)? (EPD_2IN9D_WIDTH / 8 ): (EPD_2IN9D_WIDTH / 8 + 1);
    
    /* send data */
    EPD_2IN9D_SendCommand(0x13);
    for (uint16_t j = 0; j < EPD_2IN9D_HEIGHT; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            EPD_2IN9D_SendData(0xFF);
        }
    }
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

/**
 * @brief Initialize pins used to drive the display, execute the initialization
 * process, and display the Golioth logo
 *
 */
void epaper_init(void) {
    _display_asleep = true;
    LOG_INF("Setup ePaper pins");
  	DEV_Module_Init();

    LOG_INF("ePaper Init and Clear");
    EPD_2IN9D_Init();
    EPD_2IN9D_Clear();

    LOG_INF("Show Golioth logo");
    EPD_2IN9D_Display((void *)gImage_2in9); /* cast because function is not expecting a CONST array) */

    EPD_2IN9D_Refresh();
    EPD_2IN9D_Display((void *)gImage_2in9); /* cast because function is not expecting a CONST array) */
    EPD_2IN9D_SetPartReg();
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
    _display_asleep = true;
}
