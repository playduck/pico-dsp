#include "iir.h"

void IIR::filter(int32_t *s)
{
    /*
        The state_error is the truncated part of the accumulator.
        This acts as an error, which is fed back (without filter)
        resulting in a rudimentary noise shaping feedback loop.
        One could potentially add an LSB's worth of TPDF dither ontop.
    */
    int64_t accumulator = (int64_t)state_error;

    /* populate the accumulator, the explicit casts are required */
    accumulator += (int64_t)b[0] * (int64_t)(*s);
    accumulator += (int64_t)b[1] * (int64_t)x[0];
    accumulator += (int64_t)b[2] * (int64_t)x[1];
    accumulator += (int64_t)a[0] * (int64_t)y[0];
    accumulator += (int64_t)a[1] * (int64_t)y[1];

    // accumulator = CLAMP(accumulator, ACC_MAX, ACC_MIN);

    /* truncate the result */
    state_error = accumulator & ACC_REM;
    int32_t out = (int32_t)(accumulator >> (int64_t)(q));

    /* shift the delay lines */
    x[1] = x[0];
    y[1] = y[0];

    /* populate the delay lines */
    x[0] = (*s);
    y[0] = out;

    *s = out;
}

// https://www.earlevel.com/main/2011/01/02/biquad-formulas/
IIR::IIR(filter_type_t type, float Fc, float Q, float peakGain, float Fs)
{
    /*
        calculate the iir filter coefficients based on more intuitively
        understandable parameters
        the coefficients get scaled by the selected scaling factor
    */

    float a0 = 0, a1 = 0, a2 = 0, b1 = 0, b2 = 0, norm = 0;

    float V = powf(10, fabsf(peakGain) / 20);
    float K = tanf(M_PI * Fc / Fs);

    switch (type)
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
        /* fall-through */
    default:
        a0 = 1;
        a1 = 0;
        a2 = 0;
        b1 = 0;
        b2 = 0;
        break;
    }

    type = type;

    b[0] = (int32_t)(a0 * scaleQ);
    b[1] = (int32_t)(a1 * scaleQ);
    b[2] = (int32_t)(a2 * scaleQ);

    a[0] = (int32_t)(-b1 * scaleQ);
    a[1] = (int32_t)(-b2 * scaleQ);

    x[0] = 0;
    x[1] = 0;

    y[0] = 0;
    y[1] = 0;

    state_error = 0;
}
