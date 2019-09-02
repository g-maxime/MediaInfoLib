/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_AC4_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Ac4.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
// CRC_16_Table
extern const int16u CRC_16_Table[256];

//---------------------------------------------------------------------------
extern const float64 Ac4_frame_rate[2][16]=
{
    { //44.1 kHz
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)0,
        (float64)11025/(float64)512,
        (float64)0,
        (float64)0,
    },
    { //48 kHz
        (float64)24000/(float64)1001,
        (float64)24,
        (float64)25,
        (float64)30000/(float64)1001,
        (float64)30,
        (float64)48000/(float64)1001,
        (float64)48,
        (float64)50,
        (float64)60000/(float64)1001,
        (float64)60,
        (float64)100,
        (float64)120000/(float64)1001,
        (float64)120,
        (float64)12000/(float64)512,
        (float64)0,
        (float64)0,
    },
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Ac4::File_Ac4()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    MustSynchronize=true;
    Buffer_TotalBytes_FirstSynched_Max=32*1024;
    Buffer_TotalBytes_Fill_Max=1024*1024;
    PTS_DTS_Needed=true;
    StreamSource=IsStream;
    Frame_Count_NotParsedIncluded=0;

    //In
    Frame_Count_Valid=0;
    MustParse_dac4=false;
}

//---------------------------------------------------------------------------
File_Ac4::~File_Ac4()
{
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Streams_Fill()
{
    Fill(Stream_General, 0, General_Format, "AC-4");

    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "AC-4");
    Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, "Dolby AC-4");
    Fill(Stream_Audio, 0, Audio_Format_Version, __T("Version ")+Ztring::ToZtring(bitstream_version));
    Fill(Stream_Audio, 0, Audio_SamplingRate, fs_index?48000:44100);
    Fill(Stream_Audio, 0, Audio_FrameRate, Ac4_frame_rate[fs_index][frame_rate_index]);
}

//---------------------------------------------------------------------------
void File_Ac4::Streams_Finish()
{
}

