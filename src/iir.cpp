#include "iir.h"
#include <math.h>

void IIR::filter(float *s)
{
    float out = b[0] * (*s) + d[0];
    d[0] = b[1] * (*s) + a[1] * out + d[1];
    d[1] = b[2] * (*s) + a[2] * out;
    *s = out;
}

// https://www.earlevel.com/main/2011/01/02/biquad-formulas/
IIR::IIR(filter_type_t _type, float Fc, float Q, float peakGain, float Fs)
{
    float a0 = 0.0f, a1 = 0.0f, a2 = 0.0f, b1 = 0.0f, b2 = 0.0f, norm = 0.0f;

    float V = powf(10.0f, fabsf(peakGain) / 20.0f);
    float K = tanf(M_PI * Fc / Fs);

    switch (_type)
    {
    case lowpass:
        norm = 1 / (1 + K / Q + K * K);
        a0 = K * K * norm;
        a1 = 2 * a0;
        a2 = a0;
        b1 = 2 * (K * K - 1) * norm;
        b2 = (1 - K / Q + K * K) * norm;
        break;

    case highpass:
        norm = 1 / (1 + K / Q + K * K);
        a0 = 1 * norm;
        a1 = -2 * a0;
        a2 = a0;
        b1 = 2 * (K * K - 1) * norm;
        b2 = (1 - K / Q + K * K) * norm;
        break;

    case bandpass:
        norm = 1 / (1 + K / Q + K * K);
        a0 = K / Q * norm;
        a1 = 0;
        a2 = -a0;
        b1 = 2 * (K * K - 1) * norm;
        b2 = (1 - K / Q + K * K) * norm;
        break;

    case notch:
        norm = 1 / (1 + K / Q + K * K);
        a0 = (1 + K * K) * norm;
        a1 = 2 * (K * K - 1) * norm;
        a2 = a0;
        b1 = a1;
        b2 = (1 - K / Q + K * K) * norm;
        break;

    case peak:
        if (peakGain >= 0)
        { // boost
            norm = 1 / (1 + 1 / Q * K + K * K);
            a0 = (1 + V / Q * K + K * K) * norm;
            a1 = 2 * (K * K - 1) * norm;
            a2 = (1 - V / Q * K + K * K) * norm;
            b1 = a1;
            b2 = (1 - 1 / Q * K + K * K) * norm;
        }
        else
        { // cut
            norm = 1 / (1 + V / Q * K + K * K);
            a0 = (1 + 1 / Q * K + K * K) * norm;
            a1 = 2 * (K * K - 1) * norm;
            a2 = (1 - 1 / Q * K + K * K) * norm;
            b1 = a1;
            b2 = (1 - V / Q * K + K * K) * norm;
        }
        break;
    case lowshelf:
        if (peakGain >= 0)
        { // boost
            norm = 1 / (1 + M_SQRT2 * K + K * K);
            a0 = (1 + sqrtf(2 * V) * K + V * K * K) * norm;
            a1 = 2 * (V * K * K - 1) * norm;
            a2 = (1 - sqrtf(2 * V) * K + V * K * K) * norm;
            b1 = 2 * (K * K - 1) * norm;
            b2 = (1 - M_SQRT2 * K + K * K) * norm;
        }
        else
        { // cut
            norm = 1 / (1 + sqrtf(2 * V) * K + V * K * K);
            a0 = (1 + M_SQRT2 * K + K * K) * norm;
            a1 = 2 * (K * K - 1) * norm;
            a2 = (1 - M_SQRT2 * K + K * K) * norm;
            b1 = 2 * (V * K * K - 1) * norm;
            b2 = (1 - sqrtf(2 * V) * K + V * K * K) * norm;
        }
        break;
    case highshelf:
        if (peakGain >= 0)
        { // boost
            norm = 1 / (1 + M_SQRT2 * K + K * K);
            a0 = (V + sqrtf(2 * V) * K + K * K) * norm;
            a1 = 2 * (K * K - V) * norm;
            a2 = (V - sqrtf(2 * V) * K + K * K) * norm;
            b1 = 2 * (K * K - 1) * norm;
            b2 = (1 - M_SQRT2 * K + K * K) * norm;
        }
        else
        { // cut
            norm = 1 / (V + sqrtf(2 * V) * K + K * K);
            a0 = (1 + M_SQRT2 * K + K * K) * norm;
            a1 = 2 * (K * K - 1) * norm;
            a2 = (1 - M_SQRT2 * K + K * K) * norm;
            b1 = 2 * (K * K - V) * norm;
            b2 = (V - sqrtf(2 * V) * K + K * K) * norm;
        }
        break;
    case none:
    default:
        type = _type;
        a0 = 0.0f;
        a1 = 0.0f;
        a2 = 0.0f;
        b1 = 0.0f;
        b2 = 0.0f;
        break;
    }

    type = _type;
    b[0] = a0;
    b[1] = a1;
    b[2] = a2;
    a[0] = 1.0f;
    a[1] = -b1;
    a[2] = -b2;
    d[0] = 0.0f;
    d[1] = 0.0f;
}
