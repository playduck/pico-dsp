# PICO-DSP

## Functionality

Audio DSP Project for the Raspberry Pi Pico (RP2040).
The project reads an I2S input stream, performs signal processing and outputs it as an I2S stream.
The pico acts as the I2S master generating all required clocks (including MCLK).

DSP is done at 44.1kHz at 32 Bits.
For optimal performance the RP2040 is overclocked to around 230 MHz and undervolted to 1.0V.
Tested using a `Rpi Pico`, a `PCM5102A` DAC and a `WM8782` ADC.

## Usage

The I2S interfaces are realized via the PIOs.
The project takes strong influence from the [`pico-extras`](https://github.com/raspberrypi/pico-extras) and the [`arduino-pico`](https://github.com/earlephilhower/arduino-pico) repositories.

**Use the DAC Clocks (DAC WS and DAC BCK) for both the ADC and DAC**.
The ADC Clocks are shifted by one `nop` statement.

The I2S communication works by using 3 PIOs.
One transmitter, one receiver and a clock generator.
The transmitter and receiver are clocked at the bitclock frequency (Bits * Channels * FS), while MCLK is run at it's own frequency (x * FS).
These are synchronized using IRQ7.
This could perhaps be consolidated into just two or even only one PIO.

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

- hardware documentation
- restructure Ringbuffer
- Add more DSP functionality
- Cleanup unused clock outputs (I2S PIO In)
- try to optimize performance
- more docs

## Further resources

- The great [earlevel engineering blog](https://www.earlevel.com/main/) is a great resource for IIR Filters and various DSP subjects.
- My own projects [ESP32 Bluetooth DSP](https://github.com/playduck/esp32-bluetooth-dsp) and it's [DSP Playground](https://github.com/playduck/dsp-playground) are peers to this project.
