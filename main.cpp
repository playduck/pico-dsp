#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "compatability.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/binary_info.h"

#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/divider.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "hardware/vreg.h"

#include "I2S.h"
#include "iir.h"

#include "pio_i2s.pio.h"

const int sampleRate = 44100;
const int bitDepth = 32;
const float mclkFactor = 512.0;

const int input_BCLK_Base = 3;
const int input_DATA = 5;
const int output_BCLK_Base = 6;
const int output_DATA = 8;
const int mclk_pin = 15;

mutex_t _pioMutex; /* external definition in comaptability.h */

int __not_in_flash_func(main)()
{
    /* binary info */
    bi_decl(bi_program_description("pico-dsp - a simple audio dsp"));
    bi_decl(bi_1pin_with_name(input_BCLK_Base, "I2S Input (ADC) BCLK - DON'T USE (invalid timing)"));
    bi_decl(bi_1pin_with_name(input_BCLK_Base + 1, "I2S Input (ADC) LRCK - DON'T USE (invalid timing)"));
    bi_decl(bi_1pin_with_name(input_DATA, "I2S Input (ADC) Data"));
    bi_decl(bi_1pin_with_name(output_BCLK_Base, "I2S Output (DAC) BCLK"));
    bi_decl(bi_1pin_with_name(output_BCLK_Base + 1, "I2S Output (DAC) LRCK"));
    bi_decl(bi_1pin_with_name(output_DATA, "I2S Output (DAC) Data"));
    bi_decl(bi_1pin_with_name(mclk_pin, "I2S MCLK"));

    /* start uart */
    stdio_init_all();

    /* on-board led */
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    /* PIOs wait for IRQ7, so enable before loading */
    irq_set_enabled(7, true);


    /* undervolt core voltage
        might enable higher clock speeds */
    // vreg_set_voltage(VREG_VOLTAGE_1_00);
    // sleep_ms(1000); // vreg settle

    /* overclock system */
    // set_sys_clock_khz(230000, true);
    // sleep_ms(100);

    IIR lowpass1(lowpass,  880, BIQUAD_Q_ORDER_4_1, 0.0, sampleRate);
    IIR lowpass2(lowpass,  880, BIQUAD_Q_ORDER_4_2,  0.0, sampleRate);
    IIR highpass1(highpass, 880, BIQUAD_Q_ORDER_4_1, 0.0, sampleRate);
    IIR highpass2(highpass, 880, BIQUAD_Q_ORDER_4_2,  0.0, sampleRate);

    IIR shaping1(peak, 80, BIQUAD_Q_ORDER_2, 6.0, sampleRate);

    /* load mclk pio */
    int off = 0, sm = 0;
    PIO pio;
    PIOProgram* mclkPio = new PIOProgram(&pio_i2s_mclk_program);
    if(mclkPio->prepare(&pio, &sm, &off))  {
        pio_i2s_mclk_program_init(pio, sm, off, mclk_pin);
        // set mclk to a multiple of fs
        float mclkFrequency = mclkFactor  * (float)sampleRate;
        pio_sm_set_clkdiv(pio, sm, (float)clock_get_hz(clk_sys) / mclkFrequency);
    }   else    {
        printf("failed to allocate MCLK PIO");
        while(1);
    }

    /* initilize I2S */
    I2S I2S_Output(OUTPUT, output_BCLK_Base, output_DATA, bitDepth);
    I2S I2S_Input(INPUT, input_BCLK_Base, input_DATA, bitDepth);

    I2S_Input.setFrequency(sampleRate);
    I2S_Output.setFrequency(sampleRate);

    if (!I2S_Output.begin())
    {
        printf("failed to initialize I2S Output!");
        while (1);
    }

    if (!I2S_Input.begin())
    {
        printf("failed to initialize I2S Input!");
        while (1);
    }

    /* loop variables */
    int32_t left_rx = 0, right_rx = 0;
    int32_t left_tx = 0, right_tx = 0;

    printf("entering main loop");
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    /* synchronously start all pio0s and clocks */
    pio_enable_sm_mask_in_sync(pio0, 0xF);
    /* run PIOs in sync using falling edge of IRQ7 */
    irq_set_enabled(7, false);

    while (1)
    {
        I2S_Input.read(&left_rx, true);
        I2S_Input.read(&right_rx, true);

        /* scale 24 Bit sample to range */
        left_tx = left_rx >> 8;
        right_tx = right_rx >> 8;

        lowpass1.filter(&left_tx); // +0dB
        lowpass2.filter(&left_tx); // +0dB
        shaping1.filter(&left_tx); // +6dB

        highpass1.filter(&right_tx); // +0dB
        highpass2.filter(&right_tx); // +0dB

        /* makeup gain
            +6dB max -> scale by 2^1
            headroom is 2^8 - 2^1 -> 2^7 */
        left_tx <<= 7;
        right_tx <<= 7;

        I2S_Output.write(left_tx, true);
        I2S_Output.write(right_tx, true);
    }

    return 0;
}