//---------------------------------------------------------------------------
void File_Ac4::Read_Buffer_Unsynched()
{
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ac4::Synchronize()
{
    //Synchronizing
    size_t Buffer_Offset_Current;
    while (Buffer_Offset<Buffer_Size)
    {
        Buffer_Offset_Current=Buffer_Offset;
        Synched=true; //For using Synched_Test()
        int8s i=0;
        const int8s count=4;
        for (; i<count; i++) //4 frames in a row tested
        {
            if (!Synched_Test())
            {
                Buffer_Offset=Buffer_Offset_Current;
                Synched=false;
                return false;
            }
            if (!Synched)
                break;
            Buffer_Offset+=frame_size;
        }
        Buffer_Offset=Buffer_Offset_Current;
        if (i==count)
            break;
        Buffer_Offset++;
    }
    Buffer_Offset=Buffer_Offset_Current;

    //Parsing last bytes if needed
    if (Buffer_Offset+4>Buffer_Size)
    {
        while (Buffer_Offset+2<=Buffer_Size && (BigEndian2int16u(Buffer+Buffer_Offset)>>1)!=(0xAC40>>1))
            Buffer_Offset++;
        if (Buffer_Offset+1==Buffer_Size && Buffer[Buffer_Offset]==0xAC)
            Buffer_Offset++;
        return false;
    }

    //Synched
    return true;
}

//---------------------------------------------------------------------------
void File_Ac4::Synched_Init()
{
    Accept();
    
    if (!Frame_Count_Valid)
        Frame_Count_Valid=Config->ParseSpeed>=0.3?32:2;

    //FrameInfo
    PTS_End=0;
    if (!IsSub)
    {
        FrameInfo.DTS=0; //No DTS in container
        FrameInfo.PTS=0; //No PTS in container
    }
    DTS_Begin=FrameInfo.DTS;
    DTS_End=FrameInfo.DTS;
    if (Frame_Count_NotParsedIncluded==(int64u)-1)
        Frame_Count_NotParsedIncluded=0; //No Frame_Count_NotParsedIncluded in the container
}

//---------------------------------------------------------------------------
bool File_Ac4::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+4>=Buffer_Size)
        return false;

    //sync_word
    sync_word=BigEndian2int16u(Buffer+Buffer_Offset);
    if ((sync_word>>1)!=(0xAC40>>1)) //0xAC40 or 0xAC41
    {
        Synched=false;
        return true;
    }

    //frame_size
    frame_size=BigEndian2int16u(Buffer+Buffer_Offset+2);
    if (frame_size==(int16u)-1)
    {
        if (Buffer_Offset+7>Buffer_Size)
            return false;
        frame_size=BigEndian2int24u(Buffer+Buffer_Offset+4)+7;
    }
    else
        frame_size+=4;

    //crc_word
    if (sync_word&1)
    {
        frame_size+=2;
        if (Buffer_Offset+frame_size>Buffer_Size)
            return false;
        if (!CRC_Compute(frame_size))
            Synched=false;
    }

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Read_Buffer_Continue()
{
    if (MustParse_dac4)
    {
        dac4();
        return;
    }

    if (!MustSynchronize)
    {
        raw_ac4_frame();
        Buffer_Offset=Buffer_Size;
    }
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Header_Parse()
{
    //Parsing
    //sync_word & frame_size16 were previously calculated in Synched_Test()
    int16u frame_size16;
    Skip_B2 (                                                   "sync_word");
    Get_B2 (frame_size16,                                       "frame_size");
    if (frame_size16==0xFFFF)
        Skip_B3(                                                "frame_size");

    //Filling
    Header_Fill_Size(frame_size);
    Header_Fill_Code(sync_word, "ac4_syncframe");
}

//---------------------------------------------------------------------------
void File_Ac4::Data_Parse()
{
    //CRC
    if (Element_Code==0xAC41)
        Element_Size-=2;

    //Parsing
    raw_ac4_frame();
    
    //CRC
    Element_Offset=Element_Size;
    if (Element_Code==0xAC41)
    {
        Element_Size+=2;
        Skip_B2(                                                "crc_word");
    }
}

//---------------------------------------------------------------------------
void File_Ac4::raw_ac4_frame()
{
    Element_Begin1("raw_ac4_frame");
    BS_Begin();
    ac4_toc();
    BS_End();
    Element_End0();

    FILLING_BEGIN();
        Frame_Count++;
        if (!Status[IsFilled] && Frame_Count>=Frame_Count_Valid)
        {
            Fill();
            Finish();
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_toc()
{
    int16u sequence_counter;
    Element_Begin1("raw_ac4_toc");
    Get_S1 (2, bitstream_version,                               "bitstream_version");
    if (bitstream_version==3)
    {
        int32u bitstream_version32; 
        Get_V4 (2, bitstream_version32,                         "bitstream_version");
        bitstream_version32+=3;
        bitstream_version=(int8u)bitstream_version32;
    }
    Get_S2 (10, sequence_counter,                               "sequence_counter");
    TEST_SB_SKIP(                                               "b_wait_frames");
        int8u wait_frames;
        Get_S1 (3, wait_frames,                                 "wait_frames");
        if (wait_frames)
            Skip_S1(2,                                          "reserved");
    TEST_SB_END();
    Get_SB (   fs_index,                                        "fs_index");
    Get_S1 (4, frame_rate_index,                                "frame_rate_index"); Param_Info1(Ac4_frame_rate[fs_index][frame_rate_index]);
    Skip_SB(                                                    "b_iframe_global");
    TESTELSE_SB_SKIP(                                           "b_single_presentation");
        n_presentations=1;
    TESTELSE_SB_ELSE(                                           "b_single_presentation");
        TESTELSE_SB_SKIP(                                       "b_more_presentations");
            int32u n_presentations32;
            Get_V4 (2, n_presentations32,                       "n_presentations");
            n_presentations32+=2;
            n_presentations=(int8u)n_presentations32;
        TESTELSE_SB_ELSE(                                       "b_more_presentations");
            n_presentations=0;
        TESTELSE_SB_END();
    TESTELSE_SB_END();

    payload_base=0;
    TEST_SB_SKIP(                                               "b_payload_base");
        int8u payload_base_minus1;
        Get_S1 (7, payload_base_minus1,                         "payload_base");
        payload_base=payload_base_minus1+1;
        if (payload_base==0x20)
        {
            int32u payload_base32;
            Get_V4 (3, payload_base32,                          "payload_base");
            payload_base32+=payload_base;
            payload_base=(int8u)payload_base32;
        }
    TEST_SB_END();

    for(size_t Pos=0; Pos<n_presentations; Pos++) {
        ac4_presentation_info();
    }
    substream_index_table();

    // TODO: byte align

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_info()
{
    bool b_single_substream;
    bool b_add_umd_substreams=false;

    Element_Begin1(                                             "ac4_presentation_info");
    Get_SB(b_single_substream,                                  "b_single_substream");
    if (!b_single_substream)
    {
        Get_S1(3, presentation_config,                          "presentation_config");
        if (presentation_config==7) {
            int32u presentation_config32;
            Get_V4(2, presentation_config32,                    "presentation_config");
            presentation_config+=presentation_config32;
        }
    }

    Get_VB(presentation_version,                                "presentation_version");

    if (!b_single_substream && presentation_config==6)
    {
        b_add_umd_substreams=true;
    }
    else
    {
        bool b_mdcompat;
        Get_SB(b_mdcompat,                                      "b_mdcompat");
        TESTELSE_SB_SKIP(                                       "b_belongs_to_presentation_group");
            int32u presentation_group32;
            Get_V4(2, presentation_group32,                     "presentation_group");
            presentation_group=(int8u)presentation_group32;
        TESTELSE_SB_ELSE(                                       "b_belongs_to_presentation_group");
            presentation_group=0;
        TESTELSE_SB_END();

        frame_rate_multiply_info();
        umd_info();

        if (b_single_substream)
        {
            ac4_substream_info();
        }
        else
        {
            bool b_hsf_ext;
            Get_SB(b_hsf_ext,                                   "b_hsf_ext");
            switch(presentation_config) // TODO: Symplify
            {
            case 0: // Dry main + Dialog
                ac4_substream_info(); // Main
                if (b_hsf_ext)
                    ac4_hsf_ext_substream_info(); // Main HSF
                ac4_substream_info(); // Dialog
            break;
            case 1: // Main + DE
                ac4_substream_info(); // Main
                if (b_hsf_ext)
                    ac4_hsf_ext_substream_info(); // Main HSF
                ac4_substream_info(); // DE
            break;
            case 2: // Main + Associate
                ac4_substream_info(); // Main
                if (b_hsf_ext)
                    ac4_hsf_ext_substream_info(); // Main HSF
                ac4_substream_info(); // Associate
            break;
            case 3: // Dry Main + Dialog + Associate
                ac4_substream_info(); // Main
                if (b_hsf_ext)
                    ac4_hsf_ext_substream_info(); // Main HSF
                ac4_substream_info(); // Dialog
                ac4_substream_info(); // Associate
            break;
            case 4: // Main + DE Associate
                ac4_substream_info(); // Main
                if (b_hsf_ext)
                    ac4_hsf_ext_substream_info(); // Main HSF
                ac4_substream_info(); // DE
                ac4_substream_info(); // Associate
            break;
            case 5: // Main + HSF ext
                ac4_substream_info(); // Main
                if (b_hsf_ext)
                    ac4_hsf_ext_substream_info(); // Main HSF
            break;
            default:
                presentation_config_ext_info();
            break;
            }
        }
        Skip_SB(                                                "b_pre_virtualized");
        Get_SB(b_add_umd_substreams,                            "b_add_umd_substreams");
    }

    if (b_add_umd_substreams)
    {
        int8u n_add_umd_substreams;
        Get_S1 (2, n_add_umd_substreams,                        "n_add_umd_substreams");
        if (n_add_umd_substreams==0)
        {
            int32u n_add_umd_substreams32;
            Get_V4(2, n_add_umd_substreams32,                   "n_add_umd_substreams");
            n_add_umd_substreams32+=4;
            n_add_umd_substreams=(int8u)n_add_umd_substreams32;
        }

        for (int8u Pos=0; Pos < n_add_umd_substreams; Pos++)
            umd_info();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info()
{
    Element_Begin1(                                             "ac4_substream_info");
        int32u channel_mode32;
        Get_V4(1, 2, 4, 7, channel_mode32,                      "channel_mode");
        if (channel_mode32==127)
        {
            Get_V4(2, channel_mode32,                           "channel_mode");
            channel_mode32+=127;
        }
        channel_mode=(int8u) channel_mode32;

        if (fs_index)
        {
            TEST_SB_SKIP(                                       "b_sf_multiplier");
                Get_SB(sf_multiplier,                           "sf_multiplier");
            TEST_SB_END();
        }

        TEST_SB_SKIP(                                           "b_bitrate_info");
            //TODO: move to a function
            int8u Count=3;
            Peek_S1(Count, bitrate_indicator);
            if (bitrate_indicator==4)
            {
                Count=5;
                Peek_S1(Count, bitrate_indicator);
            }
            BS->Skip(Count);

            #if MEDIAINFO_TRACE
            if (Trace_Activated)
            {
                Param("bitrate_indicator", bitrate_indicator, Count);
                Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
            }
            #endif
        TEST_SB_END();
        if (channel_mode >=122 && channel_mode <= 125)
            Get_SB(add_ch_base,                                 "add_ch_base");

        TEST_SB_SKIP(                                           "b_content_type");
            content_type();
        TEST_SB_END();

        for (int8u Pos=0; Pos<frame_rate_factor; Pos++)
            Skip_SB(                                            "b_iframe");

        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4(2, substream_index32,                        "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_hsf_ext_substream_info()
{
    Element_Begin1(                                             "ac4_hsf_ext_substream_info");
    Get_S1(2, substream_index,                                  "substream_index");
    if (substream_index==3)
    {
        int32u substream_index32;
        Get_V4(2, substream_index32,                            "substream_index");
        substream_index32+=3;
        substream_index=(int8u)substream_index32;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::presentation_config_ext_info()
{
    Element_Begin1(                                             "presentation_config_ext_info");
    int8u n_skip_bytes; // TODO: verify max size
    Get_S1 (5, n_skip_bytes,                                    "n_skip_bytes");
    TEST_SB_SKIP(                                               "b_more_skip_bytes");
        int32u n_skip_bytes32;
        Get_V4 (2, n_skip_bytes32,                              "n_skip_bytes");
        n_skip_bytes32<<5;
        n_skip_bytes32+=n_skip_bytes;
        n_skip_bytes=(int8u)n_skip_bytes32;
    TEST_SB_END();

    BS->Skip(n_skip_bytes * 8);
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            Param("reserved", n_skip_bytes * 8, n_skip_bytes * 8);
            Param_Info(__T("(")+Ztring::ToZtring(n_skip_bytes * 8)+__T(" bits)"));
        }
    #endif //MEDIAINFO_TRACE

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::content_type()
{
    Element_Begin1(                                             "content_type");
        Get_S1(3, content_classifier,                           "content_classifier");
        TEST_SB_SKIP(                                           "b_language_indicator");
            TESTELSE_SB_SKIP(                                   "b_serialized_language_tag");
                Get_SB(b_start_tag,                             "b_start_tag");
                Get_S2(16, language_tag_chunk,                  "language_tag_chunk");
            TESTELSE_SB_ELSE(                                   "b_serialized_language_tag");
                int8u n_language_tag_bytes;
                Get_S1(6, n_language_tag_bytes,                 "n_language_tag_bytes");
                for (int8u Pos=0; Pos<n_language_tag_bytes; Pos++)
                    Skip_S1(8,                                  "language_tag_bytes");
            TESTELSE_SB_END();
        TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::frame_rate_multiply_info()
{
    b_multiplier=false;
    multiplier_bit=false;
    frame_rate_factor=1;
    Element_Begin1(                                             "frame_rate_multiply_info");
    switch (frame_rate_index)
    {
        case 2:
        case 3:
        case 4:
            Get_SB(b_multiplier,                                "b_multiplier");
            if (b_multiplier)
            {
                Get_SB(multiplier_bit,                          "multiplier_bit");
                if (multiplier_bit)
                    frame_rate_factor=4;
                else
                    frame_rate_factor=2;
            }
        break;
        case 0:
        case 1:
        case 7:
        case 8:
        case 9:
            Get_SB(b_multiplier,                                "b_multiplier");
            if (b_multiplier)
                frame_rate_factor=2;
        break;
        default:
        break;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::umd_info()
{
    Element_Begin1(                                             "umd_info");
    Get_S1 (2, umd_version,                                     "umd_version");
    if (umd_version==3)
    {
        int32u umd_version32; 
        Get_V4 (2, umd_version32,                               "umd_version");
        umd_version32+=3;
        umd_version=(int8u)umd_version32;
    }

    Get_S1 (3, key_id,                                          "key_id");
    if (key_id==7)
    {
        int32u key_id32; 
        Get_V4 (3, key_id32,                                   "key_id");
        key_id32+=7;
        key_id=(int8u)key_id32;
    }

    TEST_SB_SKIP(                                               "b_umd_payloads_substream_info");
        umd_payloads_substream_info();
    TEST_SB_END();
    umd_protection();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::umd_payloads_substream_info()
{
    Element_Begin1(                                             "umd_payloads_substream_info");
    Get_S1 (2, substream_index,                                 "substream_index");
    if (substream_index==3)
    {
        int32u substream_index32; 
        Get_V4 (2, substream_index32,                           "substream_index");
        substream_index32+=3;
        substream_index=(int8u)substream_index32;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::umd_protection()
{
    int8u protection_length_primary, protection_length_secondary;
    Element_Begin1(                                             "umd_protection");
    Get_S1(2, protection_length_primary,                        "protection_length_primary");
    Get_S1(2, protection_length_secondary,                      "protection_length_secondary");
    switch(protection_length_primary)
    {
        case 1:
            Skip_S1(8,                                          "protection_bits_primary");
        break;
        case 2:
            Skip_S4(32,                                         "protection_bits_primary");
        break;
        case 3:
        //TODO:One skip
            Skip_S8(64,                                         "protection_bits_primary");
            Skip_S8(64,                                         "protection_bits_primary");
        break;
        default:
        break;
    }

    switch(protection_length_secondary)
    {
        case 1:
            Skip_S1(8,                                          "protection_bits_secondary");
        break;
        case 2:
            Skip_S4(32,                                         "protection_bits_secondary");
        break;
        case 3:
        //TODO: One skip
            Skip_S8(64,                                         "protection_bits_secondary");
            Skip_S8(64,                                         "protection_bits_secondary");
        break;
        default:
        break;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::substream_index_table()
{
    Element_Begin1(                                             "substream_index_table");
    Get_S1(2, n_substreams,                                     "n_substreams");
    if (n_substreams==0)
    {
        int32u n_substreams32;
        Get_V4 (2, n_substreams32,                              "n_substreams");
        n_substreams32+=4;
        n_substreams=(int8u)n_substreams32;
    }

    bool b_size_present;
    if (n_substreams==1)
        Get_SB(b_size_present,                                  "b_size_present");
    else
        b_size_present=1;

    if (b_size_present)
    {
        for (int8u Pos=0; Pos<n_substreams; Pos++)
        {
            bool b_more_bits;
            Get_SB (b_more_bits,                                "b_more_bits");
            Skip_S2 (10,                                        "substream_size");
            if (b_more_bits)
            {
                Skip_V4(2,                                      "substream_size");
            }
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::dac4()
{
    Element_Begin1("ac4_dsi");
    BS_Begin();
    int16u n_presentations;
    int8u ac4_dsi_version;
    Get_S1 (3, ac4_dsi_version,                                 "ac4_dsi_version");
    if (ac4_dsi_version>1)
    {
        Skip_BS(Data_BS_Remain(),                               "Unknown");
        BS_End();
        return;
    }
    Get_S1 (7, bitstream_version,                               "bitstream_version");
    if (bitstream_version>2)
    {
        Skip_BS(Data_BS_Remain(),                               "Unknown");
        BS_End();
        return;
    }
    Get_SB (   fs_index,                                        "fs_index");
    Get_S1 (4, frame_rate_index,                                "frame_rate_index"); Param_Info1(Ac4_frame_rate[fs_index][frame_rate_index]);
    Get_S2 (9, n_presentations,                                 "n_presentations");
    BS_End();
    Element_End0();

    FILLING_BEGIN();
        Accept();
    FILLING_END();
    Element_Offset=Element_Size;
    MustParse_dac4=false;
}

//***************************************************************************
// Parsing
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ac4::Get_V4(int8u  Bits, int32u  &Info, const char* Name)
{
    Info = 0;

    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int8u Count = 0;
            do
            {
                Info += BS->Get4(Bits);
                Count += Bits;
            }
            while (BS->GetB());
            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            do
                Info += BS->Get4(Bits);
            while (BS->GetB());
        }
}

//---------------------------------------------------------------------------
void File_Ac4::Skip_V4(int8u  Bits, const char* Name)
{
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int8u Info = 0;
            int8u Count = 0;
            do
            {
                Info += BS->Get4(Bits);
                Count += Bits;
            }
            while (BS->GetB());
            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            do
                BS->Skip(Bits);
            while (BS->GetB());
        }
}

//---------------------------------------------------------------------------
void File_Ac4::Get_V4(int8u Bits1, int8u Bits2, int8u Bits3, int8u Bits4, int32u  &Info, const char* Name)
{
    Info = 0;

    int8u Temp;
    int8u Count=Bits1;
    Peek_S1(Bits1, Temp);
    if (Temp==2^Bits1-1)
    {
        Count=Bits2;
        Peek_S1(Bits2, Temp);
        if (Temp==2^Bits2-1)
        {
            Count=Bits3;
            Peek_S1(Bits3, Temp);
            if (Temp==2^Bits3-1)
            {
                Count=Bits4;
                Peek_S1(Bits4, Temp);
            }
        }
    }

    Info=(int32u)Temp;
    BS->Skip(Count);
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
    #endif //MEDIAINFO_TRACE
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void File_Ac4::Get_VB(int8u  &Info, const char* Name)
{
    Info = 0;

    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int8u Count=1;
            while (BS->GetB());
            {
                Info++;
                Count++;
            }
            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            while (BS->GetB())
                Info++;
        }
}

//---------------------------------------------------------------------------
void File_Ac4::Skip_VB(const char* Name)
{
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int8u Info=0;
            int8u Count=1;
            while (BS->GetB());
            {
                Info++;
                Count++;
            }
            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            while (BS->GetB());
        }
}


//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ac4::CRC_Compute(size_t Size)
{
    int16u CRC_16=0x0000;
    const int8u* CRC_16_Buffer=Buffer+Buffer_Offset+2; //After sync_word
    const int8u* CRC_16_Buffer_End=Buffer+Buffer_Offset+Size; //End of frame
    while(CRC_16_Buffer<CRC_16_Buffer_End)
    {
        CRC_16=(CRC_16<<8) ^ CRC_16_Table[(CRC_16>>8)^(*CRC_16_Buffer)];
        CRC_16_Buffer++;
    }

    return (CRC_16==0x0000);
}

} //NameSpace

#endif //MEDIAINFO_AC4_YES
