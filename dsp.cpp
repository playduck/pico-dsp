#include <stdio.h>
#include <math.h>

#include "compatability.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/divider.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "hardware/vreg.h"

#include "I2S.h"
#include "iir.h"

#include "pio_i2s.pio.h"

#define DEBUG_PIN 9

#define CLAMP(x, max, min) (x > max ? max : (x < min ? min : x))

const int sampleRate = 44100;
const int bitdepth = 32;

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

    printf("main entry\n");

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
        pio_i2s_mclk_program_init(pio, sm, off, 15);
        pio_sm_set_clkdiv(pio, sm, (float)clock_get_hz(clk_sys) / ((float)sampleRate * 512.0));
        // pio_sm_set_clkdiv_int_frac(pio, sm, 1, 10);
        printf("allocated MCLK PIO\n");
    }   else    {
        printf("failed to allocate MCLK PIO\n");
    }

    I2S_Input.setBCLK(3);
    I2S_Input.setDATA(5);
    I2S_Output.setBCLK(6);
    I2S_Output.setDATA(8);

    I2S_Input.setBitsPerSample(32);
    I2S_Output.setBitsPerSample(32);

    I2S_Input.setBuffers(16, 32, 0);
    I2S_Output.setBuffers(16, 32, 0);

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
