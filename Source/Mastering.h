#pragma once
// Analisis para el master automatico (C++ puro, sin JUCE):
//  - LoudnessMeter: sonoridad aproximada en LUFS (ponderacion K simplificada)
//  - MasterAnalyzer: balance tonal promedio del tema (10 bandas de octava) contra una curva de referencia
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "SimpleFFT.h"

class LoudnessMeter
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        const double pi = 3.141592653589793;

        {   // estante de agudos +4 dB a 1682 Hz
            const double A = std::pow (10.0, 4.0 / 40.0), w0 = 2.0 * pi * 1682.0 / sr;
            const double cw = std::cos (w0), alpha = std::sin (w0) / (2.0 * 0.707), sA = 2.0 * std::sqrt (A) * alpha;
            const double a0 = (A + 1) - (A - 1) * cw + sA;
            shelf.b0 = (float) ((A * ((A + 1) + (A - 1) * cw + sA)) / a0);
            shelf.b1 = (float) ((-2 * A * ((A - 1) + (A + 1) * cw)) / a0);
            shelf.b2 = (float) ((A * ((A + 1) + (A - 1) * cw - sA)) / a0);
            shelf.a1 = (float) ((2 * ((A - 1) - (A + 1) * cw)) / a0);
            shelf.a2 = (float) (((A + 1) - (A - 1) * cw - sA) / a0);
        }
        {   // pasa-altos a 38 Hz
            const double w0 = 2.0 * pi * 38.0 / sr, cw = std::cos (w0), alpha = std::sin (w0) / (2.0 * 0.5);
            const double a0 = 1.0 + alpha;
            hp.b0 = (float) (((1 + cw) / 2) / a0);
            hp.b1 = (float) ((-(1 + cw)) / a0);
            hp.b2 = hp.b0;
            hp.a1 = (float) ((-2 * cw) / a0);
            hp.a2 = (float) ((1 - alpha) / a0);
        }

        shortCoeff = (float) (1.0 - std::exp (-1.0 / (0.4 * sr)));
        longCoeff  = (float) (1.0 - std::exp (-1.0 / (6.0 * sr)));
        reset();
    }

    void reset()
    {
        shelf.clear(); hp.clear();
        shortMs = 0.0f; longMs = 0.0f; primed = false; lufs = -70.0f;
    }

    void process (const float* const* data, int numCh, int n, bool freeze)
    {
        const int chans = std::min (numCh, 2);
        if (chans < 1) return;

        for (int i = 0; i < n; ++i)
        {
            float sumSq = 0.0f;
            for (int ch = 0; ch < chans; ++ch)
            {
                const float y = hp.tick (ch, shelf.tick (ch, data[ch][i]));
                sumSq += y * y;
            }

            shortMs += shortCoeff * (sumSq - shortMs);

            if (! freeze && shortMs > 1.0e-5f)       // compuerta: ignora el silencio (aprox -50 LUFS)
            {
                if (! primed) { longMs = shortMs; primed = true; }
                else          longMs += longCoeff * (shortMs - longMs);
            }
        }

        if (primed) lufs = -0.691f + 10.0f * std::log10 (longMs + 1.0e-12f);
    }

    float getLufs() const { return lufs; }

private:
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float z1[2] { 0, 0 }, z2[2] { 0, 0 };
        void clear() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0f; }
        float tick (int ch, float x)
        {
            const float y = b0 * x + z1[ch];
            z1[ch] = b1 * x - a1 * y + z2[ch];
            z2[ch] = b2 * x - a2 * y;
            return y;
        }
    };

    double sampleRate = 44100.0;
    Biquad shelf, hp;
    float shortCoeff = 0.0f, longCoeff = 0.0f, shortMs = 0.0f, longMs = 0.0f, lufs = -70.0f;
    bool primed = false;
};

//==============================================================================
class MasterAnalyzer
{
public:
    static constexpr int fftSize  = 4096;
    static constexpr int hop      = 2048;
    static constexpr int bins     = fftSize / 2 + 1;
    static constexpr int numBands = 10;     // octavas: 31.5 Hz ... 16 kHz
    static constexpr int numGroups = 6;

    void prepare (double sr) { sampleRate = sr; reset(); }

    void reset()
    {
        ring.fill (0.0f); avg.fill (0.0f); diff.fill (0.0f);
        count = 0; frames = 0;
    }

