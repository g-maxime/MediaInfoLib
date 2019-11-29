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
#if defined(MEDIAINFO_MPEGHA_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_MpegHa.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_MpegHa::File_MpegHa()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_MpegHa::Streams_Fill()
{
    Fill(Stream_Audio, 0, Audio_Format, "MPEG-H 3D Audio");
}

//---------------------------------------------------------------------------
void File_MpegHa::Streams_Finish()
{
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_MpegHa::Header_Parse()
{
    //Parsing
    int32u MHASPacketType, MHASPacketLabel, MHASPacketLength;
    BS_Begin();
    escapedValue(MHASPacketType, 3, 8, 8,                       "MHASPacketType");
    escapedValue(MHASPacketLabel, 2, 8, 32,                     "MHASPacketLabel");
    escapedValue(MHASPacketLength, 11, 24, 24,                  "MHASPacketLength");
    BS_End();

    FILLING_BEGIN();
    Header_Fill_Code(MHASPacketType, Ztring().From_CC3(MHASPacketType));
    Header_Fill_Size(Element_Offset+ MHASPacketLength);
    FILLING_END();
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_MpegHa::Data_Parse()
{
    //Parsing
    switch (Element_Code)
    {
        case  0 : Element_Name("FILLDATA"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case  1 : Element_Name("MPEGH3DACFG"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case  2 : Element_Name("MPEGH3DAFRAME"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case  3 : Element_Name("AUDIOSCENEINFO"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case  6 : Element_Name("SYNC"); Sync(); break;
        case  7 : Element_Name("SYNCGAP"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case  8 : Element_Name("MARKER"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case  9 : Element_Name("CRC16"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 10 : Element_Name("CRC32"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 11 : Element_Name("DESCRIPTOR"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 12 : Element_Name("USERINTERACTION"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 13 : Element_Name("LOUDNESS_DRC"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 14 : Element_Name("BUFFERINFO"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 15 : Element_Name("GLOBAL_CRC16"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 16 : Element_Name("GLOBAL_CRC32"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 17 : Element_Name("AUDIOTRUNCATION"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case 18 : Element_Name("GENDATA"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        default :
                Skip_XX(Element_Size-Element_Offset, "Data");
    }

    FILLING_BEGIN();
        //Filling
        if (!Status[IsAccepted])
            Accept("MPEG-H 3D Audio");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_MpegHa::Sync()
{
    Skip_B1(                                                    "syncword");
}

//---------------------------------------------------------------------------
void File_MpegHa::escapedValue(int32u &Value, int8u nBits1, int8u nBits2, int8u nBits3, const char* Name)
{
    Element_Begin1(Name);
    Get_S4(nBits1, Value,                                       "nBits1");
    if (Value==((1<<nBits1)-1))
    {
        int32u ValueAdd;
        Get_S4(nBits2, ValueAdd,                                "nBits2");
        Value+=ValueAdd;
        if (nBits3 && Value==((1<<nBits2)-1))
        {
            Get_S4(nBits3, ValueAdd,                            "nBits3");
            Value+=ValueAdd;
        }
    }
    Element_Info1(Value);
    Element_End0();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_MPEGHA_YES

