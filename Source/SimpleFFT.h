#pragma once
// FFT radix-2 chiquita en C++ puro (sin JUCE), con ventana Hann incluida.
#include <array>
#include <cmath>
#include <utility>

template <int N>
class SimpleFFT
{
public:
    SimpleFFT()
    {
        const double twoPi = 6.283185307179586;

        for (int k = 0; k < N / 2; ++k)
        {
            cosT[(size_t) k] = (float) std::cos (twoPi * k / N);
            sinT[(size_t) k] = (float) std::sin (twoPi * k / N);
        }

        int bitsCount = 0;
        while ((1 << bitsCount) < N) ++bitsCount;

        for (int i = 0; i < N; ++i)
        {
            int r = 0;
            for (int b = 0; b < bitsCount; ++b)
                if (i & (1 << b)) r |= 1 << (bitsCount - 1 - b);
            rev[(size_t) i] = r;
            win[(size_t) i] = (float) (0.5 - 0.5 * std::cos (twoPi * i / N));
        }
    }

    float window (int i) const { return win[(size_t) i]; }

    // inverse = true hace la transformada inversa SIN dividir por N
    void run (float* re, float* im, bool inverse) const
    {
        for (int i = 0; i < N; ++i)
        {
            const int j = rev[(size_t) i];
            if (j > i) { std::swap (re[i], re[j]); std::swap (im[i], im[j]); }
        }

        for (int len = 2; len <= N; len <<= 1)
        {
            const int half = len >> 1, step = N / len;

            for (int i = 0; i < N; i += len)
                for (int j = 0; j < half; ++j)
                {
                    const float wr = cosT[(size_t) (j * step)];
                    const float wi = inverse ? sinT[(size_t) (j * step)] : -sinT[(size_t) (j * step)];
                    const int a = i + j, b = a + half;
                    const float tr = re[b] * wr - im[b] * wi;
                    const float ti = re[b] * wi + im[b] * wr;
                    re[b] = re[a] - tr; im[b] = im[a] - ti;
                    re[a] += tr;        im[a] += ti;
                }
        }
    }

private:
    std::array<float, N / 2> cosT {}, sinT {};
    std::array<int, N> rev {};
    std::array<float, N> win {};
};
