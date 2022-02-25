#pragma once
#define Version "1.0.0"
#include <filesystem>
using namespace std::filesystem;

void init_decoder(const char* input, struct _opts* option);
void loas_decoder(const char* input, struct _opts* option);
void latm_decoder(const char* input, int muxConfigPresent, struct _opts* option, std::ofstream& output);
void StreamMuxConfig(const char* input, struct _latmheader *config);
void AudioSpecificConfig(const char* input, struct _latmheader* config);
int GetAudioObjectType(const char* input);
int GetSamplingFrequency(const char* input);
int GetChannelConfiguration(const char* input);
int getADTSProfileValue(int value);
int getADTSSamplingRateValue(int samplingrate);

struct _T_adtsheader {
	uint8_t b0;
	uint8_t b1;
	uint8_t b2;
	uint8_t b3;
	uint8_t b4;
	uint8_t b5;
	uint8_t b6;
};

struct _latmheader {
	int chanelConfiguration;
	int samplingFrequency;
	int audioObjectType;
};
struct _info {
	path output;
};

struct _opts {
	bool overwrite;
	path output;

};