    void push (const float* const* data, int numCh, int n, bool freeze)
    {
        const int chans = std::min (numCh, 2);
        if (chans < 1) return;

        for (int i = 0; i < n; ++i)
        {
            float m = 0.0f;
            for (int ch = 0; ch < chans; ++ch) m += data[ch][i];
            ring[count & mask] = m / (float) chans;
            ++count;

            if ((count % (uint32_t) hop) == 0 && ! freeze)
                analyse();
        }
    }

    // cuanto se aleja cada banda de la referencia (dB, ya normalizado): + = de mas, - = de menos
    float bandDiffDb (int i) const { return diff[(size_t) i]; }
    bool ready() const { return frames >= 4; }

    // correcciones del EQ (dB) para los 6 filtros: estante 90 Hz, 190, 500, 1.5k, 4k, estante 9k
    void corrections (float amount, float tone, float* out) const
    {
        const float d[numBands] = { diff[0], diff[1], diff[2], diff[3], diff[4], diff[5], diff[6], diff[7], diff[8], diff[9] };
        const float g[numGroups] = { 0.5f * (d[0] + d[1]), 0.5f * (d[2] + d[3]), d[4],
                                     0.5f * (d[5] + d[6]), d[7], 0.5f * (d[8] + d[9]) };
        const float toneGain[numGroups] = { -2.0f, -1.0f, -0.3f, 0.6f, 1.5f, 2.5f };

        for (int i = 0; i < numGroups; ++i)
        {
            float c = 0.0f;
            if (ready()) c = std::max (-3.5f, std::min (3.5f, -g[i] * 0.6f)) * amount;
            out[i] = c + toneGain[i] * tone;
        }
    }

    static float targetDb (int i)
    {
        static const float t[numBands] = { -10, -2, 0, -3, -6, -9, -12, -16, -21, -28 };
        return t[i];
    }

private:
    static constexpr uint32_t mask = (uint32_t) fftSize - 1;

    void analyse()
    {
        float e = 0.0f;
        for (int i = 0; i < fftSize; ++i)
        {
            const float v = ring[(count - (uint32_t) fftSize + (uint32_t) i) & mask];
            e += v * v;
            re[(size_t) i] = v * fft.window (i);
            im[(size_t) i] = 0.0f;
        }

        if (std::sqrt (e / (float) fftSize) < 0.0018f) return;     // silencio (menos de -55 dBFS): no aprende

        fft.run (re.data(), im.data(), false);

        const float hopSec = (float) hop / (float) sampleRate;
        const float w = std::max (hopSec / 8.0f, 1.0f / (float) (frames + 1));

        for (int k = 0; k < bins; ++k)
        {
            const float p = re[(size_t) k] * re[(size_t) k] + im[(size_t) k] * im[(size_t) k];
            avg[(size_t) k] += (p - avg[(size_t) k]) * w;
        }
        ++frames;

        const float binHz = (float) sampleRate / (float) fftSize;
        float db[numBands], tg[numBands];
        float mDb = 0.0f, mTg = 0.0f;

        for (int b = 0; b < numBands; ++b)
        {
            const float fc = 31.5f * std::pow (2.0f, (float) b);
            const int k0 = std::max (1, (int) std::ceil (fc / 1.41421356f / binHz));
            int k1 = std::min (bins - 1, (int) std::floor (fc * 1.41421356f / binHz));
            if (k1 < k0) k1 = k0;

            float sum = 0.0f;
            for (int k = k0; k <= k1; ++k) sum += avg[(size_t) k];

            db[b] = 10.0f * std::log10 (sum + 1.0e-20f);
            tg[b] = targetDb (b);
            if (b >= 2 && b <= 7) { mDb += db[b]; mTg += tg[b]; }
        }

        mDb /= 6.0f; mTg /= 6.0f;
        for (int b = 0; b < numBands; ++b)
            diff[(size_t) b] = (db[b] - mDb) - (tg[b] - mTg);
    }

    double sampleRate = 44100.0;
    SimpleFFT<fftSize> fft;
    std::array<float, fftSize> ring {}, re {}, im {};
    std::array<float, bins> avg {};
    std::array<float, numBands> diff {};
    uint32_t count = 0;
    int frames = 0;
};
