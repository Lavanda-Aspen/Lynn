#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>


typedef struct {
	char chunkID[4];          // "RIFF"
    uint32_t chunkSize;      // File size - 8
    char format[4];           // "WAVE"
    char subchunk1ID[4];      // "fmt "
    uint32_t subchunk1Size;   // 16 for PCM
    uint16_t audioFormat;     // 1 = PCM
    uint16_t numChannels;     // 1 = Mono, 2 = Stereo
    uint32_t sampleRate;      // 44100, 48000, etc.
    uint32_t byteRate;        // SampleRate * NumChannels * BitsPerSample/8
    uint16_t blockAlign;      // NumChannels * BitsPerSample/8
    uint16_t bitsPerSample;   // 8, 16, 24, etc.
    char subchunk2ID[4];      // "data"
    uint32_t subchunk2Size;   // Data size
}WavHeader;


class AudioDecoder
{
	std::ifstream file;
	WavHeader header;
	bool is_valid = false;
public:
	AudioDecoder(const std::string file_path) {
		file.open(file_path, std::ios::binary);

		if (!file.is_open()) return;

		file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));
        if (std::string(header.chunkID, 4) != "RIFF" || std::string(header.format, 4) != "WAVE") {
            return;
        }

        is_valid = true;
	}

	bool isValid() const { return is_valid; }
    uint32_t getSampleRate() const { return header.sampleRate; }
    uint16_t getNumChannels() const { return header.numChannels; }
    uint16_t getBitsPerSample() const { return header.bitsPerSample; }

    size_t readChunk(std::vector<int16_t>& out_buffer, size_t max_samples) {
        out_buffer.resize(max_samples);
        file.read(reinterpret_cast<char*>(out_buffer.data()), max_samples * sizeof(int16_t));
        size_t bytes_read = file.gcount();
        size_t samples_read = bytes_read / sizeof(int16_t);
        out_buffer.resize(samples_read);
        return samples_read;
    }
    
    void rewind() {
        file.clear();
        file.seekg(sizeof(WavHeader), std::ios::beg);
    }

	~AudioDecoder() {
        if (file.is_open()) {
            file.close();
        }
    }
	
};