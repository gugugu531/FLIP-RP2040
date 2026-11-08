/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2021-03-16
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
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
******************************************************************************/
#include "DEV_Config.h"
#include "hardware/dma.h"

uint slice_num;
static int g_lcd_spi_dma_chan = -1;
static dma_channel_config g_lcd_spi_dma_cfg;
static spi_inst_t *g_spi_dma_active_port = NULL;
static bool g_spi_dma_inflight = false;

/**
 * delay x ms
 **/
void DEV_Delay_ms(uint32_t xms)
{
    sleep_ms(xms);
}

void DEV_Delay_us(uint32_t xus)
{
    sleep_us(xus);
}

void DEV_GPIO_Init(void)
{

    DEV_GPIO_Mode(LCD_RST_PIN, 1);
    DEV_GPIO_Mode(LCD_DC_PIN, 1);
    DEV_GPIO_Mode(LCD_CS_PIN, 1);
    DEV_GPIO_Mode(LCD_BL_PIN, 1);

    DEV_Digital_Write(LCD_CS_PIN, 1);
    DEV_Digital_Write(LCD_DC_PIN, 0);
    DEV_Digital_Write(LCD_BL_PIN, 1);
}

/**
 * GPIO read and write
 **/
void DEV_Digital_Write(uint16_t Pin, uint8_t Value)
{
    gpio_put(Pin, Value);
}

uint8_t DEV_Digital_Read(uint16_t Pin)
{
    return gpio_get(Pin);
}

/**
 * SPI
 **/
void DEV_SPI_WriteByte(spi_inst_t *SPI_PORT,uint8_t Value)
{
    spi_write_blocking(SPI_PORT, &Value, 1);
}

void DEV_SPI_Write_nByte(spi_inst_t *SPI_PORT,uint8_t pData[], uint32_t Len)
{
    if (Len == 0) {
        return;
    }

    if (g_lcd_spi_dma_chan >= 0) {
        dma_channel_configure(
            g_lcd_spi_dma_chan,
            &g_lcd_spi_dma_cfg,
            &spi_get_hw(SPI_PORT)->dr,
            pData,
            Len,
            true);
        dma_channel_wait_for_finish_blocking(g_lcd_spi_dma_chan);
        while (spi_is_busy(SPI_PORT)) {
            tight_loop_contents();
        }
        return;
    }

    spi_write_blocking(SPI_PORT, pData, Len);
}

void DEV_SPI_Write_nByte_Async(spi_inst_t *SPI_PORT,uint8_t *pData, uint32_t Len)
{
    if (Len == 0) {
        return;
    }

    if (g_lcd_spi_dma_chan >= 0) {
        if (g_spi_dma_inflight) {
            DEV_SPI_Wait_Async_Complete(g_spi_dma_active_port ? g_spi_dma_active_port : SPI_PORT);
        }

        g_spi_dma_active_port = SPI_PORT;
        g_spi_dma_inflight = true;
        dma_channel_configure(
            g_lcd_spi_dma_chan,
            &g_lcd_spi_dma_cfg,
            &spi_get_hw(SPI_PORT)->dr,
            pData,
            Len,
            true);
        return;
    }

    spi_write_blocking(SPI_PORT, pData, Len);
    while (spi_is_busy(SPI_PORT)) {
        tight_loop_contents();
    }
}

/* Pixels are byte-swapped in RAM, like the existing Waveshare image API.
 * 16-bit SPI frames reduce the inter-frame gap. DMA swaps each halfword back
 * to native RGB565 before SPI shifts the most significant bit first. */
void DEV_SPI_Write_RGB565_Async(spi_inst_t *port, const uint16_t *pixels, uint32_t count)
{
    if (!count) return;
    DEV_SPI_Wait_Async_Complete(port);
    dma_channel_config cfg = g_lcd_spi_dma_cfg;
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_bswap(&cfg, true);
    g_spi_dma_active_port = port;
    g_spi_dma_inflight = true;
    dma_channel_configure(g_lcd_spi_dma_chan, &cfg, &spi_get_hw(port)->dr,
                          pixels, count, true);
}

void DEV_SPI_Wait_Async_Complete(spi_inst_t *SPI_PORT)
{
    if (g_lcd_spi_dma_chan >= 0 && g_spi_dma_inflight) {
        dma_channel_wait_for_finish_blocking(g_lcd_spi_dma_chan);
        while (spi_is_busy(SPI_PORT)) {
            tight_loop_contents();
        }
        g_spi_dma_inflight = false;
        g_spi_dma_active_port = NULL;
    }
}

/**
 * I2C
 **/

