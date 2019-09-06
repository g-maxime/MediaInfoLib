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

    enum ac4_substream_type
    {
        Type_Ac4_Substream,
        Type_Ac4_Hsf_Ext_Substream,
        Type_Emdf_Payloads_Substream,
        Type_Ac4_Presentation_Substream,
        Type_Oamd_Substream
    };


    struct drc_decoder_config_infos
    {
        bool Compression_Curve_Flag;
        int8u Gains_Config;
        drc_decoder_config_infos() : Compression_Curve_Flag(false), Gains_Config(0) {};
    };

    struct ac4_substream_infos
    {
        bool Sus_Ver;
        bool Pres_Ndot;
        bool Alternative;
        int16u Substream_Size;
        int16u Channel_Mode;
        int8u Substream_Groups;
        int8u Presentation_Substreams;
        ac4_substream_type Substream_Type;
        ac4_substream_infos() : Sus_Ver(false), Pres_Ndot(false), Alternative(false), Substream_Size(0), Channel_Mode(0), Substream_Groups(0), Presentation_Substreams(0), Substream_Type(Type_Ac4_Substream) {};
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
    void ac4_substream_info_chan(bool b_substreams_present);
    void ac4_substream_info_ajoc(bool b_substreams_present);
    void ac4_substream_info_obj(bool b_substreams_present);
    void ac4_presentation_substream_info(int8u n_substream_groups, int8u n_substreams_in_presentation);
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
    void basic_metadata(int16u channel_mode, bool sus_ver);
    void extended_metadata(int16u channel_mode, bool sus_ver);

    void drc_frame(bool b_iframe);
    void drc_config(int8u& drc_decoder_nr_mode, std::map<int8u, int8u>& decoder_ids, std::map<int8u, drc_decoder_config_infos>& decoder_infos);
    void drc_data(int8u drc_decoder_nr_modes, std::map<int8u, int8u> decoder_ids, std::map<int8u, drc_decoder_config_infos> decoder_infos);
    void drc_gains(int8u drc_decoder_mode_id);
    void drc_decoder_mode_config(int8u Index, std::map<int8u, int8u>& decoder_ids, std::map<int8u, drc_decoder_config_infos>& decoder_infos);
    void drc_compression_curve();

    void further_loudness_info(bool sus_ver, bool b_presentation_ldn);

    void dac4();

    //Parsing
    void Get_V4 (int8u  Bits, int32u  &Info, const char* Name);
    void Skip_V4(int8u  Bits, const char* Name);
    void Get_V4(int8u Bits1, int8u Bits2, int8u Bits3, int8u Bits4, int32u  &Info, const char* Name);
    void Get_V4(int8u Bits1, int8u Bits2, int8u Bits3, int8u Bits4, int8u Bits5, int8u Bits6, int32u  &Info, const char* Name);
    void Get_VB (int8u  &Info, const char* Name);
    void Skip_VB(const char* Name);

    //Utils
    bool CRC_Compute(size_t Size);
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

    std::map<int8u, ac4_substream_infos> Substream_Info;
    std::map<int8u, int32u> Substream_Size;
    std::map<int8u, ac4_substream_type> Substream_Type;
};

} //NameSpace

#endif
