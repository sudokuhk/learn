/*************************************************************************
    > File Name: bit.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Mon Jan  7 16:13:05 2019
 ************************************************************************/

#include <stdio.h>

#define MKBETAG(a,b,c,d) ((d) | ((c) << 8) | ((b) << 16) | ((unsigned)(a) << 24))

enum AudioObjectType {
    AOT_NULL,
    // Support?                Name
    AOT_AAC_MAIN,              ///< Y                       Main
    AOT_AAC_LC,                ///< Y                       Low Complexity
    AOT_AAC_SSR,               ///< N (code in SoC repo)    Scalable Sample Rate
    AOT_AAC_LTP,               ///< Y                       Long Term Prediction
    AOT_SBR,                   ///< Y                       Spectral Band Replication
    AOT_AAC_SCALABLE,          ///< N                       Scalable
    AOT_TWINVQ,                ///< N                       Twin Vector Quantizer
    AOT_CELP,                  ///< N                       Code Excited Linear Prediction
    AOT_HVXC,                  ///< N                       Harmonic Vector eXcitation Coding
    AOT_TTSI             = 12, ///< N                       Text-To-Speech Interface
    AOT_MAINSYNTH,             ///< N                       Main Synthesis
    AOT_WAVESYNTH,             ///< N                       Wavetable Synthesis
    AOT_MIDI,                  ///< N                       General MIDI
    AOT_SAFX,                  ///< N                       Algorithmic Synthesis and Audio Effects
    AOT_ER_AAC_LC,             ///< N                       Error Resilient Low Complexity
    AOT_ER_AAC_LTP       = 19, ///< N                       Error Resilient Long Term Prediction
    AOT_ER_AAC_SCALABLE,       ///< N                       Error Resilient Scalable
    AOT_ER_TWINVQ,             ///< N                       Error Resilient Twin Vector Quantizer
    AOT_ER_BSAC,               ///< N                       Error Resilient Bit-Sliced Arithmetic Coding
    AOT_ER_AAC_LD,             ///< N                       Error Resilient Low Delay
    AOT_ER_CELP,               ///< N                       Error Resilient Code Excited Linear Prediction
    AOT_ER_HVXC,               ///< N                       Error Resilient Harmonic Vector eXcitation Coding
    AOT_ER_HILN,               ///< N                       Error Resilient Harmonic and Individual Lines plus Noise
    AOT_ER_PARAM,              ///< N                       Error Resilient Parametric
    AOT_SSC,                   ///< N                       SinuSoidal Coding
    AOT_PS,                    ///< N                       Parametric Stereo
    AOT_SURROUND,              ///< N                       MPEG Surround
    AOT_ESCAPE,                ///< Y                       Escape Value

    AOT_L1,                    ///< Y                       Layer 1
    AOT_L2,                    ///< Y                       Layer 2
    AOT_L3,                    ///< Y                       Layer 3
    AOT_DST,                   ///< N                       Direct Stream Transfer
    AOT_ALS,                   ///< Y                       Audio LosslesS
    AOT_SLS,                   ///< N                       Scalable LosslesS
    AOT_SLS_NON_CORE,          ///< N                       Scalable LosslesS (non core)
    AOT_ER_AAC_ELD,            ///< N                       Error Resilient Enhanced Low Delay
    AOT_SMR_SIMPLE,            ///< N                       Symbolic Music Representation Simple
    AOT_SMR_MAIN,              ///< N                       Symbolic Music Representation Main
    AOT_USAC_NOSBR,            ///< N                       Unified Speech and Audio Coding (no SBR)
    AOT_SAOC,                  ///< N                       Spatial Audio Object Coding
    AOT_LD_SURROUND,           ///< N                       Low Delay MPEG Surround
    AOT_USAC,                  ///< N                       Unified Speech and Audio Coding
};

const int avpriv_mpeg4audio_sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
};

const unsigned int ff_mpeg4audio_channels[8] = {
    0, 1, 2, 3, 4, 5, 6, 8
};

class CBitStream {
public:
    CBitStream(const unsigned char * p, int nbits)
        : _p(p), _n(nbits), _off(0) {
    }
    
