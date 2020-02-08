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

struct variable_size
{
    int8u AddedSize;
    int16u Value;
};

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
        int16u max_truepk;
        bool b_loudcorr_type;
        int16u loudrelgat;
        int16u loudspchgat;
        int8u loudspchgat_dialgate_prac_type;
        int16u lra;
        int8u lra_prac_type;
        int16u max_loudmntry;

        loudness_info() :
            dialnorm_bits((int8u)-1),
            loud_prac_type((int8u)-1),
            loud_dialgate_prac_type((int8u)-1),
            max_truepk((int16u)-1),
            loudrelgat((int16u)-1),
            loudspchgat((int16u)-1),
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

    struct drc_decoder_config_curve
    {
        int8u drc_lev_nullband_low;
        int8u drc_lev_nullband_high;
        int8u drc_gain_max_boost;
        int8u drc_gain_max_cut;
        int8u drc_lev_max_cut;
        int8u drc_gain_section_cut;
        int8u drc_lev_section_cut;
        int8u drc_tc_attack;
        int8u drc_tc_release;
        int8u drc_tc_attack_fast;
        int8u drc_tc_release_fast;
        int8u drc_attack_threshold;
        int8u drc_release_threshold;

        drc_decoder_config_curve()
        {
            memset(this, -1, sizeof(drc_decoder_config_curve));
        }

        drc_decoder_config_curve(int8u drc_lev_nullband_low_, int8u drc_lev_nullband_high_, int8u drc_gain_max_boost_, int8u drc_gain_max_cut_, int8u drc_lev_max_cut_, int8u drc_gain_section_cut_, int8u drc_lev_section_cut_, int8u drc_tc_attack_, int8u drc_tc_release_, int8u drc_tc_attack_fast_, int8u drc_tc_release_fast_, int8u drc_attack_threshold_, int8u drc_release_threshold_) :
            drc_lev_nullband_low(drc_lev_nullband_low_),
            drc_lev_nullband_high(drc_lev_nullband_high_),
            drc_gain_max_boost(drc_gain_max_boost_),
            drc_gain_max_cut(drc_gain_max_cut_),
            drc_lev_max_cut(drc_lev_max_cut_),
            drc_gain_section_cut(drc_gain_section_cut_),
            drc_lev_section_cut(drc_lev_section_cut_),
            drc_tc_attack(drc_tc_attack_),
            drc_tc_release(drc_tc_release_),
            drc_tc_attack_fast(drc_tc_attack_fast_),
            drc_tc_release_fast(drc_tc_release_fast_),
            drc_attack_threshold(drc_attack_threshold_),
            drc_release_threshold(drc_release_threshold_)
        {
        }

        bool operator==(const drc_decoder_config_curve& C)
        {
            return !memcmp(this, &C, sizeof(drc_decoder_config_curve));
        }
    };

    struct drc_decoder_config
    {
        int8u drc_repeat_id;
        bool drc_default_profile_flag;
        int8u drc_decoder_mode_id;
        bool drc_compression_curve_flag;
        drc_decoder_config_curve drc_compression_curve;
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
    enum substream_type_t
    {
        Type_Ac4_Substream,
        Type_Ac4_Hsf_Ext_Substream,
        Type_Emdf_Payloads_Substream,
        Type_Ac4_Presentation_Substream,
        Type_Oamd_Substream
    };

    //Presentations
    struct presentation_substream
    {
        substream_type_t substream_type;
        int8u substream_index;
    };
    struct presentation
    {
        vector<size_t> substream_group_info_specifiers;
        vector<presentation_substream> Substreams;

        int8u presentation_version;
        int32u presentation_id;
        bool b_pres_ndot;
        bool b_alternative;
        bool dolby_atmos_indicator;
        int8u substream_index;
        int8u presentation_config;
        int8u n_substream_groups;
        int8u b_multi_pid_PresentAndValue;
        int8u frame_rate_fraction_minus1;
        loudness_info LoudnessInfo;
        drc_info DrcInfo;
        dmx Dmx;

        //Computed
        int8u pres_ch_mode;
        int8u pres_ch_mode_core;
        int8u pres_immersive_stereo;
        int8u n_substreams_in_presentation;
        bool b_pres_4_back_channels_present;
        bool b_pres_centre_present;
        int8u pres_top_channel_pairs;
        string Language;

        presentation() :
            presentation_config((int8u)-1),
            presentation_id((int32u)-1),
            frame_rate_fraction_minus1(0),
            dolby_atmos_indicator(false)
        {}
    };
    vector<presentation> Presentations;

    //Groups
    struct group_substream
    {
        substream_type_t substream_type;
        int8u substream_index;
        bool b_iframe;
        bool sus_ver;

        // b_channel_coded
        int8u ch_mode;
        bool b_4_back_channels_present;
        bool b_centre_present;
        int8u top_channels_present;
        int8u hsf_substream_index;

        // b_ajoc
        bool b_ajoc;
        bool b_static_dmx;
        int8u n_fullband_upmix_signals;
        int8u n_fullband_dmx_signals;

        // !b_ajoc
        int8u n_objects_code;
        bool b_dynamic_objects;
        bool b_lfe;
        int32u nonstd_bed_channel_assignment_mask;

        // Computed
        int8u ch_mode_core;
        int8u immersive_stereo;
        int8u top_channel_pairs;

        group_substream() :
            sus_ver(false),
            ch_mode((int8u)-1),
            ch_mode_core((int8u)-1),
            immersive_stereo((int8u)-1),
            top_channels_present((int8u)-1),
            hsf_substream_index((int8u)-1),
            nonstd_bed_channel_assignment_mask((int32u)-1)
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

    //Audio substreams
    struct buffer
    {
        int8u* Data;
        size_t Offset;
        size_t Size;

        void Create(size_t NewSize)
        {
            delete[] Data; 
            Size=NewSize;
            Data=new int8u[Size];
            Offset=0;
        }

        void Clear()
        {
            delete[] Data;
            Data=NULL;
        }

        void Append(const uint8_t* BufferToAdd, size_t SizeToAdd)
        {
            if (!Data)
                Create(SizeToAdd);
            size_t NewOffset=Offset+SizeToAdd;
            if (NewOffset>Size)
            {
                uint8_t* Data_ToDelete=Data;
                Size=NewOffset;
                Data=new int8u[Size];
                memcpy(Data, Data_ToDelete, Offset);
                delete[] Data_ToDelete;
            }
            memcpy(Data+Offset, BufferToAdd, SizeToAdd);
            Offset=NewOffset;
        }

        buffer() :
            Data(NULL)
            {}

        ~buffer()
        {
            delete[] Data;
        }
    };
    struct audio_substream
    {
        loudness_info LoudnessInfo;
        drc_info DrcInfo;
        de_info DeInfo;
        preprocessing Preprocessing;
        buffer Buffer;
        int8u Buffer_Index;
        bool b_iframe;

        audio_substream(bool b_iframe_) :
            Buffer_Index(0),
            b_iframe(b_iframe_)
        {}
    };
    std::map<int8u, audio_substream> AudioSubstreams;

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
    void raw_ac4_frame_substreams();
    void ac4_toc();
    void ac4_presentation_info();
    void ac4_presentation_v1_info();
    void ac4_sgi_specifier(presentation& P);
    void ac4_substream_info(presentation& P);
    void ac4_substream_group_info(presentation* P=NULL);
    void ac4_hsf_ext_substream_info(group_substream& G, bool b_substreams_present);
    void ac4_substream_info_chan(group_substream& G, size_t Pos, bool b_substreams_present);
    void ac4_substream_info_ajoc(group_substream& G, bool b_substreams_present);
    void ac4_substream_info_obj(group_substream& G, bool b_substreams_present);
    void ac4_presentation_substream_info(presentation& P);
    void presentation_config_ext_info(int8u presentation_config);
    void bed_dyn_obj_assignment(group_substream& G);
    void content_type(content_info& ContentInfo);
    void frame_rate_multiply_info();
    void frame_rate_fractions_info(presentation& P);
    void emdf_info(presentation_substream& P);
    void emdf_payloads_substream_info(presentation_substream& P);
    void emdf_protection();
    void substream_index_table();
    void oamd_substream_info(group_substream& G, bool b_substreams_present);
    void oamd_common_data();

    void ac4_substream(size_t substream_index);
    void ac4_presentation_substream(size_t substream_index, size_t Substream_Index);

    void metadata(audio_substream& AudioSubstream, size_t Substream_Index);
    void basic_metadata(loudness_info& LoudnessInfo, preprocessing& Preprocessing, int8u ch_mode, bool sus_ver);
    void extended_metadata(bool b_associated, bool b_dialog, int8u ch_mode, bool sus_ver);
    void dialog_enhancement(de_info& Info, int8u ch_mode, bool b_iframe);
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
    void drc_compression_curve(drc_decoder_config_curve& Curve);

    void further_loudness_info(loudness_info& LoudnessInfo, bool sus_ver, bool b_presentation_ldn);

    void dac4();
    void alternative_info();
    void ac4_bitrate_dsi();
    void ac4_presentation_v1_dsi(presentation& P);
    void ac4_substream_group_dsi(presentation& P);

    //Parsing
    void Get_V4 (int8u  Bits, int32u  &Info, const char* Name);
    void Skip_V4(int8u  Bits, const char* Name);
    void Get_V4(int8u Bits1, int8u Bits2, int8u Flag_Value, int32u &Info, const char* Name);
    void Skip_V4(int8u Bits1, int8u Bits2, int8u Flag_Value, const char* Name);
    void Get_V4(int8u Bits1, int8u Bits2, int8u Bits3, int8u Bits4, int32u  &Info, const char* Name);
    void Get_V4(const variable_size* Bits, int8u &Info, const char* Name);
    void Get_VB (int8u  &Info, const char* Name);
    void Skip_VB(const char* Name);

    //Utils
    bool CRC_Compute(size_t Size);
    int8u Superset(int8u Ch_Mode1, int8u Ch_Mode2);
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
    int8u n_substreams;


    vector<size_t> IFrames;
    std::vector<size_t> Substream_Size;
    std::map<int8u, substream_type_t> Substream_Type;
};

} //NameSpace

#endif
