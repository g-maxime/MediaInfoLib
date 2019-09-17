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
        int8u dialnorm_bits;
        int16u truepk;

        loudness_info() :
            dialnorm_bits((int8u)-1),
            truepk((int16u)-1)
        {}
    };

    struct drc_info
    {
        int8u drc_eac3_profile;

        drc_info() :
            drc_eac3_profile((int8u)-1)
        {}
    };

    struct ac4_substream_infos
    {
        bool Sus_Ver;
        bool Channel_Coded;
        int16u Channel_Mode;
        loudness_info LoudnessInfo;
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
    void content_type();
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
    void basic_metadata(loudness_info& LoudnessInfo, int16u channel_mode, bool sus_ver);
    void extended_metadata(int16u channel_mode, bool sus_ver);

    void custom_dmx_data(int8u pres_ch_mode, int8u pres_ch_mode_core, bool b_pres_4_back_channels_present, int8u pres_top_channel_pairs, bool b_pres_has_lfe);
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
    void drc_data();
    void drc_gains(int8u Index);
    void drc_decoder_mode_config(int8u Index);
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

        presentation() :
            presentation_config((int8u)-1)
        {}
    };
    vector<presentation> Presentations;
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
        int8u content_classifier;
        string language_tag_bytes;
        bool b_channel_coded;
        bool b_hsf_ext;
        group() :
            content_classifier((int8u)-1)
        {}
    };
    vector<group> Groups;
    struct drc_decoder_config
    {
        int8u drc_decoder_mode_id;
        bool drc_compression_curve_flag;
        int8u drc_gains_config;
    };
    vector<drc_decoder_config>Decoders;


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
    std::map<int8u, ac4_substream_infos> Substream_Infos;
    //std::map<int8u, ac4_substream_group_infos> Substream_Group_Infos;
};

} //NameSpace

#endif
