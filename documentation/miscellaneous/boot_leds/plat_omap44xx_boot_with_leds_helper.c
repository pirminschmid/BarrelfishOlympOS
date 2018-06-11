/**
 * LED blink helpers to debug booting of 2nd core
 * code is based on milestone 0 code
 * 
 * version 2017-11-10, pisch
 */

#include <kernel.h>

void boot_leds_init(void);
void boot_leds_one(void);
void boot_leds_two(void);
void boot_leds_both(void);
void boot_leds_off(void);
void boot_leds_wait(void);

// LED D2: GPIO_WK8
#define GPIO1_ADDR_START ((uint32_t *)0x4a310000)
#define LED_D2_OE ((uint32_t *)0x4a310134)
#define LED_D2_DATAOUT ((uint32_t *)0x4a31013c)
#define LED_D2_BIT (1<<8)

// note: output enable is 0


// LED D1: adjust mux
#define REG_CONTROL_CORE_PAD0_SDMMC1_DAT7_PAD1_ABE_MCBSP2_CLKX ((uint32_t *)0x4A1000F4)
#define SHIFT_ABE_MCBSP2_CLKX_MUXMODE 16
#define RESET_GPIO_110_MUX (7<<SHIFT_ABE_MCBSP2_CLKX_MUXMODE)
#define SELECT_GPIO_110 (3<<SHIFT_ABE_MCBSP2_CLKX_MUXMODE)

// LED D1: GPIO_110
#define GPIO4_ADDR_START ((uint32_t *)0x48059000)
#define LED_D1_OE ((uint32_t *)0x48059134)
#define LED_D1_DATAOUT ((uint32_t *)0x4805913C)
#define LED_D1_BIT (1<<(110-96))



void boot_leds_init(void)
{
    volatile uint32_t *d2_oe = LED_D2_OE;

    volatile uint32_t *d1_mux_reg = REG_CONTROL_CORE_PAD0_SDMMC1_DAT7_PAD1_ABE_MCBSP2_CLKX;
    volatile uint32_t *d1_oe = LED_D1_OE;

    // prepare D1 mux
    *d1_mux_reg &= ~RESET_GPIO_110_MUX;
    *d1_mux_reg |= SELECT_GPIO_110;

    // prepare D2 and D1 OE (output is on if bit == 0)
    // note: 1 for Input, 0 for Output
    *d2_oe &= ~LED_D2_BIT;
    *d1_oe &= ~LED_D1_BIT;
}

void boot_leds_one(void)
{
    volatile uint32_t *d1_do = LED_D1_DATAOUT;
    *d1_do |= LED_D1_BIT;
}

void boot_leds_two(void)
{
    volatile uint32_t *d2_do = LED_D2_DATAOUT;
    *d2_do |= LED_D2_BIT;
}

void boot_leds_both(void)
{
    volatile uint32_t *d1_do = LED_D1_DATAOUT;
    volatile uint32_t *d2_do = LED_D2_DATAOUT;
    *d1_do |= LED_D1_BIT;
    *d2_do |= LED_D2_BIT;
}

void boot_leds_off(void)
{
    volatile uint32_t *d1_do = LED_D1_DATAOUT;
    volatile uint32_t *d2_do = LED_D2_DATAOUT;
    *d2_do &= ~LED_D2_BIT;
    *d1_do &= ~LED_D1_BIT;
}


#define SLEEP_CONST 300
#define SLEEP_TIME_MS 100
#define SLEEP (SLEEP_CONST * SLEEP_CONST * SLEEP_TIME_MS)


void boot_leds_wait(void)
{
    volatile int dummy = 0;
    for (int i = 0; i < SLEEP; i++) {
        dummy += 1;
    }
}
