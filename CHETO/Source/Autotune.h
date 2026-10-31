#pragma once
// Autotune simple en C++ puro (sin JUCE): deteccion de tono YIN + cambio de tono
// con dos lineas de retardo cruzadas. Latencia fija = latency muestras.
#include <algorithm>
#include <array>
#include <cmath>

class AutoTuner
{
public:
    static constexpr int window       = 2048;
    static constexpr int latency      = window / 2;
    static constexpr int ringSize     = 8192;
    static constexpr int analysisSize = 2048;
    static constexpr int yinWindow    = 1024;
    static constexpr int hop          = 512;

    struct Settings { int key = 0; int scale = 0; float speed = 0.2f; float amount = 1.0f; };

    void prepare (double sr) { sampleRate = sr; reset(); }

    void reset()
    {
        for (auto& r : ring) r.fill (0.0f);
        mono.fill (0.0f);
        writePos = 0; monoPos = 0; hopCounter = 0;
        phase = 0.0f; currentShift = 0.0f; targetShift = 0.0f;
        unvoicedHops = 99; lastHz = 0.0f;
    }

    float getDetectedHz() const { return lastHz; }

    void process (float* const* data, int numChannels, int numSamples, const Settings& s)
    {
        const int chans = std::min (numChannels, 2);
        if (chans < 1) return;
        settings = s;

        const float tauMs = 1.0f + s.speed * s.speed * 400.0f;
        const float coeff = 1.0f - std::exp (-1.0f / (tauMs * 0.001f * (float) sampleRate));

        for (int i = 0; i < numSamples; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < chans; ++ch)
            {
                ring[(size_t) ch][(size_t) writePos] = data[ch][i];
                sum += data[ch][i];
            }

            mono[(size_t) monoPos] = sum / (float) chans;
            monoPos  = (monoPos + 1) & (analysisSize - 1);
            writePos = (writePos + 1) & (ringSize - 1);

            if (++hopCounter >= hop) { hopCounter = 0; analyse(); }

            currentShift += (targetShift - currentShift) * coeff;

            const float ratio = std::exp2 (currentShift / 12.0f);
            phase += (1.0f - ratio) / (float) window;
            phase -= std::floor (phase);

            float p2 = phase + 0.5f;
            p2 -= std::floor (p2);

            const float d1 = phase * (float) window;
            const float d2 = p2 * (float) window;
            const float s1 = std::sin (3.14159265f * phase);
            const float g1 = s1 * s1;
            const float g2 = 1.0f - g1;

            // si el cambio es minimo usamos solo la toma "seca" (evita efecto peine)
            const float a = std::fabs (currentShift) / 0.15f;
            const float w = a >= 1.0f ? 1.0f : a * a * (3.0f - 2.0f * a);

            for (int ch = 0; ch < chans; ++ch)
            {
                const float dry = readDelay (ch, (float) latency);
                const float wet = g1 * readDelay (ch, d1) + g2 * readDelay (ch, d2);
                data[ch][i] = dry + w * (wet - dry);
            }
        }
    }

private:
    float readDelay (int ch, float d) const
    {
        const int newest = (writePos - 1) & (ringSize - 1);
        const int di = (int) d;
        const float frac = d - (float) di;
        const float a = ring[(size_t) ch][(size_t) ((newest - di) & (ringSize - 1))];
        const float b = ring[(size_t) ch][(size_t) ((newest - di - 1) & (ringSize - 1))];
        return a + frac * (b - a);
    }

    void analyse()
    {
        for (int k = 0; k < analysisSize; ++k)
            buf[(size_t) k] = mono[(size_t) ((monoPos + k) & (analysisSize - 1))];

        float e = 0.0f;
        for (int k = analysisSize - yinWindow; k < analysisSize; ++k)
            e += buf[(size_t) k] * buf[(size_t) k];

        const float rms = std::sqrt (e / (float) yinWindow);
        const float hz = rms > 0.004f ? yin() : 0.0f;
        lastHz = hz;

        if (hz > 0.0f)
        {
            unvoicedHops = 0;
            const float midi = 69.0f + 12.0f * std::log2 (hz / 440.0f);
            targetShift = std::max (-6.0f, std::min (6.0f, (nearestNote (midi) - midi) * settings.amount));
        }
        else if (++unvoicedHops > 4)
        {
            unvoicedHops = 99;
            targetShift = 0.0f;
        }
    }

    float nearestNote (float midi) const
    {
        static const bool masks[3][12] = {
            { true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true  },  // cromatica
            { true,  false, true,  false, true,  true,  false, true,  false, true,  false, true  },  // mayor
            { true,  false, true,  true,  false, true,  false, true,  true,  false, true,  false }   // menor
        };

        const int sc = std::max (0, std::min (2, settings.scale));
        const int c = (int) std::lround (midi);
        float best = (float) c, bestDist = 1.0e9f;

        for (int n = c - 7; n <= c + 7; ++n)
        {
            const int pc = (((n - settings.key) % 12) + 12) % 12;
            if (! masks[sc][pc]) continue;
            const float dist = std::fabs ((float) n - midi);
            if (dist < bestDist) { bestDist = dist; best = (float) n; }
        }

        return best;
    }

    float yin()
    {
        const int tauMax = std::min ((int) (sampleRate / 70.0), analysisSize - yinWindow - 1);
        const int tauMin = std::max (2, (int) (sampleRate / 1000.0));
        const int start  = analysisSize - yinWindow - tauMax;

        for (int tau = 1; tau <= tauMax; ++tau)
        {
            float sum = 0.0f;
            for (int j = 0; j < yinWindow; j += 2)
            {
                const float d = buf[(size_t) (start + j)] - buf[(size_t) (start + j + tau)];
                sum += d * d;
            }
            diff[(size_t) tau] = sum;
        }

        float running = 0.0f;
        cmnd[0] = 1.0f;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            running += diff[(size_t) tau];
            cmnd[(size_t) tau] = running > 0.0f ? diff[(size_t) tau] * (float) tau / running : 1.0f;
        }

        int best = -1;
        for (int tau = tauMin; tau < tauMax; ++tau)
        {
            if (cmnd[(size_t) tau] < 0.15f)
            {
                while (tau + 1 < tauMax && cmnd[(size_t) (tau + 1)] < cmnd[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        }

        if (best < 0)
        {
            float m = 1.0e9f;
            for (int tau = tauMin; tau < tauMax; ++tau)
                if (cmnd[(size_t) tau] < m) { m = cmnd[(size_t) tau]; best = tau; }

            if (best < 0 || m > 0.3f) return 0.0f;
        }

        float t = (float) best;
        if (best > 1 && best < tauMax)
        {
            const float s0 = cmnd[(size_t) (best - 1)], s1 = cmnd[(size_t) best], s2 = cmnd[(size_t) (best + 1)];
            const float den = s0 + s2 - 2.0f * s1;
            if (std::fabs (den) > 1.0e-9f) t = (float) best + 0.5f * (s0 - s2) / den;
        }

        return (float) sampleRate / t;
    }

    double sampleRate = 44100.0;
    Settings settings;
    std::array<std::array<float, ringSize>, 2> ring {};
    std::array<float, analysisSize> mono {}, buf {};
    std::array<float, analysisSize + 2> diff {}, cmnd {};
    int writePos = 0, monoPos = 0, hopCounter = 0, unvoicedHops = 99;
    float phase = 0.0f, currentShift = 0.0f, targetShift = 0.0f, lastHz = 0.0f;
};
