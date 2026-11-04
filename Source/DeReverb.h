#pragma once
// Quita-reverb espectral en C++ puro (sin JUCE).
// Estima la "cola" de reverberacion de cada frecuencia a partir de lo que sono unos
// 50 ms antes (con la caida del tamano del cuarto) y la resta. Latencia fija = latency.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "SimpleFFT.h"

class DeReverb
{
public:
    static constexpr int fftSize = 1024;
    static constexpr int hop     = 256;
    static constexpr int bins    = fftSize / 2 + 1;
    static constexpr int latency = fftSize;

    struct Settings { float amount = 0.5f; float roomSeconds = 0.8f; };

    DeReverb() { reset(); }

    void prepare (double sr) { sampleRate = sr; reset(); }
    void reset() { for (auto& c : chan) c.reset(); }

    void process (float* const* data, int numChannels, int numSamples, const Settings& s)
    {
        const int chans = std::min (numChannels, 2);
        const float floorGain = std::pow (10.0f, -(std::max (0.0f, s.amount) * 18.0f) / 20.0f);
        const float alpha = 0.7f + 2.3f * s.amount;

        const float hopSec = (float) hop / (float) sampleRate;
        const int delayFrames = std::max (2, (int) std::lround (0.05f / hopSec));
        const float t60 = std::max (0.1f, s.roomSeconds);
        const float decay = std::pow (10.0f, -6.0f * (float) delayFrames * hopSec / t60);

        for (int ch = 0; ch < chans; ++ch)
        {
            auto& c = chan[(size_t) ch];
            float* x = data[ch];

            for (int i = 0; i < numSamples; ++i)
            {
                c.inRing[c.count & mask] = x[i];
                ++c.count;

                if ((c.count % (uint32_t) hop) == 0)
                    processFrame (c, floorGain, alpha, delayFrames, decay);

                const uint32_t idx = (c.count - (uint32_t) fftSize) & mask;
                x[i] = c.accum[idx];
                c.accum[idx] = 0.0f;
            }
        }
    }

private:
    static constexpr uint32_t mask = (uint32_t) fftSize - 1;
    static constexpr int histLen = 64;

    struct Chan
    {
        std::array<float, fftSize> inRing {}, accum {};
        std::array<float, bins> smooth {}, gain {};
        std::array<std::array<float, bins>, histLen> hist {};
        uint32_t count = 0;
        int histPos = 0;
        bool primed = false;

        void reset()
        {
            inRing.fill (0.0f); accum.fill (0.0f);
            smooth.fill (0.0f); gain.fill (1.0f);
            for (auto& h : hist) h.fill (0.0f);
            count = 0; histPos = 0; primed = false;
        }
    };

    void processFrame (Chan& c, float floorGain, float alpha, int delayFrames, float decay)
    {
        for (int i = 0; i < fftSize; ++i)
        {
            re[(size_t) i] = c.inRing[(c.count - (uint32_t) fftSize + (uint32_t) i) & mask] * fft.window (i);
            im[(size_t) i] = 0.0f;
        }

        fft.run (re.data(), im.data(), false);

        const float floorPow = floorGain * floorGain;
        auto& cur = c.hist[(size_t) c.histPos];
        const auto& old = c.hist[(size_t) ((c.histPos - delayFrames + histLen) % histLen)];

        for (int k = 0; k < bins; ++k)
        {
            const float p = re[(size_t) k] * re[(size_t) k] + im[(size_t) k] * im[(size_t) k];
            c.smooth[(size_t) k] = c.primed ? 0.5f * c.smooth[(size_t) k] + 0.5f * p : p;
            const float sm = c.smooth[(size_t) k];
            cur[(size_t) k] = sm;

            const float late = old[(size_t) k] * decay;
            const float gp = 1.0f - alpha * late / (sm + 1.0e-20f);
            const float target = gp <= floorPow ? floorGain : std::sqrt (gp);

            float& g = c.gain[(size_t) k];
            g += (target - g) * (target < g ? 0.6f : 0.3f);
        }

        c.primed = true;
        c.histPos = (c.histPos + 1) % histLen;

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

        fft.run (re.data(), im.data(), true);

        const float scale = 1.0f / ((float) fftSize * 1.5f);
        for (int i = 0; i < fftSize; ++i)
            c.accum[(c.count - (uint32_t) fftSize + (uint32_t) i) & mask] += re[(size_t) i] * fft.window (i) * scale;
    }

    double sampleRate = 44100.0;
    SimpleFFT<fftSize> fft;
    std::array<Chan, 2> chan;
    std::array<float, fftSize> re {}, im {};
    std::array<float, bins> gs {};
};
