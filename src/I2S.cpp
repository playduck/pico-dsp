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
// #include <Arduino.h>
#include "I2S.h"
#include "pio_i2s.pio.h"

I2S::I2S(PinMode direction, pin_size_t pinBCLK, pin_size_t pinDOUT, int bps) {
    _running = false;
    _bps = bps;
    _writtenHalf = false;
    _pinBCLK = pinBCLK;
    _pinDOUT = pinDOUT;
    _freq = 48000;
    _arb = nullptr;
    _isOutput = direction == OUTPUT;
    _cb = nullptr;
    _buffers = 32;
    _bufferWords = 64;
    _silenceSample = 0;
}

I2S::~I2S() {
}

bool I2S::setFrequency(int newFreq) {
    _freq = newFreq;
    if (_running) {
        float bitClk = _freq * _bps * 2.0 /* channels */ * 2.0 /* edges per clock */;
        pio_sm_set_clkdiv(_pio, _sm, (float)clock_get_hz(clk_sys) / bitClk);
    }
    return true;
}

void I2S::onTransmit(void(*fn)(void)) {
    if (_isOutput) {
        _cb = fn;
        if (_running) {
            _arb->setCallback(_cb);
        }
    }
}

void I2S::onReceive(void(*fn)(void)) {
    if (!_isOutput) {
        _cb = fn;
        if (_running) {
            _arb->setCallback(_cb);
        }
    }
}

bool I2S::begin() {
    _running = true;
    _hasPeeked = false;
    int off = 0;
    _i2s = new PIOProgram(_isOutput ? &pio_i2s_out_program : &pio_i2s_in_program);
    _i2s->prepare(&_pio, &_sm, &off);
    if (_isOutput) {
        pio_i2s_out_program_init(_pio, _sm, off, _pinDOUT, _pinBCLK, _bps);
    } else {
        pio_i2s_in_program_init(_pio, _sm, off, _pinDOUT, _bps);
    }
    setFrequency(_freq);
    if (_bps == 8) {
        uint8_t a = _silenceSample & 0xff;
        _silenceSample = (a << 24) | (a << 16) | (a << 8) | a;
    } else if (_bps == 16) {
        uint16_t a = _silenceSample & 0xffff;
        _silenceSample = (a << 16) | a;
    }
    _arb = new AudioRingBuffer(_buffers, _bufferWords, _silenceSample, _isOutput ? OUTPUT : INPUT);
    _arb->begin(pio_get_dreq(_pio, _sm, _isOutput), _isOutput ? &_pio->txf[_sm] : (volatile void*)&_pio->rxf[_sm]);
    _arb->setCallback(_cb);
    // pio_sm_set_enabled(_pio, _sm, true);

    return true;
}

void I2S::end() {
    _running = false;
    delete _arb;
    _arb = nullptr;
    delete _i2s;
    _i2s = nullptr;
}

size_t I2S::write(int32_t val, bool sync) {
    if (!_running || !_isOutput) {
        return 0;
    }
    return _arb->write(val, sync);
}

size_t I2S::read(int32_t *val, bool sync) {
    if (!_running || _isOutput) {
        return 0;
    }
    return _arb->read((uint32_t *)val, sync);
}
