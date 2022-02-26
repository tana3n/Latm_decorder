#include <iostream>
#include <fstream>
#include <bitset>
#include <string>
#include <filesystem>

#include <chrono>
#include "Latm_decoder.h"

using namespace std::filesystem;



void usage() {
    std::cout << "Usage: latm_decoder [options] <input> \n";
    std::cout << "--overwrite\tIf it exists in the output destination, it will be overwritten.\n";
    std::cout << "--benchmark\tShow elapsed time.\n";

    return;
}
void cli_parser(char* argopts[], int optsum) {

    std::ifstream ifs(argopts[optsum - 1]);
    if (!ifs.is_open()) {
        std::cout << "[Error]Failed read LATM/LAOS File\n\n";
        return;
    }
    struct _opts option;
    option.benchmark = false;

    for (int i = 0; i < optsum; i++) {
        if (!_stricmp(argopts[i], "--overwrite")) {
            option.overwrite = true;
            continue;
        }
        if (!_stricmp(argopts[i], "--benchmark")) {
            option.benchmark = true;
            continue;
        }
    }
    init_decoder(argopts[optsum - 1], &option);

    return;
}

int main(int argc, char* argv[]){
    std::cout << "latm-decoder Version " << Version << "\n\n";


    if (!argv[1]) {
        usage();

        return 0;
    }
    cli_parser(argv, argc);

}
void init_decoder(const char* input, struct _opts* option) {
    std::cout << "InputFile: " << input << "\n";
    std::ofstream output_adts;
    path p = input;
    option->output = p.replace_extension("aac");
    if (exists(option->output) & !(option->overwrite)) {
        std::cout << "[Error]" << option->output << "is exist. (use overwrite options: --overwrite)" << std::endl;

        return;
    }
    auto start = std::chrono::steady_clock::now();

    loas_decoder(input, option);
    if (option->benchmark) {
        auto end = std::chrono::steady_clock::now();
        std::cout << "\nelapsed time: " << duration_cast<std::chrono::seconds>(end - start)  << std::endl;
    }

}

void loas_decoder(const char* input, struct _opts* option) {
    std::uintmax_t size = file_size(input);
    //std::cout << "Filesize: " << size << " bytes\n";

    std::ifstream import_latm(input, std::ios::in | std::ios::binary);
    if (!import_latm) {
        std::cout << "Can not open source: " << input;
        return;
    }

    std::ofstream output_adts;
    path filename = option->output;
    output_adts.open(filename, std::ios::out | std::ios::binary);
    char* hBuf = new char[4096];

    import_latm.seekg(0);
    
    while (import_latm.tellg() < size) {
        int t = import_latm.tellg();
        import_latm.read(hBuf, 3);
        if (hBuf[0] != 0x56 || (hBuf[1] & 0xE0) != 0xe0) {
            import_latm.seekg(t+1);
            continue;
        }
        std::uintmax_t length = ((((((unsigned char*)hBuf)[1] & 0x1F) << 8) | ((unsigned char*)hBuf)[2]) + 3);
        import_latm.read(hBuf, length);

        latm_decoder(hBuf, 1, option, output_adts);
        //std::cout << "[" << std::setfill('0') << std::left << std::setw(4) << std::floor(double(t + length) / (double)size * 10000) / 100
        //    << "%]\r";//Output " << double((size_t)i + length)/1024/1024 << "Mbytes" ;
        import_latm.seekg(t+length);
        

    }
    delete[] hBuf;
 }

