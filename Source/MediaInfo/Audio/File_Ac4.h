/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_Ac4H
#define MediaInfo_Ac4H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/TimeCode.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{
typedef const int8s (*ac4_huffman)[2];

const int8s de_hcb_abs_0[31][2] = {
   {  3,   1},
   {  5,   2},
   {-64,   9},
   {  7,   4},
   {-54, -63},
   {  6,  12},
   { 26, -62},
   {  8,  15},
   {-56, -61},
   { 11,  10},
   { 16, -60},
   { 28, -59},
   { 13,  14},
   { 30, -58},
   { 24, -57},
   { 18, -55},
   { 17, -46},
   {-49, -53},
   { 21,  19},
   { 20, -37},
   { 23, -52},
   { 22, -36},
   {-51, -34},
   {-33, -50},
   {-41,  25},
   {-48, -35},
   { 27,  29},
   {-47, -43},
   {-39, -45},
   {-44, -38},
   {-42, -40}
};

const int8s de_hcb_abs_1[60][2] = {
    {-64,   1},
    {  2,  12},
    { 36,   3},
    {  4,  58},
    {-59,   5},
    { 53,   6},
    {-50,   7},
    {  8,  52},
    {-94,   9},
    { 10, -80},
    {-38,  11},
    {-93, -92},
    { 13,  27},
    { 14, -63},
    { 15, -54},
    {-51,  16},
    { 17,  22},
    {-49,  18},
    { 19, -74},
    {-79,  20},
    {-85,  21},
    {-91, -35},
    {-70,  23},
    { 48,  24},
    { 25, -34},
    { 26, -84},
    {-90, -89},
    { 28,  57},
    { 29,  59},
    {-66,  30},
    { 31, -52},
    { 49,  32},
    { 33, -73},
    { 34, -77},
    { 35, -40},
    {-36, -88},
    { 37,  55},
    { 38, -57},
    {-68,  39},
    { 40,  45},
    { 41,  51},
    { 44,  42},
    {-83,  43},
    {-37, -87},
    {-86, -39},
    {-72,  46},
    { 47, -45},
    {-41, -82},
    {-42, -81},
    { 50, -47},
    {-78, -43},
    {-44, -76},
    {-46, -75},
    {-69,  54},
    {-48, -71},
    { 56, -58},
    {-67, -53},
    {-65, -62},
    {-60, -61},
    {-55, -56}
};

const int8s de_hcb_diff_0[62][2] = {
    {  1, -64},
    {  2,  32},
    {  3, -63},
    {  4,  22},
    {-61,   5},
    {  6, -68},
    {-57,   7},
    {  8, -71},
    { 54,   9},
    {-74,  10},
    { 11,  17},
    { 12,  14},
    { 13, -76},
    {-95, -94},
    { 15,  16},
    {-93, -92},
    {-90, -91},
    { 18,  20},
    {-42,  19},
    {-89, -88},
    { 21,  61},
    {-87, -35},
    {-67,  23},
    {-60,  24},
    {-58,  25},
    { 26, -55},
    { 27, -72},
    {-52,  28},
    {-48,  29},
    { 58,  30},
    { 31,  49},
    {-37, -86},
    { 33, -65},
    {-66,  34},
    { 35, -62},
    { 36,  59},
    {-69,  37},
    { 38, -56},
    { 39,  44},
    { 40, -53},
    {-49,  41},
    {-46,  42},
    { 43,  50},
    {-38, -85},
    {-73,  45},
    { 46, -50},
    { 47,  51},
    { 48, -44},
    {-40, -84},
    {-83, -82},
    {-39, -81},
    { 53,  52},
    {-80, -41},
    {-79, -78},
    {-51,  55},
    {-47,  56},
    { 57, -45},
    {-77, -33},
    {-75, -43},
    {-59,  60},
    {-54, -70},
    {-34, -36}
};

