#define SDL_MAIN_HANDLED

#include "ParticleOfLife.hpp"
#include "FFT.hpp"
#include "AudioDecoder.hpp"
#include "RingBuffer.hpp"

#include <SDL2/SDL.h>
#include <thread>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <string>
#include <chrono>

#define NUM_TYPES 4
#define TOTAL_BAR (NUM_TYPES * NUM_TYPES)
#define FFT_WINDOW 512
#define WIDTH 1400
#define HEIGHT 900

#define MAX_PARTICLES 700
#define R_MAX 110.0f
#define BETA 0.35f
#define FORCE_SCALE 600.0f
#define FRICTION 0.85f
#define MIN_DIST 4.0f


void Stereo_Left_FFT(std::vector<float>& leftInput, std::vector<float>& leftFFT, std::vector<float>& rightInput, std::vector<float>& rightFFT, std::vector<float>& smoothAudio);
void Stereo_Right_FFT(std::vector<float>& leftInput, std::vector<float>& leftFFT, std::vector<float>& rightInput, std::vector<float>& rightFFT, std::vector<float>& smoothAudio);
void Stereo_FFT(std::vector<float>& leftInput, std::vector<float>& leftFFT, std::vector<float>& rightInput, std::vector<float>& rightFFT, std::vector<float>& smoothAudio);
void Stereo_Split_FFT(std::vector<float>& leftInput, std::vector<float>& leftFFT, std::vector<float>& rightInput, std::vector<float>& rightFFT, std::vector<float>& smoothAudio);
void Stereo_Alternating_FFT(std::vector<float>& leftInput, std::vector<float>& leftFFT, std::vector<float>& rightInput, std::vector<float>& rightFFT, std::vector<float>& smoothAudio);


const size_t BUFFER_SIZE = 65536; // 2^16
RingBuffer<int16_t, BUFFER_SIZE> audioRingBuffer;
RingBuffer<int16_t, BUFFER_SIZE> fftRingBuffer;
std::atomic<bool> isRunning(true);