void latm_decoder(const char* input, int muxConfigPresent, struct _opts* option, std::ofstream& output) {
     // AudioMuxElement()
     struct _latmheader latmconf;
     const char* latm = input;
     if (muxConfigPresent) {
        int useSameStreamMux =( (input[0] >> 7)&1);
        if (!useSameStreamMux)
        {
           StreamMuxConfig(input,&latmconf);
        }
    }
     //PayloadLengthInfo()
     int allStreamsSameTimeFraming = ((input[0] >> 5) & 1);
     if (allStreamsSameTimeFraming) {
         std::uintmax_t tmpt = 6;
         std::uintmax_t tmp = 0;
         std::uintmax_t latmBufferFullness = ((input[4] << 3) | ((input[5] >> 5) & 0x7));
         uintmax_t MuxSlotLengthBytes =0;
         do {
             std::uintmax_t tmp2 = (input[tmpt] << 5) & 0xFF;//明示的に切り捨て
             std::uintmax_t tmp3 = ((input[tmpt + 1] >> 3) & 0x1F);
             tmp = (tmp2 | tmp3);
             MuxSlotLengthBytes += tmp;
             tmpt += 1;
         } while (tmp == 255);


         //Write ADTSHeader
         int sync = (0xFF << 4) & 0xFF;
         int ID = 0 << 3 ;
         int protection_bit = 1;
         int profile = ( getADTSProfileValue(latmconf.audioObjectType) << 6 ) & 0xFF;
         std::uintmax_t SampRate = (getADTSSamplingRateValue(48000) <<2) &0x3F;
         //PrivateBit is disabled:
         int b1_CC = (latmconf.chanelConfiguration >> 2) & 0x1;
         
         struct _T_adtsheader adtsheader;

         adtsheader.b0 = 0xFF;//Sync
         adtsheader.b1 = (sync | ID | protection_bit);
         adtsheader.b2 = (profile| SampRate | b1_CC);
         int framesize = MuxSlotLengthBytes + sizeof(adtsheader);
         adtsheader.b3 = ( ((latmconf.chanelConfiguration << 6) & 0xFF) |((framesize >> 11) & 0x3) );
         adtsheader.b4 = (framesize >> 3) & 0xFF;
         adtsheader.b5 = (framesize << 5) | 0x1F;
         adtsheader.b6 = 0xFC;
         output.write((char*)&adtsheader,sizeof(adtsheader));


         char* out = new char[MuxSlotLengthBytes];
         for (int tmpt2 = tmpt; tmpt2 < MuxSlotLengthBytes+tmpt; tmpt2 =1+ tmpt2) {
             out[tmpt2-tmpt] = ((input[tmpt2] << 5) | ((input[tmpt2 + 1] >> 3) & 0x1F) );
         }
         output.write(out, MuxSlotLengthBytes);
         delete[] out;

     }
}