const int8s de_hcb_diff_1[120][2] = {
    { -64,    1},
    {  94,    2},
    {  49,    3},
    {   4,  -63},
    {   5,  -62},
    { -68,    6},
    {  13,    7},
    {  19,    8},
    {  45,    9},
    {  10,   58},
    {  11,   62},
    {  12,   61},
    {-124, -102},
    { -56,   14},
    {  67,   15},
    { -78,   16},
    {  24,   17},
    {  18,  118},
    {-122, -123},
    {  20,   34},
    {  29,   21},
    {  26,   22},
    {  23,   28},
    {-121,  -14},
    {  25,  119},
    {-120,   -6},
    {  32,   27},
    {-116, -119},
    {-113, -118},
    { -82,   30},
    {  33,   31},
    {-117,   -5},
    {-114, -115},
    {-110, -112},
    {  40,   35},
    {  36,   38},
    {  37,  -88},
    {-111,  -12},
    { 114,   39},
    { -19, -109},
    {  41,   43},
    {  42,  117},
    {  -9, -108},
    {  44,  116},
    { -20, -107},
    {  46,   86},
    {  47,  111},
    { 115,   48},
    {-106, -105},
    {  50,   69},
    {  51,  -67},
    {  52,  -69},
    { -57,   53},
    {  54,   88},
    {  55,   93},
    {  56,   65},
    {  57,  -42},
    { -97, -104},
    { -48,   59},
    {  60,  -43},
    {-103,  -33},
    {-101,  -32},
    {  64,   63},
    {-100,  -98},
    { -99,  -31},
    {  66,  -85},
    { -96,  -95},
    {  68,  -50},
    { -47,  -94},
    {  70,  101},
    {  71,   77},
    {  72,  -71},
    { -74,   73},
    { -76,   74},
    { 105,   75},
    { -44,   76},
    { -93,  -92},
    {  78,  -58},
    { -55,   79},
    {  91,   80},
    {  81,   84},
    {  82,   83},
    { -39,  -91},
    { -40,  -90},
    { -83,   85},
    { -41,  -89},
    { 108,   87},
    { 109,  -87},
    {  89,  -52},
    { -80,   90},
    { 107,  -86},
    {  92,  -79},
    { -84,  -46},
    { -81,  -49},
    { -65,   95},
    { -66,   96},
    {  97,  -61},
    { -59,   98},
    { -72,   99},
    { 100,  -54},
    { -77,  -51},
    { -60,  102},
    { 103,  -70},
    { -73,  104},
    { -53,  -75},
    { 106,  -45},
    { -37,  -38},
    { -36,  -35},
    { -34,  110},
    { -30,  -29},
    { -28,   -4},
    { 113,  112},
    { -25,  -27},
    { -24,  -26},
    { -17,  -23},
    { -21,  -22},
    { -15,  -18},
    { -16,  -13},
    { -10,  -11},
    {  -7,   -8}
};

//***************************************************************************
// Class File_Ac4
//***************************************************************************

class File_Ac4 : public File__Analyze
{
public :
    //In
    int64u Frame_Count_Valid;
    bool   MustParse_dac4;

    struct loudness_info
    {
        int8u dialnorm_bits; //TODO
        int8u loud_prac_type;
        int8u loud_dialgate_prac_type;
        bool b_loudcorr_type;
        int16u loudrelgat;
        int16u loudspchgat;
        int16u truepk;
        int16u lra;
        int8u lra_prac_type;
        int16u max_loudmntry;

        loudness_info() :
            dialnorm_bits((int8u)-1),
            loud_prac_type((int8u)-1),
            loud_dialgate_prac_type((int8u)-1),
            loudrelgat((int16u)-1),
            loudspchgat((int16u)-1),
            truepk((int16u)-1),
            lra((int16u)-1),
            max_loudmntry((int16u)-1)
        {}
    };

    struct preprocessing
    {
        int8u pre_dmixtyp_2ch;
        int8u phase90_info_2ch;
        int8u pre_dmixtyp_5ch;
        int8u phase90_info_mc;
        bool b_surround_attenuation_known;
        bool b_lfe_attenuation_known;

        preprocessing() :
            pre_dmixtyp_2ch((int8u)-1),
            pre_dmixtyp_5ch((int8u)-1),
            phase90_info_mc((int8u)-1)
        {}
    };

    struct drc_decoder_config
    {
        int8u drc_repeat_id;
        bool drc_default_profile_flag;
        int8u drc_decoder_mode_id;
        bool drc_compression_curve_flag;
        int8u drc_gains_config;

        drc_decoder_config() :
            drc_repeat_id((int8u)-1)
        {}
    };

