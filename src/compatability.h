#pragma once
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "pico/mutex.h"

#define PinMode uint8_t

#define OUTPUT (1)
#define INPUT (0)

typedef uint8_t pin_size_t;

class Stream
{
public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;

    virtual int availableForWrite()
    {
        return 0;
    }

    virtual void flush() {}

    virtual size_t write(uint8_t) = 0;
    size_t write(const char *str)
    {
        if (str == NULL)
            return 0;
        return write((const uint8_t *)str, strlen(str));
    }

    size_t write(const char *buffer, size_t size)
    {
        return write((const uint8_t *)buffer, size);
    }

    virtual size_t write(const uint8_t *buffer, size_t size)
    {
        size_t n = 0;
        while (size--)
        {
            if (write(*buffer++))
                n++;
            else
                break;
        }
        return n;
    }
};

class CoreMutex
{
public:
    CoreMutex(mutex_t *mutex)
    {
        uint32_t owner;
        _mutex = mutex;
        _acquired = false;
        if (!mutex_try_enter(_mutex, &owner))
        {
            if (owner == get_core_num())
            { // Deadlock!
                printf("Deadlock detected!\n");
                // DEBUGCORE("CoreMutex - Deadlock detected!\n");
                return;
            }
            mutex_enter_blocking(_mutex);
        }
        _acquired = true;
    }

    ~CoreMutex()
    {
        if (_acquired)
        {
            mutex_exit(_mutex);
        }
    }

    operator bool()
    {
        return _acquired;
    }

private:
    mutex_t *_mutex;
    bool _acquired;
};

class PIOProgram
{
public:
    PIOProgram(const pio_program_t *pgm)
    {
        _pgm = pgm;
    }

    // Possibly load into a PIO and allocate a SM
    bool prepare(PIO *pio, int *sm, int *offset)
    {
        extern mutex_t _pioMutex;
        CoreMutex m(&_pioMutex);
        // Is there an open slot to run in, first?
        if (!_findFreeSM(pio, sm))
        {
            return false;
        }
        // Is it loaded on that PIO?
        if (_offset[pio_get_index(*pio)] < 0)
        {
            // Nope, need to load it
            if (!pio_can_add_program(*pio, _pgm))
            {
                return false;
            }
            _offset[pio_get_index(*pio)] = pio_add_program(*pio, _pgm);
        }
        // Here it's guaranteed loaded, return values
        // PIO and SM already set
        *offset = _offset[pio_get_index(*pio)];
        return true;
    }

private:
    // Find an unused PIO state machine to grab, returns false when none available
    static bool _findFreeSM(PIO *pio, int *sm)
    {
        int idx = pio_claim_unused_sm(pio0, false);
        if (idx >= 0)
        {
            *pio = pio0;
            *sm = idx;
            return true;
        }
        idx = pio_claim_unused_sm(pio1, false);
        if (idx >= 0)
        {
            *pio = pio1;
            *sm = idx;
            return true;
        }
        return false;
    }

private:
    int _offset[2] = {-1, -1};
    const pio_program_t *_pgm;
};