void decodeThreadFunc(const std::string& filepath) {
    AudioDecoder decoder(filepath);
    if (!decoder.isValid()) return;

    std::vector<int16_t> chunk;
    while (isRunning) {
        if (audioRingBuffer.size() < BUFFER_SIZE / 2) {
            size_t read = decoder.readChunk(chunk, 1024);
            if (read == 0) {
                decoder.rewind();
                continue;
            }
            audioRingBuffer.push(chunk.data(), read);
            fftRingBuffer.push(chunk.data(), read);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void audioCallback(void* userdata, Uint8* stream, int len) {
    int16_t* output = reinterpret_cast<int16_t*>(stream);
    size_t samplesRequested = len / sizeof(int16_t);
    size_t read = audioRingBuffer.pop(output, samplesRequested);
    
    if (read < samplesRequested) {
        std::memset(output + read, 0, (samplesRequested - read) * sizeof(int16_t));
    }
}

int main(int argc, char* argv[]) {
	std::string Wav_Path = "";

	if (argc > 1) {
        Wav_Path = argv[1];
    } else {
        std::cout << "file path: ";
        std::getline(std::cin, Wav_Path);
        if (Wav_Path.empty()) {
            std::cerr << "Invalid Path" << std::endl;
        }
    }

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    	std::cerr << "Unable to init SDL" << std::endl;
    	return -1;
    }

    AudioDecoder decoder(Wav_Path.c_str());
    bool file_loaded = decoder.isValid();
    if (!file_loaded) {
    	std::cerr << "Can't open file or file does not exist" << std::endl;
    	return -1;
    }

    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = file_loaded ? decoder.getSampleRate() : 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = file_loaded ? decoder.getNumChannels() : 2;
    desired.samples = 1024;
    desired.callback = audioCallback;

    SDL_AudioDeviceID devId = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (devId == 0) return -1;

    std::thread decodeThread(decodeThreadFunc, Wav_Path);
    SDL_PauseAudioDevice(devId, 0);

    POL pol(MAX_PARTICLES, NUM_TYPES, R_MAX, BETA, FORCE_SCALE, FRICTION, MIN_DIST, WIDTH, HEIGHT);
    pol.start();

    std::vector<float> leftInput(FFT_WINDOW);
    std::vector<float> rightInput(FFT_WINDOW);
    std::vector<float> leftFFT, rightFFT;

    std::vector<float> smoothAudio(TOTAL_BAR, 0.0f);

    while (pol.Active()) {
    	Stereo_FFT(leftInput, leftFFT, rightInput, rightFFT, smoothAudio);
        pol.Drift_Angle(smoothAudio);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    pol.stop();
    isRunning = false;
    if (decodeThread.joinable()) decodeThread.join();
    SDL_CloseAudioDevice(devId);
    SDL_Quit();
    return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void inline Stereo_Left_FFT(std::vector<float>& leftInput, std::vector<float>& leftFFT, std::vector<float>& smoothAudio) {
    std::vector<int16_t> tempBuffer(FFT_WINDOW * 2);
    size_t peeked = fftRingBuffer.pop(tempBuffer.data(), FFT_WINDOW * 2);

    for (size_t i = 0; i < FFT_WINDOW; ++i) {
        if ((i * 2 + 1) < peeked) {
            leftInput[i]  = static_cast<float>(tempBuffer[i * 2]);
        } else {
            leftInput[i]  = 0.0f;
        }
    }

    FFT::compute(leftInput, leftFFT);

    if (!leftFFT.empty()) {
        int binSpan = std::max(1, static_cast<int>(leftFFT.size()) / TOTAL_BAR);
        for (int b = 0; b < TOTAL_BAR; ++b) {
            float sum = 0.0f;
            int count = 0;
            int start = b * binSpan;
            int end = start + binSpan;
            for (int k = start; k < end && k < static_cast<int>(leftFFT.size()); ++k) {
                sum += leftFFT[k];
                count++;
            }
            float avgVal = (count > 0) ? (sum / count) : 0.0f;
            
            float target = std::log10(avgVal + 1.0f) * 0.1f;
            smoothAudio[b] += (target - smoothAudio[b]) * 0.25f;
        }
    }
}

void inline Stereo_Right_FFT(std::vector<float>& rightInput, std::vector<float>& rightFFT, std::vector<float>& smoothAudio) {
    std::vector<int16_t> tempBuffer(FFT_WINDOW * 2);
    size_t peeked = fftRingBuffer.pop(tempBuffer.data(), FFT_WINDOW * 2);

    for (size_t i = 0; i < FFT_WINDOW; ++i) {
        if ((i * 2 + 1) < peeked) {
            rightInput[i]  = static_cast<float>(tempBuffer[i * 2 + 1]);
        } else {
            rightInput[i]  = 0.0f;
        }
    }

    FFT::compute(rightInput, rightFFT);

    if (!rightFFT.empty()) {
        int binSpan = std::max(1, static_cast<int>(rightFFT.size()) / TOTAL_BAR);
        for (int b = 0; b < TOTAL_BAR; ++b) {
            float sum = 0.0f;
            int count = 0;
            int start = b * binSpan;
            int end = start + binSpan;
            for (int k = start; k < end && k < static_cast<int>(rightFFT.size()); ++k) {
                sum += rightFFT[k];
                count++;
            }
            float avgVal = (count > 0) ? (sum / count) : 0.0f;
            
            float target = std::log10(avgVal + 1.0f) * 0.1f;
            smoothAudio[b] += (target - smoothAudio[b]) * 0.25f;
        }
    }
        
}

void inline Stereo_FFT(std::vector<float>& leftInput, std::vector<float>& leftFFT, std::vector<float>& rightInput, std::vector<float>& rightFFT, std::vector<float>& smoothAudio) {
    std::vector<int16_t> tempBuffer(FFT_WINDOW * 2);
    size_t peeked = fftRingBuffer.pop(tempBuffer.data(), FFT_WINDOW * 2);
    for (size_t i = 0; i < FFT_WINDOW; ++i) {
        if ((i * 2 + 1) < peeked) {
            leftInput[i]  = static_cast<float>(tempBuffer[i * 2]);
            rightInput[i] = static_cast<float>(tempBuffer[i * 2 + 1]);
        } else {
            leftInput[i]  = 0.0f;
            rightInput[i] = 0.0f;
        }
    }

    FFT::compute(leftInput, leftFFT);
    FFT::compute(rightInput, rightFFT);

    if (!leftFFT.empty() && !rightFFT.empty()) {
        int binSpan = std::max(1, static_cast<int>(leftFFT.size()) / TOTAL_BAR);
        for (int b = 0; b < TOTAL_BAR; ++b) {
            float sum = 0.0f;
            int count = 0;
            int start = b * binSpan;
            int end = start + binSpan;
            for (int k = start; k < end && k < static_cast<int>(leftFFT.size()); ++k) {
                sum += leftFFT[k] + rightFFT[k];
                count++;
            }
            float avgVal = (count > 0) ? (sum / count) : 0.0f;
            
            float target = std::log10(avgVal + 1.0f) * 0.08f ;
            smoothAudio[b] += (target - smoothAudio[b]) * 0.225f;
        }
    }
}

void inline Stereo_Split_FFT(std::vector<float>& leftInput, std::vector<float>& leftFFT, std::vector<float>& rightInput, std::vector<float>& rightFFT, std::vector<float>& smoothAudio) {
    std::vector<int16_t> tempBuffer(FFT_WINDOW * 2);
    size_t peeked = fftRingBuffer.pop(tempBuffer.data(), FFT_WINDOW * 2);
    for (size_t i = 0; i < FFT_WINDOW; ++i) {
        if ((i * 2 + 1) < peeked) {
            leftInput[i]  = static_cast<float>(tempBuffer[i * 2]);
            rightInput[i] = static_cast<float>(tempBuffer[i * 2 + 1]);
        } else {
            leftInput[i]  = 0.0f;
            rightInput[i] = 0.0f;
        }
    }

    FFT::compute(leftInput, leftFFT);
    FFT::compute(rightInput, rightFFT);

    if (!leftFFT.empty() && !rightFFT.empty()) {
        int halfBar = TOTAL_BAR / 2;
        int binSpan = std::max(1, static_cast<int>(leftFFT.size()) / TOTAL_BAR);
        for (int b = 0; b < halfBar; ++b) {
            float sumleft = 0.0f;
            float sumright = 0.0f;
            int count = 0;
            int start = b * binSpan;
            int end = start + binSpan;
            for (int k = start; k < end && k < static_cast<int>(leftFFT.size()); ++k) {
                sumleft += leftFFT[k];
                sumright += rightFFT[k];
                count++;
            }
            float avgVal_left = (count > 0) ? (sumleft / count) : 0.0f;
            float avgVal_right = (count > 0) ? (sumright / count) : 0.0f;
            
            float target_left = std::log10(avgVal_left + 1.0f) * 0.09f ;
            float target_right = std::log10(avgVal_right + 1.0f) * 0.09f ;
            smoothAudio[b] += (target_left - smoothAudio[b]) * 0.275f;
            smoothAudio[b + halfBar] += (target_right - smoothAudio[b + halfBar]) * 0.275f;
        }
    }
}

void inline Stereo_Alternating_FFT(std::vector<float>& leftInput, std::vector<float>& leftFFT, std::vector<float>& rightInput, std::vector<float>& rightFFT, std::vector<float>& smoothAudio) {
    std::vector<int16_t> tempBuffer(FFT_WINDOW * 2);
    size_t peeked = fftRingBuffer.pop(tempBuffer.data(), FFT_WINDOW * 2);
    for (size_t i = 0; i < FFT_WINDOW; ++i) {
        if ((i * 2 + 1) < peeked) {
            leftInput[i]  = static_cast<float>(tempBuffer[i * 2]);
            rightInput[i] = static_cast<float>(tempBuffer[i * 2 + 1]);
        } else {
            leftInput[i]  = 0.0f;
            rightInput[i] = 0.0f;
        }
    }

    FFT::compute(leftInput, leftFFT);
    FFT::compute(rightInput, rightFFT);

    if (!leftFFT.empty() && !rightFFT.empty()) {
        int halfBar = TOTAL_BAR / 2;
        int binSpan = std::max(1, static_cast<int>(leftFFT.size()) / TOTAL_BAR);
        for (int b = 0; b < halfBar; ++b) {
            float sumleft = 0.0f;
            float sumright = 0.0f;
            int count = 0;
            int start = b * binSpan;
            int end = start + binSpan;
            for (int k = start; k < end && k < static_cast<int>(leftFFT.size()); ++k) {
                sumleft += leftFFT[k];
                sumright += rightFFT[k];
                count++;
            }
            float avgVal_left = (count > 0) ? (sumleft / count) : 0.0f;
            float avgVal_right = (count > 0) ? (sumright / count) : 0.0f;
            
            float target_left = std::log10(avgVal_left + 1.0f) * 0.1f ;
            float target_right = std::log10(avgVal_right + 1.0f) * 0.1f ;
            smoothAudio[2 * b] += (target_left - smoothAudio[2 * b]) * 0.25f;
            smoothAudio[2 * b + 1] += (target_right - smoothAudio[2 * b + 1]) * 0.25f;
        }
    }
}