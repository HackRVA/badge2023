//
// Created by Samuel Jones on 12/21/21.
//

#include "display.h"
#include "pinout_rp2040.h"
#include "delay.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

/* controller commands- S6B33B2 */
#define NOP                  0x00
#define OSCILLATION_MODE     0x02
#define DRIVER_OUTPUT_MODE   0x10
#define DCDC_SELECT          0x20
#define BIAS_SET             0x22
#define DCDC_CLOCK_DIV       0x24
#define DCDC_AMP_ONOFF       0x26
#define TEMP_COMPENSATION    0x28
#define CONTRAST_CONTROL1    0x2A
#define CONTRAST_CONTROL2    0x2B
#define STANDBY_OFF          0x2C
#define STANDBY_ON           0x2D
#define DDRAM_BURST_OFF      0x2E
#define DDRAM_BURST_ON       0x2F
#define ADDRESSING_MODE      0x30
#define ROW_VECTOR_MODE      0x32
#define N_LINE_INVERSION     0x34
#define FRAME_FREQ_CONTROL   0x36
#define RED_PALETTE          0x38
#define GREEN_PALETTE        0x3A
#define BLUE_PALETTE         0x3C
#define ENTRY_MODE           0x40
#define X_ADDR_AREA          0x42
#define Y_ADDR_AREA          0x43
#define RAM_SKIP_AREA        0x45
#define DISPLAY_OFF          0x50
#define DISPLAY_ON           0x51
#define SPEC_DISPLAY_PATTERN 0x53
#define PARTIAL_DISPLAY_MODE 0x55
#define PARTIAL_START_LINE   0x56
#define PARTIAL_END_LINE     0x57
#define AREA_SCROLL_MODE     0x59
#define SCROLL_START_LINE    0x5A
#define DATA_FORMAT_4K_XRGB  0x60
#define DATA_FORMAT_4K_RGBX  0x61
#define OTP_MODEOFF          0xEA

// The high/low on the A0 pin tells the LCD if we are sending a data or command
#define LCD_COMMAND 0
#define LCD_DATA    1



static unsigned const char G_bias  = 0b00000000; /* 0x00 = 1/4  0x11 = 1/5 0x22 = 1/6 0x33 = 1/7 */

// PEB 20150529 unsigned char G_entry = 0b10000000; 0b00000000 inc Y when X=Xend VS 0b00000010 inc X when Y=Yend
static unsigned char G_entry = 0b10000000; /* 0x80 */

// PEB WAS 20150313 unsigned char G_outputMode = 0b00000010; /* 0x02 lines=132 SDIR=0 SWP=1 CDIR=0 */
// unsigned char G_outputMode = 0b00000110; /* 0x02 lines=132 SDIR=0 SWP=1 CDIR=0 */
static unsigned char G_outputMode = DISPLAY_MODE_NORMAL; /* 0x02 lines=132 SDIR=0 SWP=1 CDIR=0 */

/* 0x11 = fose/32 & fose/16 -> set clock fpck=fose/32(Normal)/fpck=fose/16(partial1)-------*/
static unsigned const char G_clockDiv = 0b00010001; /* default = fose/32 & fose/64  normal and partial modes each */

static unsigned const char G_DCDCselect = 0b00000000; /* step up multplier 1, 1.5 and 2 */

static unsigned const char G_displayPattern = 0b00000000; /* 0 = normal, 1 = inverted, 2&3 read datasheet */

static unsigned const char G_addressMode = 0b00011101; /* 0b0001 1101 65536 colors default GSM=00 DSG=1 SGF=1 SGP=10 SGM=1  */

static unsigned const char G_rowVector = 0b00001110; /* default row vector type=Diagonal INC=111 V=0-----*/

static unsigned const char G_contrast1 = 0b00110100; /* 52 = 0x34 48 = hex 0x30 */

static unsigned const char G_contrast2 = 0b00110100; /* 52 = 0x34 48 = hex 0x30 */