void StreamMuxConfig(const char* input, struct _latmheader* config){
    int audioMuxVersion = ( (input[0] >> 6 )& 1);
    int audioMuxVersionA = 0;
    int taraBufferFullness;
    //std::cout << "audioMuxVersion= " << audioMuxVersion << std::endl;

    if (audioMuxVersion) {
        audioMuxVersionA = ( (input[0]  >> 5) & 1);
        std::cout << "audioMuxVersion= " << audioMuxVersion << std::endl;

    }
    //std::cout << "Set audioMuxVersionA= " << audioMuxVersionA << std::endl;

    //bitseek(const char* input, bits)
    //16超えたら適当に割って次のバイトを参照するアレをつくる

    if (!audioMuxVersionA) {
        if (audioMuxVersion) {
            taraBufferFullness = 0;//LatmGetValue();
        }
        int streamCnt = 0;
        int allStreamsSameTimeFraming = ((input[0] >> 5) & 1);
        //std::cout << "allStreamsSameTimeFraming= " << allStreamsSameTimeFraming << std::endl;
        if (allStreamsSameTimeFraming != 1) {
            std::cout << "[Warm] Latm_decorder is supported allStreamsSameTimeFraming == 1 only" << std::endl;
            return;
        }

        std::uintmax_t numSubFrames = ( (input[0] & 0xF) <<1 ) | ((input[1] >> 7) & 0x1);
        //std::cout << "numSubFrames= " << numSubFrames << std::endl;
        std::uintmax_t numProgram = ((input[1] >> 3) & 0xf);// | (input[2]) + 3);
        //std::cout << "numProgram= " << numProgram << std::endl;
        int prog;
        for (prog = 0; prog <= numProgram; prog++) {
            std::uintmax_t numLayer = (input[0] & 0xF) & ((input[1] >> 3) & 0x7);// | (input[2]) + 3);
            int lay;
            int progSindx[1] = {};
            int laySIndx[1] = {};
            int streamID[1][1] = {};
            //std::cout << "numlayer= " << numLayer << std::endl;

            for (lay = 0; lay <= numLayer; lay++) {
                //progSindx[0] =  prog;
                //laySIndx[0] = lay;
                //streamID[prog][ray] = streamCnt++;
                int useSameConfig = 0;
                if (prog == 0 && lay == 0) {
                }else{
                    useSameConfig = (input[2] >> 7);
                }
                if (!useSameConfig) {
                    if (audioMuxVersion == 0) {
                        AudioSpecificConfig(input,config);
                    } else {
                        std::cout << "audioMuxVersion is not 0 " << std::endl;

                    }
                }
                std::uintmax_t frameLengthType = ( (input[4] >> 5 )& 0x7);

                //std::cout << "frameLengthType= " << frameLengthType << std::endl;
                //ともかく0とする
                std::uintmax_t latmBufferFullness = ( (input[4]<<3) | ((input[5] >> 5) & 0x7) );


                if (!allStreamsSameTimeFraming) {
                    return;
                }
                std::uintmax_t otherDataPresent = (input[5] >> 4) & 0x1;
                //std::cout << "otherDataPresent= " <<otherDataPresent << std::endl;
                std::uintmax_t CRC_flags = (input[5] >> 3) & 0x1;
                if (CRC_flags) {
                    std::uintmax_t CRC_CheckSum =  (input[5] << 3) & 0xFF;//明示的に切り捨て
                    CRC_CheckSum = CRC_CheckSum | ((input[6] >> 5) & 0x7);
                    //std::cout << "CRC_CheckSum= " << std::bitset<8>(CRC_CheckSum)   << std::endl;
                }
                else {
                    std::cout << "CRC_flags= " << CRC_flags << std::endl;

                }
            }
        }

    }

    }
void AudioSpecificConfig(const char* input, struct _latmheader* config) {
    config->audioObjectType = GetAudioObjectType(input);
    config->samplingFrequency = GetSamplingFrequency(input);
    config->chanelConfiguration = GetChannelConfiguration(input);

    //if 0xfは別処理
    //switch (audioObjectType)　GASpecificConfigを
    std::uintmax_t frameLengthFlags = ((input[3] >> 2 )& 1);//1024
    std::uintmax_t dependsOnCoreCoder = ((input[3] >> 1) & 1);//1024
    std::uintmax_t extensionFlag = ((input[3]) & 1);//1024
    return;// latmconf;
}
int GetAudioObjectType(const char* input) {
    //int audioObjectType = ((input[2] >> 3) & 0x1F);// | (input[2]) + 3);
    std::uintmax_t audioObjectType = ((input[2] >> 3) & 0x1F);// | (input[2]) + 3);
    return audioObjectType ;//2→AAC-LC
}

int GetSamplingFrequency(const char* input) {

    std::uintmax_t SamplingFrequencyIndex = (input[2]& 0x7) << 1 |( (input[3] >> 7) & 0x1);// | (input[2]) + 3);

    switch (SamplingFrequencyIndex) {
    case 3:
        return 48000;
    
    }
    return 0;
}
int GetChannelConfiguration(const char* input) {
    //int channelConfigurationIndex = ((input[3] >> 3) & 0xF);// | (input[2]) + 3);
    std::uintmax_t channelConfigurationIndex = ((input[3] >> 3) & 0xF);// | (input[2]) + 3);

    return channelConfigurationIndex;
}

// ADTSHeader 
int getADTSSamplingRateValue(int samplingrate) {
    if (samplingrate == 48000) {
        return 3;
    }
    else if (samplingrate == 96000) {
        return 0;
    }
    return NULL;

}
int getADTSProfileValue(int value) {
    if (value == 2) {
        return 1;
    }
    std::cout << "[Warn] Latm_decorder is supported AAC-LC only" << std::endl;

    return NULL;
}