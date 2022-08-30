#ifndef IIR_H
#define IIR_H
#pragma once

#include <math.h>
#include <stdint.h>

#define CLAMP(x, a, b) (x > a ? a : (x < b ? b : x))

#define q (30)
#define scaleQ (powf(2.0, q))

#define ACC_MAX ((int64_t)  0x7FFFFFFFFF)
#define ACC_MIN ((int64_t) -0x8000000000)
#define ACC_REM ((uint64_t)0x3FFFFFFFU)

#define BIQUAD_Q_ORDER_2 0.70710678
#define BIQUAD_Q_ORDER_4_1 0.54119610
#define BIQUAD_Q_ORDER_4_2 1.3065630

typedef enum
{
    lowpass,
    highpass,
    bandpass,
    notch,
    peak,
    lowshelf,
    highshelf,
    none
} filter_type_t;

class IIR {
private:
    int32_t a[2];
    int32_t b[3];

    int32_t x[2];
    int32_t y[2];
    int32_t state_error;

public:
    filter_type_t type;

    void filter(int32_t *s);
    IIR(filter_type_t type, float Fc, float Q, float peakGain, float Fs);
};

#endif
