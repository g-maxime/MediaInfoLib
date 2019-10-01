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