    struct drc_info
    {
        vector<drc_decoder_config> Decoders;
        int8u drc_eac3_profile;

        drc_info() :
            drc_eac3_profile((int8u)-1)
        {}
    };

    struct de_config
    {
        int8u de_method;
        int8u de_max_gain;
        int8u de_channel_config;

        de_config() :
            de_method((int8u)-1)
        {}
    };

    struct de_info
    {
        bool b_de_data_present;
        de_config Config;

        de_info() :
            b_de_data_present(false)
        {}
    };

    struct dmx
    {
        int8u loro_centre_mixgain;
        int8u loro_surround_mixgain;
        int8u ltrt_centre_mixgain;
        int8u ltrt_surround_mixgain;
        int8u lfe_mixgain;
        int8u preferred_dmx_method;

        dmx() :
            loro_centre_mixgain((int8u)-1),
            loro_surround_mixgain((int8u)-1),
            ltrt_centre_mixgain((int8u)-1),
            ltrt_surround_mixgain((int8u)-1),
            lfe_mixgain((int8u)-1),
            preferred_dmx_method((int8u)-1)
        {}
    };

    struct content_info
    {
        int8u content_classifier;
        string language_tag_bytes;
        content_info() :
            content_classifier((int8u)-1)
        {}
    };

    //Constructor/Destructor
    File_Ac4();
    ~File_Ac4();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Synchro
    bool Synchronize();
    void Synched_Init();
    bool Synched_Test();

    //Buffer - Global
    void Read_Buffer_Continue ();
    void Read_Buffer_Unsynched();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void raw_ac4_frame();
    void ac4_toc();
    void ac4_presentation_info();
    void ac4_presentation_v1_info();
    void ac4_sgi_specifier();
    void ac4_substream_info();
    void ac4_substream_group_info();
    void ac4_hsf_ext_substream_info(bool b_substreams_present);
    void ac4_substream_info_chan(bool sus_ver);
    void ac4_substream_info_ajoc(bool b_substreams_present);
    void ac4_substream_info_obj(bool b_substreams_present);
    void ac4_presentation_substream_info();
    void presentation_config_ext_info(int8u presentation_config);
    void bed_dyn_obj_assignment(int8u n_signals);
    void content_type(content_info& ContentInfo);
    void frame_rate_multiply_info();
    void frame_rate_fractions_info();
    void emdf_info();
    void emdf_payloads_substream_info();
    void emdf_protection();
    void substream_index_table();
    void oamd_substream_info(bool b_substreams_present);
    void oamd_common_data();

    void ac4_substream(size_t Substream_Index);
    void ac4_presentation_substream(size_t Substream_Index);

    void metadata(size_t Substream_Index);
    void basic_metadata(loudness_info& LoudnessInfo, preprocessing& Preprocessing, int16u channel_mode, bool sus_ver);
    void extended_metadata(int16u channel_mode, bool sus_ver);
    void dialog_enhancement(de_info& Info, int16u channel_mode, bool b_iframe);
    void dialog_enhancement_config(de_info& Info);
    void dialog_enhancement_data(de_info& Info, bool b_iframe, bool b_de_simulcast);
    void custom_dmx_data(dmx& Dmx, int8u pres_ch_mode, int8u pres_ch_mode_core, bool b_pres_4_back_channels_present, int8u pres_top_channel_pairs, bool b_pres_has_lfe);
    void cdmx_parameters(int8u bs_ch_config, int8u out_ch_config);
    void tool_scr_to_c_l();
    void tool_b4_to_b2();
    void tool_t4_to_t2();
    void tool_t4_to_f_s_b();
    void tool_t4_to_f_s();
    void tool_t2_to_f_s_b();
    void tool_t2_to_f_s();
    void loud_corr(int8u pres_ch_mode, int8u pres_ch_mode_core, bool b_objects);
    void drc_frame(drc_info& DrcInfo, bool b_iframe);
    void drc_config(drc_info& DrcInfo);
    void drc_data(drc_info& DrcInfo);
    void drc_gains(drc_decoder_config& Decoder);
    void drc_decoder_mode_config(drc_decoder_config& Decoder);
    void drc_compression_curve();