static int dma_channel = -1;
static bool dma_transfer_started = true;

static void wait_until_ready() {
    if (dma_transfer_started) {
        dma_channel_wait_for_finish_blocking(dma_channel);
        // Seems like the display requires some time between DMA transfers finishing and being properly ready. Hard to
        // tell precisely when we need this and when we don't, partial display writes in particular get
        // messed with
        dma_transfer_started = false;
        sleep_us(10);
    }
    while (spi_is_busy(spi0));
}

void display_send_command(unsigned char data) {
    wait_until_ready();
    gpio_put(BADGE_GPIO_DISPLAY_DC, LCD_COMMAND);
    spi_set_format(spi0, 8, 0, 0, SPI_MSB_FIRST);
    spi_write_blocking(spi0, &data, 1);
}

void display_send_data(unsigned short data) {
    wait_until_ready();
    gpio_put(BADGE_GPIO_DISPLAY_DC, LCD_DATA);
    spi_set_format(spi0, 16, 0, 0, SPI_MSB_FIRST);
    spi_write16_blocking(spi0, &data, 1);
}

void display_send_data_multi(const unsigned short *data, int len) {
    wait_until_ready();
    gpio_put(BADGE_GPIO_DISPLAY_DC, LCD_DATA);
    spi_set_format(spi0, 16, 0, 0, SPI_MSB_FIRST);

    dma_channel_transfer_from_buffer_now(dma_channel, data, len);
    dma_transfer_started = true;
}

void display_init_gpio(void) {

    gpio_init(BADGE_GPIO_DISPLAY_CS);
    gpio_set_function(BADGE_GPIO_DISPLAY_CS, GPIO_FUNC_SPI);

    gpio_init(BADGE_GPIO_DISPLAY_SCK);
    gpio_set_function(BADGE_GPIO_DISPLAY_SCK, GPIO_FUNC_SPI);

    gpio_init(BADGE_GPIO_DISPLAY_MOSI);
    gpio_set_function(BADGE_GPIO_DISPLAY_MOSI, GPIO_FUNC_SPI);

    gpio_init(BADGE_GPIO_DISPLAY_DC);
    gpio_set_dir(BADGE_GPIO_DISPLAY_DC, true);

    gpio_init(BADGE_GPIO_DISPLAY_RESET);
    gpio_set_dir(BADGE_GPIO_DISPLAY_RESET, true);

    spi_init(spi0, 20000000);

    if (dma_channel == -1) {
        dma_channel = dma_claim_unused_channel(true);
    }

    // DMA channel: use this for bulk data transfers, which are 16-bit
    dma_channel_config config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config, spi_get_dreq(spi0, true));
    dma_channel_configure(dma_channel,  &config, &spi_get_hw(spi0)->dr, NULL, 0, false);
}

