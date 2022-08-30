/*
    I2SIn and I2SOut for Raspberry Pi Pico
    Implements one or more I2S interfaces using DMA

    Copyright (c) 2022 Earle F. Philhower, III <earlephilhower@yahoo.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once
// #include <Arduino.h>
#include <vector>
#include "AudioPioRingBuffer.h"

class I2S{
public:
    I2S(PinMode direction = OUTPUT, pin_size_t pinBCLK = 0, pin_size_t pinDOUT = 2, int bps = 32);
    virtual ~I2S();

    bool setFrequency(int newFreq);

    bool begin();
    void end();

    // // from Stream
    // virtual int available() override;
    // virtual int read() override;
    // virtual int peek() override;
    // virtual void flush() override;

    // // from Print (see notes on write() methods below)
    // virtual size_t write(const uint8_t *buffer, size_t size) override;
    // virtual int availableForWrite() override;

    // Try and make I2S::write() do what makes sense, namely write
    // one sample (L or R) at the I2S configured bit width
    // virtual size_t write(uint8_t s) override {
    //     return _writeNatural(s & 0xff);
    // }

    // Write 32 bit value to port, user responsbile for packing/alignment, etc.
    size_t write(int32_t val, bool sync);

    // Write sample to I2S port, will block until completed
    size_t write8(int8_t l, int8_t r);
    size_t write16(int16_t l, int16_t r);
    size_t write24(int32_t l, int32_t r); // Note that 24b must have values left-aligned (i.e. 0xABCDEF00)
    size_t write32(int32_t l, int32_t r);

    // Read 32 bit value to port, user responsbile for packing/alignment, etc.
    size_t read(int32_t *val, bool sync);

    // Note that these callback are called from **INTERRUPT CONTEXT** and hence
    // should be in RAM, not FLASH, and should be quick to execute.
    void onTransmit(void(*)(void));
    void onReceive(void(*)(void));

private:
    pin_size_t _pinBCLK;
    pin_size_t _pinDOUT;
    int _bps;
    int _freq;
    size_t _buffers;
    size_t _bufferWords;
    int32_t _silenceSample;
    bool _isOutput;

    bool _running;

    bool _hasPeeked;
    int32_t _peekSaved;

    size_t _writeNatural(int32_t s);
    uint32_t _writtenData;
    bool _writtenHalf;

    int32_t _holdWord = 0;
    int _wasHolding = 0;

    void (*_cb)();

    AudioRingBuffer *_arb;
    PIOProgram *_i2s;
    PIO _pio;
    int _sm;
};
