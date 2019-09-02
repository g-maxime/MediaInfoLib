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
    void ac4_substream_info();
    void ac4_hsf_ext_substream_info();
    void presentation_config_ext_info();
    void content_type();
    void frame_rate_multiply_info();
    void umd_info();
    void umd_payloads_substream_info();
    void umd_protection();
    void substream_index_table();
    void dac4();

    //Parsing
    void Get_V4 (int8u  Bits, int32u  &Info, const char* Name);
    void Skip_V4(int8u  Bits, const char* Name);
    void Get_V4(int8u Bits1, int8u Bits2, int8u Bits3, int8u Bits4, int32u  &Info, const char* Name);
    void Get_VB (int8u  &Info, const char* Name);
    void Skip_VB(const char* Name);

    //Utils
    bool CRC_Compute(size_t Size);

    //Temp
    int32u frame_size;
    int16u sync_word;
    int8u bitstream_version;
    int8u frame_rate_index;
    bool fs_index;
    int8u n_presentations;
    int8u payload_base;
    int8u presentation_config;
    int8u presentation_version;
    int8u presentation_group;
    bool multiplier_bit;
    bool b_multiplier;
    int8u umd_version;
    int8u key_id;
    int8u substream_index;
    int8u channel_mode;
    bool sf_multiplier;
    int8u bitrate_indicator;
    bool add_ch_base;
    int8u frame_rate_factor;
    int8u content_classifier;
    bool b_start_tag;
    int16u language_tag_chunk;
    int8u n_substreams;
};

} //NameSpace

#endif