void display_init_device(void)
{
    wait_until_ready();
    display_send_command(STANDBY_ON);  /* standby on == display clocks off */
    display_send_command(DCDC_AMP_ONOFF);
    display_send_command(0x00);        /* booster off */

    // sam, port: this had a 10000 cycle-spinloop
    sleep_ms(2);

    display_send_command(STANDBY_OFF);  // standby on == display clocks off

    display_send_command(OSCILLATION_MODE); // internal osc. 0 = external
    display_send_command(0x01);

    display_send_command(DCDC_AMP_ONOFF);
    display_send_command(0x01); /*------booster1 on---------------*/

    display_send_command(DCDC_AMP_ONOFF);
    display_send_command(0x09); /*------booster1 on and amp on---------*/

    display_send_command(DCDC_AMP_ONOFF);
    display_send_command(0x0b); /*------booster2 on-------------*/

    display_send_command(DCDC_AMP_ONOFF);
    display_send_command(0x0f); /*------booster3 on-------------*/

    //display_send_command(TEMP_COMPENSATION);
    //display_send_command(0x01); /*------temp compsation ratio -0.05%------*/

    /* TREAD LIGHTLY THIS ONE MAY KILL DISPLAYS */
    display_send_command(DCDC_SELECT);
    /* for voltage range input = 3.3v */
    display_send_command(G_DCDCselect);  /* 0b0000 0000  normal & partial mode DC step up = * 1.0 */
    //display_send_command(0x5); /* 0b0000 0101  normal & partial mode DC step up = * 1.5 */
    /* for < 2.8v */
    //display_send_command(0xA);  /* 0b0000 1010  normal & partial mode DC step up = * 2.0 */

    /* this may save battery if run slower = 0x11 */
    /* fyi, the PDF pg 33 doc image swapped  */
    display_send_command(DCDC_CLOCK_DIV);
    display_send_command(G_clockDiv);  /* default = fose/32 & fose/64  normal and partial modes each */
    // display_send_command(0x011); /* 0x11 = fose/32 & fose/16 -> set clock fpck=fose/32(Normal)/fpck=fose/16(partial1)-------*/

    display_send_command(DRIVER_OUTPUT_MODE);
    /* bits- 0 0 DLN1 DLN0 0 SDIR SWP CDIR*/
    /* SWP = RGB vs BGR */
    /* SDIR = segment scanning dir (X) */
    /* CDIR = common scanning dir (Y) */
    /* DLN0,1 = 00=132 01=144, 10=162, 11=96  LCD panel physical y res */
    /* init -> SDIR = 0, SWP = 0, CDR = 0 */
    // PEB 20150313 WAS ->    display_send_command(G_outputMode); /* lines=132 SDIR=0 SWP=0 CDIR=0 */
    display_send_command(G_outputMode); /* lines=132 SDIR=1 SWP=1 CDIR=0 */

    display_send_command(ENTRY_MODE);
    /* 0x0 ==increment Y when X=Xmax, 0x80 == increment X when Y=Ymax */
    /* I am guessing RMW only works for parallel version of these displays */
    //display_send_command(0x0);
    display_send_command(G_entry);

    /* 0x11 = 1/5 & 1/5 -> 0x[0-3][0-3] bias normal and partial 0=1/4 1=1/5 2=1/6 3=1/7 bias --------*/
    // 0x00 = 1/4  0x11 = 1/5 0x22 = 1/6 0x33 = 1/7
    // 1/5 = 4V max , 1/6 = 3.3V max  1/4 = 3V max
    display_send_command(BIAS_SET);
    display_send_command(G_bias);

    display_send_command(SPEC_DISPLAY_PATTERN);
    display_send_command(G_displayPattern); /* 0 = normal, 1 = inverted, 2&3 read datasheet */

    display_send_command(ADDRESSING_MODE);
    display_send_command(G_addressMode); /* 0b0001 1101 65536 colors default GSM=00 DSG=1 SGF=1 SGP=10 SGM=1  */

    display_send_command(ROW_VECTOR_MODE);
    display_send_command(G_rowVector); /* default row vector type=Diagonal ,INC=111-----*/

    /*------x address set from 00 to 127--------*/
    display_send_command(X_ADDR_AREA);
    display_send_command(0x00);
    //display_send_command(0x7F); // 128
    display_send_command(0x83);   // 132 display is really 132x132 overscaned

    /*------y address set from 00 to 127--------*/
    display_send_command(Y_ADDR_AREA);
    display_send_command(0x00);
    //display_send_command(0x7F); // 128
    display_send_command(0x83);   // display is really 132x132


    display_send_command(CONTRAST_CONTROL1);
    display_send_command(G_contrast1);	 /* contrast1 v1 = 3.757v  max=4v */

    /* used for partial "display mode" which we dont use */
    display_send_command(CONTRAST_CONTROL2);
    display_send_command(G_contrast2); /* contrast2 set v1 to 3.757v max=4v */

    //display_send_command(N_LINE_INVERSION);
    ////WriteData(WRCOMM,0x89);//cd
    ////com_out(0x07);
    //display_send_command(0x07);

    display_send_command(PARTIAL_DISPLAY_MODE);
    display_send_command(0x0); /* partial display mode off */

    display_send_command(DISPLAY_ON);
}

