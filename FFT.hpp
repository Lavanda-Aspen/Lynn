#pragma once

#include <vector>
#include <complex>
#include <cmath>

constexpr float M_PI = 3.14159265358979323846f;

class FFT {
public:
    using Complex = std::complex<float>;

    static void compute(const std::vector<float>& input, std::vector<float>& output_magnitudes) {
        size_t N = input.size();
        if ((N & (N - 1)) != 0) return;

        std::vector<Complex> a(N);
        for (size_t i = 0; i < N; ++i) {
            // Hann function
            float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (N - 1)));
            a[i] = Complex(input[i] * window, 0.0f);
        }

        // Cooley-Tukey FFT
        for (size_t i = 1, j = 0; i < N; ++i) {
            size_t bit = N >> 1;
            for (; j & bit; bit >>= 1) j &= ~bit;
            j |= bit;
            if (i < j) std::swap(a[i], a[j]);
        }

        for (size_t len = 2; len <= N; len <<= 1) {
            float ang = -2 * M_PI / len;
            Complex wlen(std::cos(ang), std::sin(ang));
            for (size_t i = 0; i < N; i += len) {
                Complex w(1.0f, 0.0f);
                for (size_t j = 0; j < len / 2; ++j) {
                    Complex u = a[i + j], v = a[i + j + len / 2] * w;
                    a[i + j] = u + v;
                    a[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }

        output_magnitudes.resize(N / 2);
        for (size_t i = 0; i < N / 2; ++i) {
            output_magnitudes[i] = std::abs(a[i]);
        }
    }
};