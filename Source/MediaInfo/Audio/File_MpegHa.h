/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_MpegHH
#define MediaInfo_File_MpegHH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File__Tags.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_MpegH
//***************************************************************************

class File_MpegHa : public File__Analyze
{
public :
    //Constructor/Destructor
    File_MpegHa();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Sync();
    void escapedValue(int32u& Value, int8u nBits1, int8u nBits2, int8u nBits3, const char* Name);
};

} //NameSpace

#endif