// Was LCDReset
void display_reset(void) {
    gpio_init(BADGE_GPIO_DISPLAY_CS);
    gpio_set_dir(BADGE_GPIO_DISPLAY_CS, true);
    gpio_put(BADGE_GPIO_DISPLAY_CS, 0);
    gpio_put(BADGE_GPIO_DISPLAY_RESET, 0);

    sleep_us(1000); // was a 1000 count spinloop

    gpio_put(BADGE_GPIO_DISPLAY_RESET, 1);
    sleep_us(1000);
    gpio_put(BADGE_GPIO_DISPLAY_CS, 1);
    sleep_us(1000);
    gpio_init(BADGE_GPIO_DISPLAY_CS);
    gpio_set_function(BADGE_GPIO_DISPLAY_CS, GPIO_FUNC_SPI);

    display_init_device();
}

/* window of LCD. Send byte will auto-inc x and wrap at xsize and inc y */
void display_rect(int x, int y, int width, int height)
{
    wait_until_ready();
    display_send_command(ENTRY_MODE);
    //display_send_command(0x82); /* auto inc y instead of x */
    // also works    display_send_command(0x80);
    display_send_command(G_entry);

    display_send_command(X_ADDR_AREA);
    display_send_command(x);
    display_send_command(x + width - 1);

    display_send_command(Y_ADDR_AREA);
    display_send_command(y);
    display_send_command(y + height - 1);
}

void display_bias(unsigned char data)
{
    wait_until_ready();
    display_send_command(BIAS_SET);
    display_send_command(data); /* 0x11 = 1/5 & 1/5 -> 0x[0-3][0-3] bias normal and partial 0=1/4 1=1/5 2=1/6 3=1/7 bias --------*/
}

void display_contrast(unsigned char data)
{
    wait_until_ready();
    display_send_command(CONTRAST_CONTROL1);
    display_send_command(data);
    display_send_command(CONTRAST_CONTROL2);
    display_send_command(data); /* contrast2 set v1 to 3.757v max=4v */
}

void display_pixel(unsigned short pixel)
{
    wait_until_ready();
    display_send_data(pixel);
}

void display_pixels(unsigned short *pixel, int number) {
    wait_until_ready();
    display_send_data_multi(pixel, number);
}

void display_set_display_mode_inverted(void)
{
    wait_until_ready();
    G_outputMode = DISPLAY_MODE_INVERTED; /* 0x02 lines=132 SDIR=0 SWP=1 CDIR=0 */
    display_send_command(DRIVER_OUTPUT_MODE);
    display_send_command(DISPLAY_MODE_INVERTED);
}

void display_set_display_mode_noninverted(void)
{
    wait_until_ready();
    G_outputMode = DISPLAY_MODE_NORMAL; /* 0x02 lines=132 SDIR=0 SWP=1 CDIR=0 */
    display_send_command(DRIVER_OUTPUT_MODE);
    display_send_command(DISPLAY_MODE_NORMAL);
}

unsigned char display_get_display_mode(void)
{
    return G_outputMode;
}

int display_get_rotation(void) {
    return (G_outputMode & 0x01);
}

void display_set_rotation(int yes) {

    if (yes) {
        G_outputMode = 0b00000111; /* CDIR=1 */
        G_entry = 0b10000010; /* Y=Yend -> X incremented */
    }
    else {
        /* old way */
        G_outputMode = 0b00000110; /* CDIR=0 */
        G_entry = 0b10000000; /* X=Xend -> Y incremented */
    }

    display_reset();
}

void display_color(unsigned short pixel) {

    unsigned char i,j;

    display_rect(0, 0, 131, 131); /* display is really 132x132 */

    for (i=0; i<132; i++)
        for (j=0; j<132; j++)
            display_pixel(pixel);
}

bool display_busy(void) {
    return spi_is_busy(spi0);
}
