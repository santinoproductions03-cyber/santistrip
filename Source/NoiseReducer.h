#pragma once
// Reductor de ruido espectral en C++ puro (sin JUCE).
// Aprende solo el "piso" de ruido de cada frecuencia y baja lo que queda por debajo.
// Latencia fija = latency muestras.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

class NoiseReducer
{
public:
    static constexpr int fftSize = 1024;
    static constexpr int hop     = 256;
    static constexpr int bins    = fftSize / 2 + 1;
    static constexpr int latency = fftSize;

    struct Settings { float reductionDb = 15.0f; float sens = 0.5f; bool freeze = false; };

    NoiseReducer() { initTables(); reset(); }

    void prepare (double) { reset(); }
    void reset() { for (auto& c : chan) c.reset(); }

    void process (float* const* data, int numChannels, int numSamples, const Settings& s)
    {
        const int chans = std::min (numChannels, 2);
        const float floorGain = std::pow (10.0f, -std::max (0.0f, s.reductionDb) / 20.0f);
        const float beta = 1.6f + 2.4f * s.sens;

        for (int ch = 0; ch < chans; ++ch)
        {
            auto& c = chan[(size_t) ch];
            float* x = data[ch];

            for (int i = 0; i < numSamples; ++i)
            {
                c.inRing[c.count & mask] = x[i];
                ++c.count;

                if ((c.count % (uint32_t) hop) == 0)
                    processFrame (c, floorGain, beta, s.freeze);

                const uint32_t idx = (c.count - (uint32_t) fftSize) & mask;
                x[i] = c.accum[idx];
                c.accum[idx] = 0.0f;
            }
        }
    }

private:
    static constexpr uint32_t mask = (uint32_t) fftSize - 1;

    struct Chan
    {
        std::array<float, fftSize> inRing {}, accum {};
        std::array<float, bins> noise {}, smooth {}, gain {};
        uint32_t count = 0;
        int frames = 0;
        bool primed = false;

        void reset()
        {
            inRing.fill (0.0f); accum.fill (0.0f);
            noise.fill (0.0f); smooth.fill (0.0f); gain.fill (1.0f);
            count = 0; frames = 0; primed = false;
        }
    };

    void initTables()
    {
        const double twoPi = 6.283185307179586;

        for (int k = 0; k < fftSize / 2; ++k)
        {
            cosT[(size_t) k] = (float) std::cos (twoPi * k / fftSize);
            sinT[(size_t) k] = (float) std::sin (twoPi * k / fftSize);
        }

        int bitsCount = 0;
        while ((1 << bitsCount) < fftSize) ++bitsCount;

        for (int i = 0; i < fftSize; ++i)
        {
            int r = 0;
            for (int b = 0; b < bitsCount; ++b)
                if (i & (1 << b)) r |= 1 << (bitsCount - 1 - b);
            rev[(size_t) i] = r;
            win[(size_t) i] = (float) (0.5 - 0.5 * std::cos (twoPi * i / fftSize));
        }
    }

    void fft (bool inverse)
    {
        float* r = re.data();
        float* m = im.data();

        for (int i = 0; i < fftSize; ++i)
        {
            const int j = rev[(size_t) i];
            if (j > i) { std::swap (r[i], r[j]); std::swap (m[i], m[j]); }
        }

        for (int len = 2; len <= fftSize; len <<= 1)
        {
            const int half = len >> 1, step = fftSize / len;

            for (int i = 0; i < fftSize; i += len)
                for (int j = 0; j < half; ++j)
                {
                    const float wr = cosT[(size_t) (j * step)];
                    const float wi = inverse ? sinT[(size_t) (j * step)] : -sinT[(size_t) (j * step)];
                    const int a = i + j, b = a + half;
                    const float tr = r[b] * wr - m[b] * wi;
                    const float ti = r[b] * wi + m[b] * wr;
                    r[b] = r[a] - tr; m[b] = m[a] - ti;
                    r[a] += tr;       m[a] += ti;
                }
        }
    }

    void processFrame (Chan& c, float floorGain, float beta, bool freeze)
    {
        for (int i = 0; i < fftSize; ++i)
        {
            re[(size_t) i] = c.inRing[(c.count - (uint32_t) fftSize + (uint32_t) i) & mask] * win[(size_t) i];
            im[(size_t) i] = 0.0f;
        }

        fft (false);

        const float floorPow = floorGain * floorGain;

        for (int k = 0; k < bins; ++k)
        {
            const float p = re[(size_t) k] * re[(size_t) k] + im[(size_t) k] * im[(size_t) k];
            if (! c.primed) c.smooth[(size_t) k] = p;
            else            c.smooth[(size_t) k] = 0.8f * c.smooth[(size_t) k] + 0.2f * p;
        }

        for (int k = 0; k < bins; ++k)
        {
            const float a = c.smooth[(size_t) std::max (0, k - 1)];
            const float b = c.smooth[(size_t) k];
            const float d = c.smooth[(size_t) std::min (bins - 1, k + 1)];
            const float sm = 0.25f * (a + 2.0f * b + d);

            float& n = c.noise[(size_t) k];
            if (! c.primed) n = sm;

            if (! freeze && c.frames < 40)
            {
                n += (sm - n) * 0.15f;                           // al principio: aprende rapido
            }
            else if (! freeze)
            {
                if (sm < n)             n += (sm - n) * 0.03f;   // baja
                else if (sm < 8.0f * n) n += (sm - n) * 0.03f;   // sigue al ruido
                else                    n *= 1.002f;             // sonido fuerte (voz): casi no lo toma
            }

            const float gp = 1.0f - beta * n / (sm + 1.0e-20f);
            const float target = gp <= floorPow ? floorGain : std::sqrt (gp);

            float& g = c.gain[(size_t) k];
            g += (target - g) * (target > g ? 0.7f : 0.35f);
        }
        c.primed = true;
        if (c.frames < 1000000) ++c.frames;

        // suavizado entre frecuencias vecinas (evita el ruido "musical")
        for (int k = 0; k < bins; ++k)
        {
            const float a = c.gain[(size_t) std::max (0, k - 1)];
            const float b = c.gain[(size_t) k];
            const float d = c.gain[(size_t) std::min (bins - 1, k + 1)];
            gs[(size_t) k] = 0.25f * (a + 2.0f * b + d);
        }

        for (int k = 0; k < bins; ++k)
        {
            re[(size_t) k] *= gs[(size_t) k];
            im[(size_t) k] *= gs[(size_t) k];
            if (k > 0 && k < fftSize / 2)
            {
                re[(size_t) (fftSize - k)] *= gs[(size_t) k];
                im[(size_t) (fftSize - k)] *= gs[(size_t) k];
            }
        }

        fft (true);

        const float scale = 1.0f / ((float) fftSize * 1.5f);
        for (int i = 0; i < fftSize; ++i)
            c.accum[(c.count - (uint32_t) fftSize + (uint32_t) i) & mask] += re[(size_t) i] * win[(size_t) i] * scale;
    }

    std::array<Chan, 2> chan;
    std::array<float, fftSize / 2> cosT {}, sinT {};
    std::array<int, fftSize> rev {};
    std::array<float, fftSize> win {}, re {}, im {};
    std::array<float, bins> gs {};
};