void DEV_I2C_Write_Byte(i2c_inst_t *I2C_PORT,uint8_t addr, uint8_t reg, uint8_t Value)
{
    uint8_t data[2] = {reg, Value};
    i2c_write_blocking(I2C_PORT, addr, data, 2, false);
}

void DEV_I2C_Write_nByte(i2c_inst_t *I2C_PORT,uint8_t addr, uint8_t *pData, uint32_t Len)
{
    i2c_write_blocking(I2C_PORT, addr, pData, Len, false);
}

uint8_t DEV_I2C_Read_Byte(i2c_inst_t *I2C_PORT,uint8_t addr, uint8_t reg)
{
    uint8_t buf;
    i2c_write_blocking(I2C_PORT,addr,&reg,1,true);
    i2c_read_blocking(I2C_PORT,addr,&buf,1,false);
    return buf;
}
void DEV_I2C_Read_nByte(i2c_inst_t *I2C_PORT,uint8_t addr,uint8_t reg, uint8_t *pData, uint32_t Len)
{
    i2c_write_blocking(I2C_PORT,addr,&reg,1,true);
    i2c_read_blocking(I2C_PORT,addr,pData,Len,false);
}

/**
 * GPIO Mode
 **/
void DEV_GPIO_Mode(uint16_t Pin, uint16_t Mode)
{
    gpio_init(Pin);
    if (Mode == 0 || Mode == GPIO_IN)
    {
        gpio_set_dir(Pin, GPIO_IN);
    }
    else
    {
        gpio_set_dir(Pin, GPIO_OUT);
    }
}

/**
 * KEY Config
 **/
void DEV_KEY_Config(uint16_t Pin)
{
    gpio_init(Pin);
    gpio_pull_up(Pin);
    gpio_set_dir(Pin, GPIO_IN);
}
/*
** PWM
*/
void DEV_SET_PWM(uint8_t Value)
{
    if (Value < 0 || Value > 100)
    {
        printf("DEV_SET_PWM Error \r\n");
    }
    else
    {
        pwm_set_chan_level(slice_num, PWM_CHAN_B, Value);
    }
}
/**
 * IRQ
 **/
void DEV_IRQ_SET(uint gpio, uint32_t events, gpio_irq_callback_t callback)
{
    gpio_set_irq_enabled_with_callback(gpio,events,true,callback);
}

/******************************************************************************
function:	Module Initialize, the library and initialize the pins, SPI protocol
parameter:
Info:
******************************************************************************/
uint8_t DEV_Module_Init(void)
{
    stdio_init_all();
    // GPIO Config
    DEV_GPIO_Init();
    // ADC
    adc_init();
    adc_gpio_init(BAT_ADC_PIN);
    adc_select_input(BAR_CHANNEL);
    // PWM Config
    gpio_set_function(LCD_BL_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(LCD_BL_PIN);
    pwm_set_wrap(slice_num, 100);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 0);
    pwm_set_clkdiv(slice_num, 50);
    pwm_set_enabled(slice_num, true);

    // SPI Config
    spi_init(LCD_SPI_PORT, 40000 * 1000);
    gpio_set_function(LCD_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI_PIN, GPIO_FUNC_SPI);

    // SPI TX DMA Config
    g_lcd_spi_dma_chan = dma_claim_unused_channel(true);
    g_lcd_spi_dma_cfg = dma_channel_get_default_config(g_lcd_spi_dma_chan);
    channel_config_set_transfer_data_size(&g_lcd_spi_dma_cfg, DMA_SIZE_8);
    channel_config_set_dreq(&g_lcd_spi_dma_cfg, spi_get_dreq(LCD_SPI_PORT, true));
    channel_config_set_read_increment(&g_lcd_spi_dma_cfg, true);
    channel_config_set_write_increment(&g_lcd_spi_dma_cfg, false);
    // I2C Config
    i2c_init(SENSOR_I2C_PORT, 400 * 1000);
    gpio_set_function(DEV_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DEV_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DEV_SDA_PIN);
    gpio_pull_up(DEV_SCL_PIN);

    printf("DEV_Module_Init OK \r\n");
    return 0;
}

/******************************************************************************
function:	Module exits, closes SPI and BCM2835 library
parameter:
Info:
******************************************************************************/
void DEV_Module_Exit(void)
{
    if (g_spi_dma_inflight && g_spi_dma_active_port) {
        DEV_SPI_Wait_Async_Complete(g_spi_dma_active_port);
    }
    if (g_lcd_spi_dma_chan >= 0) {
        dma_channel_unclaim(g_lcd_spi_dma_chan);
        g_lcd_spi_dma_chan = -1;
    }
}