    void further_loudness_info(loudness_info& LoudnessInfo, bool sus_ver, bool b_presentation_ldn);

    void dac4();

    //Parsing
    void Get_V4 (int8u  Bits, int32u  &Info, const char* Name);
    void Skip_V4(int8u  Bits, const char* Name);
    void Get_V4(int8u Bits1, int8u Bits2, int8u Flag_Value, int32u &Info, const char* Name);
    void Skip_V4(int8u Bits1, int8u Bits2, int8u Flag_Value, const char* Name);
    void Get_V4(int8u Bits1, int8u Bits2, int8u Bits3, int8u Bits4, int32u  &Info, const char* Name);
    void Get_V4(int8u Bits1, int8u Bits2, int8u Bits3, int8u Bits4, int8u Bits5, int8u Bits6, int32u  &Info, const char* Name);
    void Get_VB (int8u  &Info, const char* Name);
    void Skip_VB(const char* Name);

    //Info
    enum substream_type_t
    {
        Type_Ac4_Substream,
        Type_Ac4_Hsf_Ext_Substream,
        Type_Emdf_Payloads_Substream,
        Type_Ac4_Presentation_Substream,
        Type_Oamd_Substream
    };

    //Presentations
    struct presentation
    {
        bool b_pres_ndot;
        bool b_alternative;
        int8u substream_index;
        int8u presentation_config;
        int8u n_substream_groups;
        int8u b_multi_pid_PresentAndValue;
        vector<size_t> substream_group_info_specifiers;
        loudness_info LoudnessInfo;
        drc_info DrcInfo;
        dmx Dmx;

        presentation() :
            presentation_config((int8u)-1)
        {}
    };
    vector<presentation> Presentations;
    presentation* Presentation_Current;

    //Groups
    struct group_substream
    {
        substream_type_t substream_type;
        int8u substream_index;
        int8u ch_mode_core;
        int8u ch_mode;

        bool b_4_back_channels_present; // TODO: Move to audio_substream
        int8u top_channels_present;     // TODO: Move to audio_substream
        bool b_static_dmx;              // TODO: Move to audio_substream

        group_substream() :
            ch_mode_core((int8u)-1),
            ch_mode((int8u)-1),
            b_4_back_channels_present(false),
            top_channels_present(0),
            b_static_dmx(false)
        {}
    };

    struct group
    {
        vector<group_substream> Substreams;
        content_info ContentInfo;
        bool b_channel_coded;
        bool b_hsf_ext;
    };
    vector<group> Groups;
    group* Group_Current;

    //Audio substreams
    struct audio_substream
    {
        content_info ContentInfo;
        bool Sus_Ver;
        bool Channel_Coded;
        int16u Channel_Mode;
        loudness_info LoudnessInfo;
        de_info DeInfo;
        preprocessing Preprocessing;
    };
    std::map<int8u, audio_substream> AudioSubstreams;

    //Utils
    bool CRC_Compute(size_t Size);
    int8u Superset(int8u Ch_Mode1, int8u Ch_Mode2);
    int8u Channel_Mode_to_Ch_Mode(int16u Channel_Mode);
    bool Channel_Mode_Contains_Lfe(int16u Channel_Mode);
    bool Channel_Mode_Contains_C(int16u Channel_Mode);
    bool Channel_Mode_Contains_Lr(int16u Channel_Mode);
    bool Channel_Mode_Contains_LsRs(int16u Channel_Mode);
    bool Channel_Mode_Contains_LrsRrs(int16u Channel_Mode);
    bool Channel_Mode_Contains_LwRw(int16u Channel_Mode);
    bool Channel_Mode_Contains_VhlVhr(int16u Channel_Mode);
    int16u Huffman_Decode(const ac4_huffman& Table, const char* Name);

    //Temp
    int32u frame_size;
    int16u sync_word;
    int8u bitstream_version;
    int8u frame_rate_index;
    bool fs_index;
    int8u payload_base;
    int8u frame_rate_factor;
    int8u frame_rate_fraction;
    int8u max_group_index;
    int8u sus_ver;
    int8u n_substreams;


    vector<size_t> IFrames;
    std::vector<size_t> Substream_Size;
    std::map<int8u, substream_type_t> Substream_Type;
};

} //NameSpace

#endif