    unsigned int Show(int nbits) {
        unsigned int value = 0;
        
        int byte_offset = _off / 8;
        int bit_in_byte = _off % 8;
        
        for (int i = 0; i < nbits; i++) {
            value <<= 1;
            value |= (_p[byte_offset] >> (7 - bit_in_byte)) & 0x01;
            
            bit_in_byte ++;
            if (bit_in_byte == 8) {
                byte_offset ++;
                bit_in_byte = 0;
            }
        }
        
        return value;
    }
    
    unsigned int Get(int nbits) {
        unsigned value = Show(nbits);
        _off += nbits;
        
        return value;
    }
    
    void Skip(int nbits) {
        _off += nbits;
    }
    
    int Remain(void) const {
        return _n - _off;
    }
    
    int Consume(void) const {
        return _off;
    }
    
private:
    const unsigned char  * _p;
    const int _n;
    int _off;
};

int get_object_type(CBitStream & bitstream)
{
    int object_type = bitstream.Get(5);
    if (object_type == AOT_ESCAPE) {
        object_type = 32 + bitstream.Get(6);
    }
    return object_type;
}

int get_sample_rate(CBitStream & bitstream)
{
    int index = bitstream.Get(4);
    if (index == 0x0F) {
        return bitstream.Get(24);
    }
    return avpriv_mpeg4audio_sample_rates[index];
}

int parse_config_ALS(CBitStream & bitstream)
{
    if (bitstream.Remain() < 112) {
        return -1;
    }
    
    if (bitstream.Get(32) != MKBETAG('A', 'L', 'S', '\0')) {
        return -1;
    }
    
    int sample_rate = bitstream.Get(32);
    bitstream.Skip(32);
    int chan_config = 0;
    int channels = bitstream.Get(16) + 1;

    printf("samplerate:%d, channels:%d\n", sample_rate, channels);
    return 0;
}

int main(int argc, char * argv[])
{
    unsigned char buf[5] = {0x13, 0x90, 0x56, 0xe5, 0xa0, };
    CBitStream bitstream(buf, 5 * 8);
    
    int object_type = get_object_type(bitstream);
    int sample_rate = get_sample_rate(bitstream);
    int chan_config = bitstream.Get(4);
    int ext_object_type = AOT_NULL;
    int ext_sample_rate = 0;
    
    printf("remain:%d\n", bitstream.Remain());
    printf("object_type:%d\n", object_type);
    printf("sample_rate:%d\n", sample_rate);
    printf("chan_config:%d\n", chan_config);
    
    if (object_type == AOT_SBR || 
        (object_type == AOT_PS && 
         !(bitstream.Show(3) & 0x03) && !(bitstream.Show(9) & 0x3F))) {
        printf("ext sample rate\n");
    } else {
        printf("AOT_NULL\n");
    }
    
    int specific_config_bitindex = bitstream.Consume();
    if (object_type == AOT_ALS) {
        bitstream.Skip(5);
        
        if (bitstream.Show(24) != MKBETAG('\0', 'A', 'L', 'S')) {
            bitstream.Skip(24);
        }
        specific_config_bitindex = bitstream.Consume();
        
        parse_config_ALS(bitstream);
    }
    
    if (ext_object_type != AOT_SBR && 1 /*sync_extension*/) {
        while (bitstream.Remain() > 15) {
            if (bitstream.Show(11) == 0x2b7) {
                printf("0x2b7\n");
                bitstream.Skip(11);
                ext_object_type = get_object_type(bitstream);
                int sbr = bitstream.Get(1);
                printf("SBR:%d, ext_object_type:%d\n", sbr, ext_object_type);
                if (ext_object_type == AOT_SBR && sbr) {
                    ext_sample_rate = get_sample_rate(bitstream);
                    printf("ext_sample_rate:%d\n", ext_sample_rate);
                    if (ext_sample_rate == sample_rate) {
                        sbr = -1;
                    }
                }
                
                if (bitstream.Remain() > 11 && bitstream.Get(11) == 0x548) {
                    int ps = bitstream.Get(1);
                    printf("ps:%d\n", ps);
                }
                break;
            } else {
                bitstream.Skip(1);
            }
        }
    }
    
    return 0;
}

