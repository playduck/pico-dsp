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

#define CLAMP(x, max, min) (x > max ? max : (x < min ? min : x))

const int sampleRate = 44100;
const int bitDepth = 32;
const float mclkFactor = 512.0;

const int input_BCLK_Base = 3;
const int input_DATA = 5;
const int output_BCLK_Base = 6;
const int output_DATA = 8;
const int mclk_pin = 15;

bool ret;
volatile bool receiveFlag = false;

mutex_t _pioMutex;

inline float __not_in_flash_func(fixed_to_float)(int32_t *s)
{
    return (float)((float)(*s) / (float)INT32_MAX);
}

inline int32_t __not_in_flash_func(float_to_fixed)(float* s)
{
    // hard clipping
    *s = CLAMP(*s, 1.0f, -1.0f);

    return (int32_t)(roundf(*s * (float)INT32_MAX));
}

int __not_in_flash_func(main)()
{
    stdio_init_all();

    // binary info
    bi_decl(bi_program_description("pico-dsp - a simple audio dsp"));
    bi_decl(bi_1pin_with_name(input_BCLK_Base, "I2S Input (ADC) BCLK, +1 is LRCK - DON'T USE (invalid timing)"));
    bi_decl(bi_1pin_with_name(input_DATA, "I2S Input (ADC) Data"));
    bi_decl(bi_1pin_with_name(output_BCLK_Base, "I2S Output (DAC) BCLK, +1 is LRCK"));
    bi_decl(bi_1pin_with_name(output_DATA, "I2S Output (DAC) Data"));
    bi_decl(bi_1pin_with_name(mclk_pin, "I2S MCLK"));

    // PIOs wait for IRQ7, so enable before loading
    irq_set_enabled(7, true);

    // undervolt core voltage
    // might enable higher clock speeds (smaller voltage slopes)
    vreg_set_voltage(VREG_VOLTAGE_1_00);
    sleep_ms(1000); // vreg settle

    // overclock system
    set_sys_clock_khz(230000, true);
    sleep_ms(100);

    I2S I2S_Output(OUTPUT);
    I2S I2S_Input(INPUT);

    IIR lowpass1(lowpass,  880, BIQUAD_Q_ORDER_4_1, 0.0, sampleRate);
    IIR lowpass2(lowpass,  880, BIQUAD_Q_ORDER_4_2,  0.0, sampleRate);
    IIR highpass1(highpass, 880, BIQUAD_Q_ORDER_4_1, 0.0, sampleRate);
    IIR highpass2(highpass, 880, BIQUAD_Q_ORDER_4_2,  0.0, sampleRate);

    IIR shaping1(peak, 80, BIQUAD_Q_ORDER_2, 6.0, sampleRate);

    // load mclk pio
    int off = 0, sm = 0;
    PIO pio;
    PIOProgram* mclk = new PIOProgram(&pio_i2s_mclk_program);
    if(mclk->prepare(&pio, &sm, &off))  {
        pio_i2s_mclk_program_init(pio, sm, off, mclk_pin);
        // set mclk to a multiple of fs
        pio_sm_set_clkdiv(pio, sm, (float)clock_get_hz(clk_sys) / (mclkFactor  * (float)sampleRate));
        printf("allocated MCLK PIO\n");
    }   else    {
        printf("failed to allocate MCLK PIO\n");
    }

    I2S_Input.setBCLK(input_BCLK_Base);
    I2S_Input.setDATA(input_DATA);
    I2S_Output.setBCLK(output_BCLK_Base);
    I2S_Output.setDATA(output_DATA);

    I2S_Input.setBitsPerSample(bitDepth);
    I2S_Output.setBitsPerSample(bitDepth);

    // increase buffer sizes
    I2S_Input.setBuffers(32, 64, 0);
    I2S_Output.setBuffers(32, 64, 0);

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

    printf("entering main loop\n");

    int32_t lr = 0, rr = 0;
    volatile int32_t lt = 0, rt = 0;
    float p1, p2;

    // sync all pio0 clocks
    pio_enable_sm_mask_in_sync(pio0, 0xF);
    // start PIOs in sync using falling edge of IRQ7
    irq_set_enabled(7, false);

    while (1)
    {
        I2S_Input.read32(&lr, &rr);

        p1 = fixed_to_float(&lr);
        p2 = fixed_to_float(&rr);

        lowpass1.filter(&p1);
        lowpass2.filter(&p1);
        shaping1.filter(&p1);

        highpass1.filter(&p2);
        highpass2.filter(&p2);

        lt = float_to_fixed(&p1);
        rt = float_to_fixed(&p2);

        I2S_Output.write32(lt,rt);
    }

    return 0;
}
