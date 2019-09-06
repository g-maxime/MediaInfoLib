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
#include <cmath>
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

    if (payload_base)
        Skip_BS(payload_base,                                   "fill_area");

    Skip_S1(BS->Remain()%8,                                     "byte_align");

    for (size_t Pos=0; Pos<n_substreams; Pos++)
    {
        size_t Pos_Before, Pos_After;
        switch(Substream_Info[Pos].Substream_Type)
        {
        case Type_Ac4_Substream:
            ac4_substream(Pos);
        break;
//        case Type_Ac4_Hsf_Ext_Substream:
            // TODO: ac4_hsf_ext_substream(); (skip)
//        break;
//        case Type_Emdf_Payloads_Substream:
            // TODO: emdf_payloads_substream(); (skip)
//        break;
          case Type_Ac4_Presentation_Substream:
            ac4_presentation_substream(Pos);
          break;
//        case Type_Oamd_Substream:
//            // TODO: oamd_substream(); (skip)
        default:
            Skip_BS(Substream_Info[Pos].Substream_Size*8,       "substream_data");
        break;
        }
    }
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
    int16u sequence_counter, n_presentations;
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
            Skip_S1(2,                                          "br_code");
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
        Get_S1 (7, payload_base,                                "payload_base_minus1");
        payload_base++;
        if (payload_base==32)
        {
            int32u payload_base32;
            Get_V4(3, payload_base32,                           "payload_base");
            payload_base32+=32;
            payload_base=(int8u)payload_base32;
        }
    TEST_SB_END();

    if (bitstream_version<=1)
    {
        for(int8u Pos=0; Pos<n_presentations; Pos++)
            ac4_presentation_info();
    }
    else
    {
        TEST_SB_SKIP(                                           "b_program_id");
            Skip_S2(16,                                         "short_program_id");
            TEST_SB_SKIP(                                       "b_program_uuid_present");
                Skip_BS(128,                                    "program_uuid");
            TEST_SB_END();
        TEST_SB_END();

        for (int8u Pos=0; Pos<n_presentations; Pos++)
            ac4_presentation_v1_info();

        for (int8u Pos=0; Pos<max_group_index+1; Pos++)
            ac4_substream_group_info();
    }

    substream_index_table();

    Skip_S1(BS->Remain()%8,                                     "byte_align");

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_info()
{
    bool b_single_substream, b_add_emdf_substreams=false;
    int8u presentation_config;

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

    Skip_VB(                                                    "presentation_version");

    if (!b_single_substream && presentation_config==6)
    {
        b_add_emdf_substreams=true;
    }
    else
    {
        bool b_mdcompat;
        Get_SB(b_mdcompat,                                      "b_mdcompat");
        TEST_SB_SKIP(                                           "b_belongs_to_presentation_group");
            Skip_V4(2,                                          "presentation_group");
        TEST_SB_END();

        frame_rate_multiply_info();
        emdf_info();

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
                    ac4_hsf_ext_substream_info(true); // Main HSF
                ac4_substream_info(); // Dialog
            break;
            case 1: // Main + DE
                ac4_substream_info(); // Main
                if (b_hsf_ext)
                    ac4_hsf_ext_substream_info(true); // Main HSF
                ac4_substream_info(); // DE
            break;
            case 2: // Main + Associate
                ac4_substream_info(); // Main
                if (b_hsf_ext)
                    ac4_hsf_ext_substream_info(true); // Main HSF
                ac4_substream_info(); // Associate
            break;
            case 3: // Dry Main + Dialog + Associate
                ac4_substream_info(); // Main
                if (b_hsf_ext)
                    ac4_hsf_ext_substream_info(true); // Main HSF
                ac4_substream_info(); // Dialog
                ac4_substream_info(); // Associate
            break;
            case 4: // Main + DE Associate
                ac4_substream_info(); // Main
                if (b_hsf_ext)
                    ac4_hsf_ext_substream_info(true); // Main HSF
                ac4_substream_info(); // DE
                ac4_substream_info(); // Associate
            break;
            case 5: // Main + HSF ext
                ac4_substream_info(); // Main
                if (b_hsf_ext)
                    ac4_hsf_ext_substream_info(true); // Main HSF
            break;
            default:
                presentation_config_ext_info(presentation_config);
            break;
            }
        }
        Skip_SB(                                                "b_pre_virtualized");
        Get_SB(b_add_emdf_substreams,                           "b_add_emdf_substreams");
    }

    if (b_add_emdf_substreams)
    {
        int8u n_add_emdf_substreams;
        Get_S1 (2, n_add_emdf_substreams,                       "n_add_emdf_substreams");
        if (n_add_emdf_substreams==0)
        {
            int32u n_add_emdf_substreams32;
            Get_V4(2, n_add_emdf_substreams32,                  "n_add_emdf_substreams");
            n_add_emdf_substreams32+=4;
            n_add_emdf_substreams=(int8u)n_add_emdf_substreams32;
        }

        for (int8u Pos=0; Pos<n_add_emdf_substreams; Pos++)
            emdf_info();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_v1_info()
{
    max_group_index=0;
    bool b_single_substream_group, b_add_emdf_substreams=false;
    int8u presentation_config, n_substream_groups, n_substreams_in_presentation;

    Element_Begin1(                                             "ac4_presentation_v1_info");
    Get_SB(b_single_substream_group,                            "b_single_substream_group");
    if (!b_single_substream_group)
    {
        Get_S1(3, presentation_config,                          "presentation_config");
        if (presentation_config==7) {
            int32u presentation_config32;
            Get_V4(2, presentation_config32,                    "presentation_config");
            presentation_config+=presentation_config32;
        }
    }

    if (bitstream_version!=1)
        Skip_VB(                                                "presentation_version");

    if (!b_single_substream_group && presentation_config==6)
    {
        b_add_emdf_substreams=true;
    }
    else
    {
        if (bitstream_version!=1)
            Skip_S1(3,                                          "mdcompat");

        TEST_SB_SKIP(                                           "b_presentation_id");
            Skip_V4(2,                                          "presentation_id");
        TEST_SB_END();

        frame_rate_multiply_info();
        frame_rate_fractions_info();
        emdf_info();

        TEST_SB_SKIP(                                           "b_presentation_filter");
            Skip_SB(                                            "b_enable_presentation");
        TEST_SB_END();

        if (b_single_substream_group)
        {
            ac4_sgi_specifier();
            n_substream_groups=1;
        }
        else
        {
            Skip_SB(                                            "b_multi_pid");
            switch (presentation_config) // TODO: Symplify
            {
            case 0: // Music and Effects + Dialogue
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                n_substream_groups=2;
                n_substreams_in_presentation=2;
            break;
            case 1: // Main + DE
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                n_substream_groups=1;
                n_substreams_in_presentation=2;
            break;
            case 2: // Main + Associated Audio
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                n_substream_groups=2;
                n_substreams_in_presentation=2;
            break;
            case 3: // Music and Effects + Dialogue + Associated Audio
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                n_substream_groups=3;
                n_substreams_in_presentation=3;
            break;
            case 4: // Main + DE + Associated Audio
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                n_substream_groups=2;
                n_substreams_in_presentation=3;
            break;
            case 5: // Arbitrary number of roles and substream groups
                int8u n_substream_groups_minus2;
                Get_S1 (2, n_substream_groups_minus2,           "n_substream_groups_minus2");
                n_substream_groups=n_substream_groups_minus2+2;
                if (n_substream_groups==5)
                {
                    int32u n_substream_groups32;
                    Get_V4 (2, n_substream_groups32,            "n_substream_groups");
                    n_substream_groups32+=5;
                    n_substream_groups=(int8u)n_substream_groups32;
                }

                for (int8u Pos=0; Pos<n_substream_groups; Pos++)
                    ac4_sgi_specifier();

                n_substreams_in_presentation=n_substream_groups;
            break;
            default: // EMDF and other data
                presentation_config_ext_info(presentation_config);
            break;
            }
        }
        Skip_SB(                                                "b_pre_virtualized");
        Get_SB(b_add_emdf_substreams,                           "b_add_emdf_substreams");
        ac4_presentation_substream_info(n_substream_groups, n_substreams_in_presentation);
    }

    if (b_add_emdf_substreams)
    {
        int8u n_add_emdf_substreams;
        Get_S1 (2, n_add_emdf_substreams,                       "n_add_emdf_substreams");
        if (n_add_emdf_substreams==0)
        {
            int32u n_add_emdf_substreams32;
            Get_V4(2, n_add_emdf_substreams32,                  "n_add_emdf_substreams");
            n_add_emdf_substreams32+=4;
            n_add_emdf_substreams=(int8u)n_add_emdf_substreams32;
        }

        for (int8u Pos=0; Pos<n_add_emdf_substreams; Pos++)
            emdf_info();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_sgi_specifier()
{
    Element_Begin1(                                             "ac4_sgi_specifier");
    if (bitstream_version==1)
    {
        ac4_substream_group_info();
    }
    else
    {
        int8u group_index;
        Get_S1(3, group_index,                                  "group_index");
        if (group_index==7)
        {
            int32u group_index32;
            Get_V4(2, group_index32,                            "group_index");
            group_index+=(int8u)max_group_index;
        }
        if (max_group_index<group_index)
            max_group_index=group_index;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info()
{
    int8u channel_mode, substream_index;
    Element_Begin1(                                             "ac4_substream_info");
        int32u channel_mode32;
        Get_V4(1, 2, 4, 7, channel_mode32,                      "channel_mode");
        if (channel_mode32==127)
        {
            Get_V4(2, channel_mode32,                           "channel_mode");
            channel_mode32+=127;
        }
        channel_mode=(int8u)channel_mode32;

        if (fs_index)
        {
            TEST_SB_SKIP(                                       "b_sf_multiplier");
                Skip_SB(                                        "sf_multiplier");
            TEST_SB_END();
        }

        TEST_SB_SKIP(                                           "b_bitrate_info");
            //TODO: move to a function
            int8u bitrate_indicator, Count=3;
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
            Skip_SB(                                            "add_ch_base");

        TEST_SB_SKIP(                                           "b_content_type");
            content_type();
        TEST_SB_END();

        for (int8u Pos=0; Pos<frame_rate_factor; Pos++)
            Skip_SB(                                            "b_iframe");

        Get_S1(2, substream_index,                              "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4(2, substream_index32,                        "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }
        Substream_Info[substream_index].Substream_Type=Type_Ac4_Substream;
        Substream_Info[substream_index].Channel_Mode=channel_mode;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_group_info()
{
    bool b_substreams_present, b_hsf_ext, b_single_substream;
    int8u n_lf_substreams;

    Element_Begin1(                                             "ac4_substream_group_info");
    Get_SB(b_substreams_present,                                "b_substreams_present");
    Get_SB(b_hsf_ext,                                           "b_hsf_ext");
    Get_SB(b_single_substream,                                  "b_single_substream");

    if (b_single_substream)
    {
        n_lf_substreams=1;
    }
    else
    {
        int8u n_lf_substreams_minus2;
        Get_S1(2, n_lf_substreams_minus2,                       "n_lf_substreams_minus2");
        n_lf_substreams=n_lf_substreams_minus2+2;
        if (n_lf_substreams==5)
        {
            int32u n_lf_substreams32;
            Get_V4(2, n_lf_substreams32,                        "n_lf_substreams");
            n_lf_substreams32+=5;
            n_lf_substreams=(int8u)n_lf_substreams32;
        }
    }
    TESTELSE_SB_SKIP(                                           "b_channel_coded");
        for (int8u Pos=0; Pos<n_lf_substreams; Pos++)
        {
            if (bitstream_version==1)
                Skip_SB(                                        "sus_ver"); // TODO: Pass to substream info ?

            ac4_substream_info_chan(b_substreams_present);
            if (b_hsf_ext)
                ac4_hsf_ext_substream_info(b_substreams_present);
        }
    TESTELSE_SB_ELSE(                                           "b_channel_coded");
        TEST_SB_SKIP(                                           "b_oamd_substream");
            oamd_substream_info(b_substreams_present);
        TEST_SB_END();
        for (int8u Pos=0; Pos<n_lf_substreams; Pos++)
        {
            TESTELSE_SB_SKIP(                                   "b_ajoc");
                ac4_substream_info_ajoc(b_substreams_present);
            TESTELSE_SB_ELSE(                                   "b_ajoc");
                ac4_substream_info_obj(b_substreams_present);
            TESTELSE_SB_END();
            if (b_hsf_ext)
                ac4_hsf_ext_substream_info(b_substreams_present);
        }
    TESTELSE_SB_END();

    TEST_SB_SKIP(                                               "content_type");
        content_type();
    TEST_SB_END();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_hsf_ext_substream_info(bool b_substreams_present)
{
    int8u substream_index;
    Element_Begin1(                                             "ac4_hsf_ext_substream_info");
    if (b_substreams_present)
    {
        Get_S1(2, substream_index,                              "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4(2, substream_index32,                        "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }
        Substream_Info[substream_index].Substream_Type=Type_Ac4_Hsf_Ext_Substream;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info_chan(bool b_substreams_present)
{
    int8u substream_index;
    int16u channel_mode;
    Element_Begin1(                                             "ac4_substream_info_chan");
    int32u channel_mode32;
    Get_V4(1, 2, 4, 7, 8, 9, channel_mode32,                    "channel_mode");
    if (channel_mode32==511)
        Skip_V4(2,                                              "channel_mode");
    channel_mode=(int16u)channel_mode32;

    if (channel_mode==252 || channel_mode==253 || channel_mode==508 || channel_mode==509)
    {
        Skip_SB(                                                "b_4_back_channels_present");
        Skip_SB(                                                "b_centre_present");
        Skip_S1(2,                                              "top_channels_present");
    }

    if (fs_index==1)
    {
        TEST_SB_SKIP(                                           "b_sf_multiplier");
            Skip_SB(                                            "sf_multiplier");
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_bitrate_info");
            //TODO: move to a function
            int8u bitrate_indicator, Count=3;
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
    TESTELSE_SB_END();

    if (channel_mode>=122 && channel_mode<=125)
        Skip_SB(                                                "add_ch_base");

    for (int8u Pos=0; Pos<frame_rate_factor; Pos++)
        Skip_SB(                                                "b_audio_ndot");

    Get_S1(2, substream_index,                                  "substream_index");
    if (substream_index==3)
    {
        int32u substream_index32;
        Get_V4(2, substream_index32,                            "substream_index");
        substream_index32+=3;
        substream_index=(int8u)substream_index32;
    }
    Substream_Info[substream_index].Substream_Type=Type_Ac4_Substream;
    Substream_Info[substream_index].Channel_Mode=channel_mode;

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info_ajoc(bool b_substreams_present)
{
    int8u n_fullband_dmx_signals, n_fullband_upmix_signals, substream_index;
    Element_Begin1(                                             "ac4_substream_info_ajoc");
    Skip_SB(                                                    "b_lfe");
    TESTELSE_SB_SKIP(                                           "b_static_dmx");
        n_fullband_dmx_signals=5;
    TESTELSE_SB_ELSE(                                           "b_static_dmx");
        Get_S1(4, n_fullband_dmx_signals,                       "n_fullband_dmx_signals_minus1");
        n_fullband_dmx_signals++;
        bed_dyn_obj_assignment(n_fullband_dmx_signals);
    TESTELSE_SB_END();

    TEST_SB_SKIP(                                               "b_oamd_common_data_present");
        oamd_common_data();
    TEST_SB_END();

    Get_S1(4, n_fullband_upmix_signals,                         "n_fullband_upmix_signals_minus1");
    n_fullband_upmix_signals++;

    if (n_fullband_upmix_signals==16)
    {
        int32u n_fullband_upmix_signals32;
        Get_V4(3, n_fullband_upmix_signals32,                   "n_fullband_upmix_signals");
        n_fullband_upmix_signals32+=16;
        n_fullband_upmix_signals=(int8u)n_fullband_upmix_signals32;
    }

    bed_dyn_obj_assignment(n_fullband_upmix_signals);

    if (fs_index)
    {
        TEST_SB_SKIP(                                           "b_sf_multiplier");
            Skip_SB(                                            "sf_multiplier");
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_bitrate_info");
        //TODO: move to a function
        int8u bitrate_indicator, Count=3;
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

    for (int8u Pos=0; Pos<frame_rate_factor; Pos++)
        Skip_SB(                                                "b_audio_ndot");

    if (b_substreams_present)
    {
        Get_S1(2, substream_index,                              "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4(2, substream_index32,                        "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }
        Substream_Info[substream_index].Substream_Type=Type_Ac4_Substream;
        Substream_Info[substream_index].Sus_Ver=true;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info_obj(bool b_substreams_present)
{
    int8u substream_index;
    Element_Begin1(                                             "ac4_substream_info_obj");
    Skip_S1(3,                                                  "n_objects_code");
    TESTELSE_SB_SKIP(                                           "b_dynamic_objects");
        Skip_SB(                                                "b_lfe");
    TESTELSE_SB_ELSE(                                           "b_dynamic_objects");
        TESTELSE_SB_SKIP(                                       "b_bed_objects");
            TEST_SB_SKIP(                                       "b_bed_start");
                TESTELSE_SB_SKIP(                               "b_ch_assign_code");
                    Skip_S1(3,                                  "bed_chan_assign_code");
                TESTELSE_SB_ELSE(                               "b_ch_assign_code");
                    TESTELSE_SB_SKIP(                           "b_nonstd_bed_channel_assignment");
                        Skip_S3(17,                             "nonstd_bed_channel_assignment_mask");
                    TESTELSE_SB_ELSE(                           "b_nonstd_bed_channel_assignment");
                        Skip_S2(10,                             "std_bed_channel_assignment_mask");
                    TESTELSE_SB_END();
                TESTELSE_SB_END();
            TEST_SB_END();
        TESTELSE_SB_ELSE(                                       "b_bed_objects");
            TESTELSE_SB_SKIP(                                   "b_isf");
                TEST_SB_SKIP(                                   "b_isf_start");
                    Skip_S1(3,                                  "isf_config");
                TEST_SB_END();
            TESTELSE_SB_ELSE(                                   "b_isf");
                int8u res_bytes;
                Get_S1(4, res_bytes,                            "res_bytes");
                if (res_bytes)
                    Skip_S8(res_bytes * 8,                      "reserved_data");
            TESTELSE_SB_END();
       TESTELSE_SB_END();
    TESTELSE_SB_END();

    if (fs_index)
    {
        TEST_SB_SKIP(                                           "b_sf_multiplier");
            Skip_SB(                                            "sf_multiplier");
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_bitrate_info");
        //TODO: move to a function
        int8u bitrate_indicator, Count=3;
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

    for (int8u Pos=0; Pos<frame_rate_factor; Pos++)
        Skip_SB(                                                "b_audio_ndot");

    if (b_substreams_present)
    {
        Get_S1(2, substream_index,                              "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4(2, substream_index32,                        "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }
        Substream_Info[substream_index].Substream_Type=Type_Ac4_Substream;
        Substream_Info[substream_index].Sus_Ver=true;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_substream_info(int8u n_substream_groups, int8u n_substreams_in_presentation)
{
    bool b_alternative, b_pres_ndot;
    int8u substream_index;
    Element_Begin1(                                             "ac4_presentation_substream_info");
    Get_SB(b_alternative,                                       "b_alternative");
    Get_SB(b_pres_ndot,                                         "b_pres_ndot");

    Get_S1 (2, substream_index,                                 "substream_index");
    if (substream_index==3)
    {
        int32u substream_index32;
        Get_V4(2, substream_index32,                            "substream_index");
        substream_index32+=3;
        substream_index=(int8u)substream_index32;
    }
    Substream_Info[substream_index].Substream_Type=Type_Ac4_Presentation_Substream;
    Substream_Info[substream_index].Alternative=b_alternative;
    Substream_Info[substream_index].Pres_Ndot=b_pres_ndot;
    Substream_Info[substream_index].Substream_Groups=n_substream_groups;
    Substream_Info[substream_index].Presentation_Substreams=n_substreams_in_presentation;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::presentation_config_ext_info(int8u presentation_config)
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

    if (bitstream_version==1 && presentation_config==7)
    {
        size_t Pos_Before=Data_BS_Remain();
        ac4_presentation_v1_info();
        size_t Pos_After=Data_BS_Remain();
        size_t n_bits_read=Pos_After-Pos_Before;
        if (n_bits_read%8)
        {
            int8u n_skip_bits=8-(n_bits_read%8);
            Skip_S8(n_skip_bits,                                "reserved");
            n_bits_read+=n_skip_bits;
        }
        n_skip_bytes-=(n_bits_read/8);
    }

    Skip_S8(n_skip_bytes*8,                                     "reserved");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::bed_dyn_obj_assignment(int8u n_signals)
{
    int8u n_bed_signals;
    Element_Begin1(                                             "bed_dyn_obj_assignment");
        TESTELSE_SB_SKIP(                                       "b_dyn_objects_only");
        TESTELSE_SB_ELSE(                                       "b_dyn_objects_only");
            TESTELSE_SB_SKIP(                                   "b_isf");
                Skip_S1(3,                                      "isf_config");
            TESTELSE_SB_ELSE(                                   "b_isf");
                TESTELSE_SB_SKIP(                               "b_ch_assign_code");
                    Skip_S1(3,                                  "bed_chan_assign_code");
                TESTELSE_SB_ELSE(                               "b_ch_assign_code");
                    TESTELSE_SB_SKIP(                           "b_chan_assign_mask");
                        TESTELSE_SB_SKIP(                       "b_nonstd_bed_channel_assignment");
                            Skip_S3(17,                         "nonstd_bed_channel_assignment_mask");
                        TESTELSE_SB_ELSE(                       "b_nonstd_bed_channel_assignment");
                            Skip_S2(10,                         "std_bed_channel_assignment_mask");
                        TEST_SB_END();
                    TESTELSE_SB_ELSE(                           "b_chan_assign_mask");
                        if (n_signals>1)
                        {
                            int8u bed_ch_bits=ceil(log2(n_signals));
                            Get_S1(bed_ch_bits, n_bed_signals,  "n_bed_signals_minus1"); // TODO: suffcient ?
                            n_bed_signals++;
                        }
                        else
                        {
                            n_bed_signals=1;
                        }
                        for (int8u Pos=0; Pos<n_bed_signals; Pos++)
                            Skip_S1(4,                          "nonstd_bed_channel_assignment");
                    TESTELSE_SB_END();
                TESTELSE_SB_END();
            TESTELSE_SB_END();
        TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::content_type()
{
    Element_Begin1(                                             "content_type");
        Skip_S1(3,                                              "content_classifier");
        TEST_SB_SKIP(                                           "b_language_indicator");
            TESTELSE_SB_SKIP(                                   "b_serialized_language_tag");
                Skip_SB(                                        "b_start_tag");
                Skip_S2(16,                                     "language_tag_chunk");
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
    frame_rate_factor=1;
    Element_Begin1(                                             "frame_rate_multiply_info");
    switch (frame_rate_index)
    {
        case 0:
        case 1:
        case 7:
        case 8:
        case 9:
            TEST_SB_SKIP(                                       "b_multiplier");
                frame_rate_factor=2;
            TEST_SB_END();
        break;
        case 2:
        case 3:
        case 4:
            TEST_SB_SKIP(                                       "b_multiplier");
                TESTELSE_SB_SKIP(                               "multiplier_bit");
                    frame_rate_factor=4;
                TESTELSE_SB_ELSE(                               "multiplier_bit");
                    frame_rate_factor=2;
                TESTELSE_SB_END();
            TEST_SB_END();
        break;
        default:
        break;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::frame_rate_fractions_info()
{
    frame_rate_fraction=1;
    Element_Begin1(                                             "frame_rate_fractions_info");
    switch(frame_rate_index)
    {
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            if (frame_rate_factor==1)
            {
                TEST_SB_SKIP(                                   "b_frame_rate_fraction");
                    frame_rate_fraction=2;
                TEST_SB_END();
            }
        break;
        case 10:
        case 11:
        case 12:
            TEST_SB_SKIP(                                       "b_frame_rate_fraction");
                TESTELSE_SB_SKIP(                               "b_frame_rate_fraction_is_4");
                    frame_rate_fraction=4;
                TESTELSE_SB_ELSE(                               "b_frame_rate_fraction_is_4");
                    frame_rate_fraction=2;
                TESTELSE_SB_END();
            TEST_SB_END();
        break;
        default:;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::emdf_info()
{
    int8u emdf_version, key_id;
    Element_Begin1(                                             "emdf_info");
    Get_S1 (2, emdf_version,                                    "emdf_version");
    if (emdf_version==3)
    Skip_V4(2,                                                  "emdf_version");

    Get_S1 (3, key_id,                                          "key_id");
    if (key_id==7)
        Skip_V4(3,                                              "key_id");

    TEST_SB_SKIP(                                               "b_emdf_payloads_substream_info");
        emdf_payloads_substream_info();
    TEST_SB_END();
    emdf_protection();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::emdf_payloads_substream_info()
{
    int8u substream_index;
    Element_Begin1(                                             "emdf_payloads_substream_info");
    Get_S1 (2, substream_index,                                 "substream_index");
    if (substream_index==3)
    {
        int32u substream_index32;
        Get_V4(2, substream_index32,                            "substream_index");
        substream_index32+=3;
        substream_index=(int8u)substream_index32;
    }
    Substream_Info[substream_index].Substream_Type=Type_Emdf_Payloads_Substream;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::emdf_protection()
{
    int8u protection_length_primary, protection_length_secondary;
    Element_Begin1(                                             "emdf_protection");
    Get_S1(2, protection_length_primary,                        "protection_length_primary");
    Get_S1(2, protection_length_secondary,                      "protection_length_secondary");
    switch(protection_length_primary)
    {
        case 1:
            Skip_BS(8,                                          "protection_bits_primary");Param_Info1("(8 bits)");
        break;
        case 2:
            Skip_BS(32,                                         "protection_bits_primary");Param_Info1("(32 bits)");
        break;
        case 3:
            Skip_BS(128,                                         "protection_bits_primary");Param_Info1("(128 bits)");
        break;
        default:;
    }

    switch(protection_length_secondary)
    {
        case 1:
            Skip_BS(8,                                          "protection_bits_secondary");Param_Info1("(8 bits)");
        break;
        case 2:
            Skip_BS(32,                                         "protection_bits_secondary");Param_Info1("(32 bits)");
        break;
        case 3:
            Skip_BS(128,                                        "protection_bits_secondary");Param_Info1("(128 bits)");
        break;
        default:;
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
            int16u substream_size;
            bool b_more_bits;
            Get_SB (b_more_bits,                                "b_more_bits");
            Get_S2 (10, substream_size,                         "substream_size");
            if (b_more_bits)
            {
                int32u substream_size32;
                Get_V4(2, substream_size32,                     "substream_size");
                substream_size=(int16u)substream_size32<<10;
            }
            Substream_Info[Pos].Substream_Size=substream_size;
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::oamd_substream_info(bool b_substreams_present)
{
    int8u substream_index;
    Element_Begin1(                                             "oamd_substream_info");
    Skip_SB(                                                    "b_oamd_ndot");
    if (b_substreams_present)
    {
        Get_S1(2, substream_index,                              "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4(2, substream_index32,                        "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }
        Substream_Info[substream_index].Substream_Type=Type_Oamd_Substream;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::oamd_common_data()
{
    int8u add_data_bytes;
    int16u bits_used;

    Element_Begin1(                                             "oamd_common_data");
    TESTELSE_SB_SKIP(                                           "b_default_screen_size_ratio");
    TESTELSE_SB_ELSE(                                           "b_default_screen_size_ratio");
        Skip_S1(5,                                              "master_screen_size_ratio_code");
    TESTELSE_SB_END();

    Skip_SB(                                                    "b_bed_object_chan_distribute");

    TEST_SB_SKIP(                                               "b_additional_data");
        Get_S1(1, add_data_bytes,                               "add_data_bytes_minus1");
        add_data_bytes++;

        if (add_data_bytes==2)
        {
            int32u add_data_bytes32;
            Get_V4(2, add_data_bytes32,                         "add_data_bytes");
            add_data_bytes+=(int8u)add_data_bytes32;
        }
        //TODO: bits_used=trim();
        //TODO: bits_used+=bed_render_info();
        Skip_S8(add_data_bytes*8,                              "add_data");
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream(size_t Substream_Index)
{
    int32u audio_size;
    size_t Pos_Before=BS->Remain(), Pos_After;

    Element_Begin1("ac4_substream");
    Get_S4(15, audio_size,                                      "audio_size_value");
    TEST_SB_SKIP(                                               "b_more_bits");
        int32u audio_size32;
        Get_V4(7, audio_size32,                                 "audio_size_value");
        audio_size+=audio_size32<<15;
    TEST_SB_END();

    // Skip audio
    Skip_BS((audio_size*8)-(Pos_Before-BS->Remain()),           "audio_data");

    metadata(Substream_Index);

    Pos_After=BS->Remain();

    if (Pos_Before-Pos_After<(Substream_Info[Substream_Index].Substream_Size*8))
        Skip_BS((Substream_Info[Substream_Index].Substream_Size*8)-(Pos_Before-Pos_After), "remaining_substream_data");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_substream(size_t Substream_Index)
{
    int8u name_length=32, n_targets, add_data_bytes;
    Element_Begin1("ac4_presentation_substream");
    if (Substream_Info[Substream_Index].Alternative)
    {
        TEST_SB_SKIP(                                           "b_name_present");
            TEST_SB_SKIP(                                       "b_length");
                Get_S1(5, name_length,                          "name_len");
            TEST_SB_END();
        TEST_SB_END();
        Skip_BS(name_length*8,                                  "presentation_name");

        Get_S1(2, n_targets,                                    "n_targets_minus1");
        n_targets++;
        if (n_targets==4)
        {
            int32u n_targets32;
            Get_V4(2, n_targets32,                              "n_targets");
            n_targets+=(int8u)n_targets32;
        }

        for (int8u Pos=0; Pos<n_targets; Pos++)
        {
            Skip_S1(3,                                          "target_level");
            Skip_S1(4,                                          "target_device_category[]");

            TEST_SB_SKIP(                                       "b_tdc_extension");
                Skip_S1(4,                                      "reserved_bits");
            TEST_SB_END();

            TEST_SB_SKIP(                                       "b_ducking_depth_present");
                Skip_S1(6,                                      "max_ducking_depth");
            TEST_SB_END();

            TEST_SB_SKIP(                                       "b_loud_corr_target");
                Skip_S1(5,                                      "loud_corr_target");
            TEST_SB_END();

            for (int8u Pos2=0; Pos<Substream_Info[Substream_Index].Presentation_Substreams; Pos++)
            {
                TEST_SB_SKIP(                                    "b_active");
                    TEST_SB_SKIP(                                "alt_data_set_index");
                        Skip_V4(2,                               "alt_data_set_index");
                    TEST_SB_END();
                TEST_SB_END();
            }
        }
    }

    TEST_SB_SKIP(                                               "b_additional_data");
        Get_S1(4, add_data_bytes,                               "add_data_bytes");
        add_data_bytes++;
        if (add_data_bytes==16)
        {
            int32u add_data_bytes32;
            Get_V4(2, add_data_bytes32,                         "add_data_bytes32");
        }
        Skip_S1(BS->Remain()%8,                                 "byte_align");
        Skip_BS(add_data_bytes*8,                               "add_data");
    TEST_SB_END();

    Skip_S1(7,                                                  "dialnorm_bits");
    TEST_SB_SKIP(                                               "b_further_loudness_info");
        further_loudness_info(1, 1);
    TEST_SB_END();

    Skip_S1(5,                                                   "drc_metadata_size_value");
    TEST_SB_SKIP(                                                "b_more_bits");
        Skip_V4(3,                                               "drc_metadata_size_value");
    TEST_SB_END();

    drc_frame(Substream_Info[Substream_Index].Pres_Ndot);

    if (Substream_Info[Substream_Index].Substream_Groups)
    {
        TEST_SB_SKIP(                                           "b_substream_group_gains_present");
            TESTELSE_SB_SKIP(                                   "b_keep");
            TESTELSE_SB_ELSE(                                   "b_keep");
                for (int8u Pos=0; Pos<Substream_Info[Substream_Index].Substream_Groups; Pos++)
                    Skip_S1(6,                                  "sg_gain[sg]");
            TESTELSE_SB_END();
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_associated");
        TEST_SB_SKIP(                                           "b_scale_main");
            Skip_S1(8,                                          "scale_main");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_scale_main_centre");
            Skip_S1(8,                                          "scale_main_centre");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_scale_main_front");
            Skip_S1(8,                                          "scale_main_front");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_associate_is_mono");
            Skip_S1(8,                                          "pan_associated");
        TEST_SB_END();
    TEST_SB_END();

    // TODO: custom_dmx_data(pres_ch_mode, pres_ch_mode_core, b_pres_4_back_channels_present,pres_top_channel_pairs, b_pres_has_lfe);
    // TODO: loud_corr(pres_ch_mode, pres_ch_mode_core, b_objects);
    Skip_S1(BS->Remain()%8,                                     "byte_align");
    Element_End0();
}
//---------------------------------------------------------------------------
void File_Ac4::metadata(size_t Substream_Index)
{
    Element_Begin1("metadata");
    basic_metadata(Substream_Info[Substream_Index].Channel_Mode, Substream_Info[Substream_Index].Sus_Ver);
    extended_metadata(Substream_Info[Substream_Index].Channel_Mode, Substream_Info[Substream_Index].Sus_Ver);
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::basic_metadata(int16u channel_mode, bool sus_ver)
{
    int8u ch_mode=Channel_Mode_to_Ch_Mode(channel_mode);
    Element_Begin1("basic_metadata");
    if (!sus_ver)
        Skip_S1(7,                                              "dialnorm_bits");

    TEST_SB_SKIP(                                               "b_more_basic_metadata");
        if (!sus_ver)
        {
            TEST_SB_SKIP(                                       "b_further_loudness_info");
                further_loudness_info(sus_ver, false);
            TEST_SB_END();
        }
        else
        {
            TEST_SB_SKIP(                                       "b_substream_loudness_info");
                Skip_S1(8,                                      "substream_loudness_bits");
                TEST_SB_SKIP(                                   "b_further_substream_loudness_info");
                    further_loudness_info(sus_ver, false);
                TEST_SB_END();
            TEST_SB_END();
        }

        if (ch_mode==1) // stereo
        {
            TEST_SB_SKIP(                                       "b_prev_dmx_info");
                Skip_S1(3,                                      "pre_dmixtyp_2ch");
                Skip_S1(2,                                      "phase90_info_2ch");
            TEST_SB_END();
        }
        else if (ch_mode>1)
        {
            if (sus_ver)
            {
                TEST_SB_SKIP(                                   "b_stereo_dmx_coeff");
                    Skip_S1(3,                                  "loro_centre_mixgain");
                    Skip_S1(3,                                  "loro_surround_mixgain");

                    TEST_SB_SKIP(                               "b_loro_dmx_loud_corr");
                        Skip_S1(5,                              "loro_dmx_loud_corr");
                    TEST_SB_END();

                    TEST_SB_SKIP(                               "b_ltrt_mixinfo");
                        Skip_S1(3,                              "ltrt_centre_mixgain");
                        Skip_S1(3,                              "ltrt_surround_mixgain");
                    TEST_SB_END();

                    TEST_SB_SKIP(                               "b_ltrt_dmx_loud_corr");
                        Skip_S1(5,                              "ltrt_dmx_loud_corr");
                    TEST_SB_END();

                    if (Channel_Mode_Contains_Lfe(channel_mode))
                    {
                        TEST_SB_SKIP(                           "b_lfe_mixinfo");
                            Skip_S1(5,                          "lfe_mixgain");
                        TEST_SB_END();
                    }

                    Skip_S1(2,                                  "preferred_dmx_method");
                TEST_SB_END();
            }

            if (ch_mode==3 || ch_mode==4) // 5.x
            {
                TEST_SB_SKIP(                                   "b_predmixtyp_5ch");
                    Skip_S1(3,                                  "pre_dmixtyp_5ch");
                TEST_SB_END();

                TEST_SB_SKIP(                                   "b_preupmixtyp_5ch");
                    Skip_S1(4,                                  "pre_upmixtyp_5ch");
                TEST_SB_END();
            }

            if (ch_mode>=5 && ch_mode<=12)
            {
                TEST_SB_SKIP(                                   "b_upmixtyp_7ch");
                    if (ch_mode==5) // TODO: include 3/2/2.1 and 3/4/0.1 ?
                        Skip_S1(2,                              "pre_upmixtyp_3_4");
                    else if (ch_mode==9)
                        Skip_SB(                                "pre_upmixtyp_3_2_2");
                    
                TEST_SB_END();
            }
            Skip_S1(2,                                          "phase90_info_mc");
            Skip_SB(                                            "b_surround_attenuation_known");
            Skip_SB(                                            "b_lfe_attenuation_known");
        }

        TEST_SB_SKIP(                                           "b_dc_blocking");
            Skip_SB(                                            "dc_block_on");
        TEST_SB_END();
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::extended_metadata(int16u channel_mode, bool sus_ver)
{
    int8u ch_mode=Channel_Mode_to_Ch_Mode(channel_mode);
    bool b_associated=false; // TODO: get b_associated value for stream
    bool b_dialog=false;
    Element_Begin1("extended_metadata");
    if (sus_ver)
    {
        Get_SB(b_dialog,                                        "b_dialog");
    }
    else if (b_associated)
    {
        TEST_SB_SKIP(                                           "b_scale_main");
            Skip_S1(8,                                          "scale_main");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_scale_main_centre");
            Skip_S1(8,                                          "scale_main_centre");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_scale_main_front");
            Skip_S1(8,                                          "scale_main_front");
        TEST_SB_END();

        if (ch_mode==0)
            Skip_S1(8,                                          "pan_associated");
    }

    if (b_dialog)
    {
        TEST_SB_SKIP(                                           "b_dialog_max_gain");
            Skip_S1(2,                                          "dialog_max_gain");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_pan_dialog_present");
            if (ch_mode==0)
            {
                Skip_S1(8,                                      "pan_dialog");
            }
            else
            {
                Skip_S1(8,                                      "pan_dialog[0]");
                Skip_S1(8,                                      "pan_dialog[1]");
                Skip_S1(2,                                      "pan_signal_selector");
            }
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_channels_classifier");
        if (Channel_Mode_Contains_C(channel_mode))
        {
            TEST_SB_SKIP(                                       "b_c_active");
                Skip_SB(                                        "b_c_has_dialog");
            TEST_SB_END();
        }
        if (Channel_Mode_Contains_Lr(channel_mode))
        {
            TEST_SB_SKIP(                                       "b_l_active");
                Skip_SB(                                        "b_l_has_dialog");
            TEST_SB_END();

            TEST_SB_SKIP(                                       "b_r_active");
                Skip_SB(                                        "b_r_has_dialog");
            TEST_SB_END();
        }
        if (Channel_Mode_Contains_LsRs(channel_mode))
        {
            Skip_SB(                                            "b_ls_active");
            Skip_SB(                                            "b_rs_active");
        }
        if (Channel_Mode_Contains_LrsRrs(channel_mode))
        {
            Skip_SB(                                            "b_lrs_active");
            Skip_SB(                                            "b_rrs_active");
        }
        if (Channel_Mode_Contains_LwRw(channel_mode))
        {
            Skip_SB(                                            "b_lw_active");
            Skip_SB(                                            "b_rw_active");
        }
        if (Channel_Mode_Contains_VhlVhr(channel_mode))
        {
            Skip_SB(                                            "b_vhl_active");
            Skip_SB(                                            "b_vhr_active");
        }
        if (Channel_Mode_Contains_Lfe(channel_mode))
        {
            Skip_SB(                                            "b_lfe_active");
        }
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_event_probability");
        Skip_S1(4,                                              "event_probability");
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_frame(bool b_iframe)
{
    int8u drc_decoder_nr_modes=0; // TODO: global level ?
    std::map<int8u, int8u> decoder_ids;
    std::map<int8u, drc_decoder_config_infos> decoder_infos;

    Element_Begin1("drc_frame");
        TEST_SB_SKIP(                                           "b_drc_present");
            if (b_iframe)
                drc_config(drc_decoder_nr_modes, decoder_ids, decoder_infos);

            drc_data(drc_decoder_nr_modes, decoder_ids, decoder_infos);
        TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_config(int8u& drc_decoder_nr_modes, std::map<int8u, int8u>& decoder_ids, std::map<int8u, drc_decoder_config_infos>& decoder_infos)
{
    Element_Begin1("drc_config");
        Get_S1(3, drc_decoder_nr_modes,                         "drc_decoder_nr_modes");
        for (int8u Pos=0; Pos<=drc_decoder_nr_modes; Pos++)
            drc_decoder_mode_config(Pos, decoder_ids, decoder_infos);

        Skip_S1(3,                                              "drc_eac3_profile");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_data(int8u drc_decoder_nr_modes, std::map<int8u, int8u> decoder_ids, std::map<int8u, drc_decoder_config_infos> decoder_infos)
{
    bool curve_present=false;
    int16u drc_gainset_size;
    int8u drc_version;
    size_t Remain_Before, used_bits=0;

    Element_Begin1("drc_data");
    for (int8u Pos=0; Pos<=drc_decoder_nr_modes; Pos++)
    {
        if (!decoder_infos[decoder_ids[Pos]].Compression_Curve_Flag)
        {
            Get_S2(6, drc_gainset_size,                         "drc_gainset_size");
            TEST_SB_SKIP(                                       "b_more_bits");
                int32u drc_gainset_size32;
                Get_V4(2, drc_gainset_size32,                   "drc_gainset_size");
                drc_gainset_size+=(int16u)drc_gainset_size32<<6;
            TEST_SB_END();
            Get_S1(2, drc_version,                              "drc_version");

            if (drc_version<=1)
            {
                Remain_Before=BS->Remain();
                drc_gains(decoder_ids[Pos]);
                used_bits=Remain_Before-BS->Remain();
            }

            if (drc_version>=1)
            {
                Skip_BS(drc_gainset_size-2-used_bits,           "drc2_bits");
            }
        }
        else
        {
            curve_present=true;
        }
    }

    if (curve_present)
    {
        Skip_SB(                                                "drc_reset_flag");
        Skip_S1(2,                                              "drc_reserved");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_gains(int8u drc_decoder_mode_id)
{
    Element_Begin1("drc_gains");
    // TODO:
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_decoder_mode_config(int8u Index, std::map<int8u, int8u>& decoder_ids, std::map<int8u, drc_decoder_config_infos>& decoder_infos)
{
    int8u drc_decoder_mode_id, drc_gains_config;
    bool drc_compression_curve_flag=false;

    Element_Begin1("drc_decoder_mode_config");
    Get_S1(3, drc_decoder_mode_id,                              "drc_decoder_mode_id[pcount]");
    if (drc_decoder_mode_id>3)
    {
        Skip_S1(5,                                              "drc_output_level_from");
        Skip_S1(5,                                              "drc_output_level_to");
    }

    TESTELSE_SB_SKIP(                                           "drc_repeat_profile_flag");
        Skip_S1(3,                                              "drc_repeat_id");
        drc_compression_curve_flag=true;
    TESTELSE_SB_ELSE(                                           "drc_repeat_profile_flag");
        TESTELSE_SB_SKIP(                                       "drc_default_profile_flag");
            drc_compression_curve_flag=true;
        TESTELSE_SB_ELSE(                                       "drc_default_profile_flag");

            Get_SB(drc_compression_curve_flag,                  "drc_compression_curve_flag[drc_decoder_mode_id[pcount]]");
            if (drc_compression_curve_flag)
                drc_compression_curve();
            else
                Skip_S1(2,                                      "drc_gains_config[drc_decoder_mode_id[pcount]]");
        TESTELSE_SB_END();
    TESTELSE_SB_END();
    Element_End0();

    decoder_ids[Index]=drc_decoder_mode_id;
    decoder_infos[drc_decoder_mode_id].Compression_Curve_Flag=drc_compression_curve_flag;
    decoder_infos[drc_decoder_mode_id].Gains_Config=drc_gains_config;
};

//---------------------------------------------------------------------------
void File_Ac4::drc_compression_curve()
{
    int8u drc_gain_max_boost, drc_gain_max_cut;
    Element_Begin1("drc_compression_curve");
    Skip_S1(4,                                                  "drc_lev_nullband_low");
    Skip_S1(4,                                                  "drc_lev_nullband_high");

    Get_S1(4, drc_gain_max_boost,                               "drc_gain_max_boost");
    if (drc_gain_max_boost)
    {
        Skip_S1(5,                                              "drc_lev_max_boost");
        TEST_SB_SKIP(                                           "drc_nr_boost_sections");
            Skip_S1(4,                                          "drc_gain_section_boost");
            Skip_S1(5,                                          "drc_lev_section_boost");
        TEST_SB_END();
    }

    Get_S1(4, drc_gain_max_cut,                                 "drc_gain_max_cut");
    if (drc_gain_max_cut)
    {
        Skip_S1(6,                                              "drc_lev_max_cut");
        TEST_SB_SKIP(                                           "drc_nr_cut_sections");
            Skip_S1(5,                                          "drc_gain_section_cut");
            Skip_S1(5,                                          "drc_lev_section_cut");
        TEST_SB_END();
    }

    TESTELSE_SB_SKIP(                                           "drc_tc_default_flag");
    TESTELSE_SB_ELSE(                                           "drc_tc_default_flag");
        Skip_S1(8,                                              "drc_tc_attack");
        Skip_S1(8,                                              "drc_tc_release");
        Skip_S1(8,                                              "drc_tc_attack_fast");
        Skip_S1(8,                                              "drc_tc_release_fast");

        TEST_SB_SKIP(                                           "drc_adaptive_smoothing_flag");
            Skip_S1(5,                                          "drc_attack_threshold");
            Skip_S1(5,                                          "drc_release_threshold");
        TEST_SB_END();
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::further_loudness_info(bool sus_ver, bool b_presentation_ldn)
{
    int8u loudness_version, loud_prac_type, e_bits_size; // TODO: check if size is sufficient
    Element_Begin1("further_loudness_info");
    if (b_presentation_ldn || !sus_ver)
    {
        Get_S1(2, loudness_version,                             "loudness_version");
        if (loudness_version==3)
            Skip_S1(4,                                          "extended_loudness_version");

        Get_S1(4, loud_prac_type,                               "loud_prac_type");
        if (loud_prac_type)
        {
            TEST_SB_SKIP(                                       "b_loudcorr_dialgate");
                Skip_S1(3,                                      "dialgate_prac_type");
            TEST_SB_END();
            Skip_SB(                                            "b_loudcorr_type");
        }
    }
    else
    {
        Skip_SB(                                                "b_loudcorr_dialgate");
    }

    TEST_SB_SKIP(                                               "b_loudrelgat");
        Skip_S2(11,                                             "loudrelgat");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_loudspchgat");
        Skip_S2(11,                                             "loudspchgat");
        Skip_S1(3,                                              "dialgate_prac_type");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_loudstrm3s");
        Skip_S2(11,                                             "loudstrm3s");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_max_loudstrm3s");
        Skip_S2(11,                                             "max_loudstrm3s");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_truepk");
        Skip_S2(11,                                             "truepk");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_max_truepk");
        Skip_S2(11,                                             "max_truepk");
    TEST_SB_END();

    if (b_presentation_ldn || !sus_ver)
    {
        TEST_SB_SKIP(                                           "b_prgmbndy");
            bool prgmbndy_bit=false;
            int8u test;
            Peek_S1(8, test);
            while(!prgmbndy_bit) // TODO: skip all bits in one time
                Get_SB(prgmbndy_bit,                            "prgmbndy_bit");

            Skip_SB(                                            "b_end_or_start");
            TEST_SB_SKIP(                                       "b_prgmbndy_offset");
                Skip_S2(11,                                     "prgmbndy_offset");
            TEST_SB_END();
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_lra");
        Skip_S2(10,                                             "lra");
        Skip_S1(3,                                              "lra_prac_type");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_loudmntry");
        Skip_S2(11,                                             "loudmntry");
    TEST_SB_END();

    if (sus_ver)
    {
        TEST_SB_SKIP(                                           "b_rtllcomp");
            Skip_S1(8,                                          "rtllcomp");
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_extension");
        Get_S1(5, e_bits_size,                                  "e_bits_size");
        if (e_bits_size==31)
        {
            int32u e_bits_size32;
            Get_V4(4, e_bits_size32,                            "e_bits_size");
            e_bits_size32+=31;
            e_bits_size=(int8u)e_bits_size32;
        }

        if (!sus_ver)
        {
            e_bits_size--;
           TEST_SB_SKIP(                                        "b_rtllcomp");
                e_bits_size-=8;
                Skip_S1(8,                                      "rtll_comp");
            TEST_SB_END();
        }
        Skip_BS(e_bits_size,                                    "extensions_bits");
    TEST_SB_END();
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
            {
                Info += BS->Get4(Bits);
                Count += Bits;
            }
            while (BS->GetB());

    /*
            bool More;
            do
            {
                Info += BS->Get4(Bits);
                More=BS->GetB();
                Count += Bits;
                if (More)
                {
                    Info<<=Bits;
                    Info+=(1<<Bits);
                }
            }
            while(More);
    */

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
    if (Temp==~(~0u<<Bits1))
    {
        Count=Bits2;
        Peek_S1(Bits2, Temp);
        if (Temp==~(~0u<<Bits2))
        {
            Count=Bits3;
            Peek_S1(Bits3, Temp);
            if (Temp==~(~0u<<Bits3))
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
void File_Ac4::Get_V4(int8u Bits1, int8u Bits2, int8u Bits3, int8u Bits4, int8u Bits5, int8u Bits6, int32u  &Info, const char* Name)
{
    // TODO: better code
    Info = 0;

    int16u Temp;
    int8u Count=Bits1;

    Peek_S2(Bits1, Temp);
    if (Temp==~(~0u<<Bits1))
    {
        Count=Bits2;
        Peek_S2(Bits2, Temp);
        if (Temp==~(~0u<<Bits2))
        {
            Count=Bits3;
            Peek_S2(Bits3, Temp);
            if (Temp==~(~0u<<Bits3))
            {
                Count=Bits4;
                Peek_S2(Bits4, Temp);
                if (Temp==~(~0u<<Bits4))
                {
                    Count=Bits5;
                    Peek_S2(Bits5, Temp);

                    if (Temp==~(~0u<<Bits5))
                    {
                        Count=Bits6;
                        Peek_S2(Bits6, Temp);
                    }
                }
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
            int8u Count=0;
            do
            {
                Info++;
                Count++;
            }
            while (BS->GetB());
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
            int8u Count=0;
            do
            {
                Info++;
                Count++;
            }
            while (BS->GetB());
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

//---------------------------------------------------------------------------
int8u File_Ac4::Channel_Mode_to_Ch_Mode(int16u Channel_Mode)
{
    switch (Channel_Mode)
    {
        case 0:
            return 0;
        case 2:
            return 1;
        case 12:
            return 2;
        case 13:
            return 3;
        case 14:
            return 4;
        case 120:
            return 5;
        case 121:
            return 6;
        case 122:
            return 7;
        case 123:
            return 8;
        case 124:
            return 9;
        case 125:
            return 10;
        case 252:
            return 11;
        case 253:
            return 12;
        case 508:
            return 13;
        case 509:
            return 14;
        case 510:
            return 15;
        default:
            return 16;
    }
}

//---------------------------------------------------------------------------
bool File_Ac4::Channel_Mode_Contains_Lfe(int16u Channel_Mode)
{
    int8u ch_mode=Channel_Mode_to_Ch_Mode(Channel_Mode);
    if (ch_mode==4 || ch_mode==6 || ch_mode==8 || ch_mode==10 || ch_mode==12 || ch_mode==14 || ch_mode==15)
        return true;

    return false;
}

//---------------------------------------------------------------------------
bool File_Ac4::Channel_Mode_Contains_C(int16u Channel_Mode)
{
    int8u ch_mode=Channel_Mode_to_Ch_Mode(Channel_Mode);
    if (ch_mode==0 || (ch_mode>=2 && ch_mode<=15))
        return true;

    return false;
}

//---------------------------------------------------------------------------
bool File_Ac4::Channel_Mode_Contains_Lr(int16u Channel_Mode)
{
    int8u ch_mode=Channel_Mode_to_Ch_Mode(Channel_Mode);
    if (ch_mode>=1 && ch_mode<=15)
        return true;

    return false;
}

//---------------------------------------------------------------------------
bool File_Ac4::Channel_Mode_Contains_LsRs(int16u Channel_Mode)
{
    int8u ch_mode=Channel_Mode_to_Ch_Mode(Channel_Mode);
    if (ch_mode>=3 && ch_mode<=15)
        return true;

    return false;
}

//---------------------------------------------------------------------------
bool File_Ac4::Channel_Mode_Contains_LrsRrs(int16u Channel_Mode)
{
    int8u ch_mode=Channel_Mode_to_Ch_Mode(Channel_Mode);
    if (ch_mode==5 || ch_mode==6 || (ch_mode>=11 && ch_mode<=15))
        return true;

    return false;
}

//---------------------------------------------------------------------------
bool File_Ac4::Channel_Mode_Contains_LwRw(int16u Channel_Mode)
{
    int8u ch_mode=Channel_Mode_to_Ch_Mode(Channel_Mode);
    if (ch_mode==7 || ch_mode==8 || ch_mode==15)
        return true;

    return false;
}

//---------------------------------------------------------------------------
bool File_Ac4::Channel_Mode_Contains_VhlVhr(int16u Channel_Mode)
{
    int8u ch_mode=Channel_Mode_to_Ch_Mode(Channel_Mode);
    if (ch_mode==9 || ch_mode==10)
        return true;

    return false;
}

} //NameSpace

#endif //MEDIAINFO_AC4_YES
