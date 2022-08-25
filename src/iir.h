#ifndef IIR_H
#define IIR_H
#pragma once

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
    float a[3];
    float b[3];
    float d[2];

public:
    filter_type_t type;

    void filter(float *s);
    IIR(filter_type_t type, float Fc, float Q, float peakGain, float Fs);
};
#endif
