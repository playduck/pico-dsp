# PICO-DSP

Audio DSP Project for the Raspberry Pi Pico (RP2040).
The project reads an I2S Input Stream, performs signal processing and outputs it as an I2S stream.
The pico acts as the I2S master generating all required clocks (including MCLK).

The I2S interfaces are realized via the PIOs.
The project takes strong influence from the [`pico-extras`](https://github.com/raspberrypi/pico-extras) and the [`arduino-pico`](https://github.com/earlephilhower/arduino-pico)  repositories.

DSP is done at 44.1kHz at 32 Bits.
For optimal performance the RP2040 is overclocked to around 230 MHz and undervolted to 1.0V.
Tested using a `Rpi Pico`, a `PCM5102A` DAC and a `WM8782` ADC.

**Use the DAC Clocks (DAC WS and DAC BCK) for both the ADC and DAC**.
The ADC Clocks are shifted by one `nop` statement.

## Building

Requires pico-sdk and pico-extras.

```bash
mkdir build
cd build
cmake ..
make
```

Proceed to flash the pico with the generated `.elf`.

## TODO

- Add more DSP functionality
- Cleanup unused clock outputs (I2S PIO In)
- try to optimize performance
- more docs

## Further resources

- The great [earlevel engineering blog](https://www.earlevel.com/main/) is a great resource for IIR Filters and various DSP subjects.
- My own projects [ESP32 Bluetooth DSP](https://github.com/playduck/esp32-bluetooth-dsp) and it's [DSP Playground](https://github.com/playduck/dsp-playground) are peers to this project.
