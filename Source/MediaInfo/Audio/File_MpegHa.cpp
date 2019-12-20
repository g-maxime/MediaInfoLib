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
#include <cmath>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_MpegHa::File_MpegHa()
:File_Usac()
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
        case  1 :
            Element_Name("MPEGH3DACFG");
            BS_Begin();
            mpegh3daConfig();
            BS_End();
            Skip_XX(Element_Size-Element_Offset, "BUGGY"); //TODO: Data
            break;
        case  2 : Element_Name("MPEGH3DAFRAME"); Skip_XX(Element_Size-Element_Offset, "Data"); break;
        case  3 :
            Element_Name("AUDIOSCENEINFO");
            BS_Begin();
            mae_AudioSceneInfo();
            BS_End();
            Skip_XX(Element_Size-Element_Offset, "Data");
            break;
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

/*
//***************************************************************************
// Duplicated USAC functions
// TODO: move to shared USAC class
//***************************************************************************

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

//---------------------------------------------------------------------------
void File_MpegHa::SbrConfig()
{
    Element_Begin1("SbrConfig");

    Skip_SB(                                                    "harmonicsSBR");
    Skip_SB(                                                    "bs_interTes");
    Skip_SB(                                                    "bs_pvc");
    SbrDlftHeader();

    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::SbrDlftHeader()
{
    Element_Begin1("SbrDlftHeader");

    bool dflt_header_extra1, dflt_header_extra2;
    Skip_S1(4,                                                  "dflt_start_freq");
    Skip_S1(4,                                                  "dflt_stop_freq");
    Get_SB (   dflt_header_extra1,                              "dflt_header_extra1");
    Get_SB (   dflt_header_extra2,                              "dflt_header_extra2");
    if (dflt_header_extra1)
    {
        Skip_S1(2,                                              "dflt_freq_scale");
        Skip_SB(                                                "dflt_alter_scale");
        Skip_S1(2,                                              "dflt_noise_bands");
    }
    if (dflt_header_extra2)
    {
        Skip_S1(2,                                              "dflt_limiter_bands");
        Skip_S1(2,                                              "dflt_limiter_gains");
        Skip_SB(                                                "dflt_interpol_freq");
        Skip_SB(                                                "dflt_smoothing_mode");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::Mps212Config(int8u StereoConfigindex)
{
    Element_Begin1("Mps212Config");

    int8u bsTempShapeConfig;
    bool bsOttBandsPhasePresent;
    Skip_S1(3,                                                  "bsFreqRes");
    Skip_S1(3,                                                  "bsFixedGainDMX");
    Get_S1 (2, bsTempShapeConfig,                               "bsTempShapeConfig");
    Skip_S1(2,                                                  "bsDecorrConfig");
    Skip_SB(                                                    "bsHighRatelMode");
    Skip_SB(                                                    "bsPhaseCoding");
    Get_SB (   bsOttBandsPhasePresent,                          "bsOttBandsPhasePresent");
    if (bsOttBandsPhasePresent)
    {
        Skip_S1(5,                                              "bsOttBandsPhase");
    }
    if (StereoConfigindex>1)
    {
        Skip_S1(5,                                              "bsResidualBands");
        Skip_SB(                                                "bSPseudor");
    }
    if (bsTempShapeConfig==2)
    {
        Skip_SB(                                                "bSEnvOuantMode");
    }

    Element_End0();
}
*/
//***************************************************************************
// MPEGH3DACFG
//***************************************************************************

//---------------------------------------------------------------------------
void File_MpegHa::mpegh3daConfig()
{
    int8u usacSamplingFrequencyIndex;
    Element_Begin1("mpegh3daConfig");
    Skip_S1(8,                                                  "mpegh3daProfileLevelIndication");
    Get_S1( 5, usacSamplingFrequencyIndex,                      "usacSamplingFrequencyIndex");
    if (usacSamplingFrequencyIndex==0x1f)
        Skip_S3(24,                                             "usacSamplingFrequency");
    Skip_S1(3,                                                  "coreSbrFrameLengthIndex");
    Skip_SB(                                                    "cfg_reserved");
    Skip_SB(                                                    "receiverDelayCompensation");
    SpeakerConfig3d(DefaultLayout);
    FrameworkConfig3d();
    mpegh3daDecoderConfig();
    TEST_SB_SKIP(                                               "usacConfigExtensionPresent");
        mpegh3daConfigExtension();
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::SpeakerConfig3d(speaker_layout& Layout)
{
    int8u speakerLayoutType;
    Element_Begin1("SpeakerConfig3d");
    Get_S1(2, speakerLayoutType,                                "speakerLayoutType");
    if (speakerLayoutType==0)
    {
        int8u CICPspeakerLayoutIdx;
        Get_S1(6, CICPspeakerLayoutIdx,                         "CICPspeakerLayoutIdx");
        //TODO: Import layout from IEC 23001-8
    }
    else
    {
        int32u numSpeakers;
        escapedValue(numSpeakers, 5, 8, 16,                     "numSpeakers");
        numSpeakers++;
        Layout.numSpeakers=numSpeakers;

        if (speakerLayoutType==1)
        {
            for (size_t Pos=0; Pos<numSpeakers; Pos++)
            {
                int8u CICPspeakerIdx;
                Get_S1(7, CICPspeakerIdx,                       "CICPspeakerIdx");
                //TODO: Import layout from IEC 23001-8
            }
        }
        else if (speakerLayoutType==2)
        {
            mpegh3daFlexibleSpeakerConfig(Layout);
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mpegh3daFlexibleSpeakerConfig(speaker_layout& Layout)
{
    bool angularPrecision;
    Element_Begin1("mpegh3daFlexibleSpeakerConfig");
    Get_SB(angularPrecision,                                    "angularPrecision");
    for (size_t Pos=0; Pos<Layout.numSpeakers; Pos++)
    {
        Layout.SpeakersInfo.push_back(speaker_info(angularPrecision));
        mpegh3daSpeakerDescription(Layout);
        if (Layout.SpeakersInfo.back().AzimuthAngle && Layout.SpeakersInfo.back().AzimuthAngle!=180)
            Skip_SB(                                            "alsoAddSymmetricPair");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mpegh3daSpeakerDescription(speaker_layout& Layout)
{
    Element_Begin1("mpegh3daSpeakerDescription");
    TESTELSE_SB_SKIP(                                           "isCICPspeakerIdx");
        Skip_S1(7,                                              "CICPspeakerIdx");
    TESTELSE_SB_ELSE(                                           "isCICPspeakerIdx");
        int8u ElevationClass;
        Get_S1(2, ElevationClass,                               "ElevationClass");

        switch (ElevationClass)
        {
        case 0:
            Layout.SpeakersInfo.back().ElevationAngle=0;
            break;
        case 1:
            Layout.SpeakersInfo.back().ElevationAngle=35;
            break;
        case 2:
            Layout.SpeakersInfo.back().ElevationAngle=15;
            Layout.SpeakersInfo.back().ElevationDirection=true;
            break;
        case 3:
            int8u ElevationAngleIdx;
            Get_S1(Layout.SpeakersInfo.back().angularPrecision?7:5, ElevationAngleIdx, "ElevationAngleIdx");
            Layout.SpeakersInfo.back().ElevationAngle=ElevationAngleIdx*Layout.SpeakersInfo.back().angularPrecision?1:5;

            if (Layout.SpeakersInfo.back().ElevationAngle)
                Get_SB(Layout.SpeakersInfo.back().ElevationDirection, "ElevationDirection");
            break;
        }

        int8u AzimuthAngleIdx;
        Get_S1(Layout.SpeakersInfo.back().angularPrecision?8:6, AzimuthAngleIdx, "AzimuthAngleIdx");
        Layout.SpeakersInfo.back().AzimuthAngle=AzimuthAngleIdx*Layout.SpeakersInfo.back().angularPrecision?1:5;

        if (Layout.SpeakersInfo.back().AzimuthAngle && Layout.SpeakersInfo.back().AzimuthAngle!=180)
            Get_SB(Layout.SpeakersInfo.back().AzimuthDirection, "AzimuthDirection");

        Get_SB(Layout.SpeakersInfo.back().isLFE,                "isLFE");
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::FrameworkConfig3d()
{
    numAudioChannels=0;
    numAudioObjects=0;
    numSAOCTransportChannels=0;
    numHOATransportChannels=0;

    Element_Begin1("FrameworkConfig3d");
    Element_Begin1("Signals3d");
    Get_S1(5, bsNumSignalGroups,                                "bsNumSignalGroups");
    for (int8u Pos=0; Pos<bsNumSignalGroups+1; Pos++)
    {
        int8u signalGroupType;
        Get_S1(3, signalGroupType,                              "signalGroupType");

        int32u bsNumberOfSignals;
        escapedValue(bsNumberOfSignals, 5, 8, 16,               "bsNumberOfSignals");

        if (signalGroupType==SignalGroupTypeChannels)
        {
            numAudioChannels+=bsNumberOfSignals+1;
            TEST_SB_SKIP(                                       "differsFromReferenceLayout");
                speaker_layout Layout;
                SpeakerConfig3d(Layout);
            TEST_SB_END();
        }
        else if (signalGroupType==SignalGroupTypeObject)
        {
            numAudioObjects+=bsNumberOfSignals+1;
        }
        else if (signalGroupType==SignalGroupTypeSAOC)
        {
            numSAOCTransportChannels+=bsNumberOfSignals+1;
            TEST_SB_SKIP(                                       "saocDmxLayoutPresent");
                speaker_layout Layout;
                SpeakerConfig3d(Layout);
            TEST_SB_END();
        }
        else if (signalGroupType==SignalGroupTypeHOA)
        {
            numHOATransportChannels+=bsNumberOfSignals+1;
        }
    }
    Element_End0();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mpegh3daDecoderConfig()
{
    Elements.clear();

    Element_Begin1("mpegh3daDecoderConfig");
    escapedValue(numElements, 4, 8, 16,                         "numElements");
    numElements++;

    bool elementLengthPresent;
    Get_SB(elementLengthPresent,                                "elementLengthPresent");

    for (size_t Pos=0; Pos<numElements; Pos++)
    {
        int8u usacElementType;
        Get_S1(2, usacElementType,                              "usacElementType");
        switch (usacElementType)
        {
        case ID_USAC_SCE:
            mpegh3daSingleChannelElementConfig(0 /* TODO: sbrRatioIndex? */);
            Elements.push_back(usac_element(ID_USAC_SCE));
            break;
        case ID_USAC_CPE:
            mpegh3daChannelPairElementConfig(0 /* TODO: sbrRatioIndex? */);
            Elements.push_back(usac_element(ID_USAC_CPE));
            break;
        case ID_USAC_LFE:
            Elements.push_back(usac_element(ID_USAC_LFE));
            break;
        case ID_USAC_EXT:
            mpegh3daExtElementConfig();
            Elements.push_back(usac_element(ID_USAC_EXT));
            break;
        }

    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mpegh3daSingleChannelElementConfig(int8u sbrRatioIndex)
{
    Element_Begin1("mpegh3daSingleChannelElementConfig");
    mpegh3daCoreConfig();
    if (sbrRatioIndex>0)
        SbrConfig();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mpegh3daChannelPairElementConfig(int8u sbrRatioIndex)
{
    int32u nBits=floor(log2(numAudioChannels+numAudioObjects+numHOATransportChannels+numSAOCTransportChannels-1))+1;
    int8u stereoConfigIndex=0;
    int8u qceIndex;
    Element_Begin1("mpegh3daChannelPairElementConfig");
    if(mpegh3daCoreConfig()) // TODO: Provisory
        Skip_SB(                                                "igfIndependentTiling");

    if (sbrRatioIndex>0)
    {
        SbrConfig();
        Get_S1(2, stereoConfigIndex,                            "stereoConfigIndex");
    }

    if (stereoConfigIndex>0) {
        Mps212Config(stereoConfigIndex);
    }

    Get_S1(2, qceIndex,                                         "qceIndex");
    if (qceIndex>0)
    {
        TEST_SB_SKIP(                                           "shiftIndex0");
            Skip_BS(nBits,                                      "shiftChannel0");
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "shiftIndex1");
        Skip_BS(nBits,                                          "shiftChannel1");
    TEST_SB_END();

    if (sbrRatioIndex==0 && qceIndex==0)
        Skip_SB(                                                "lpdStereoIndex");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mpegh3daExtElementConfig()
{
    Element_Begin1("mpegh3daExtElementConfig");
    int32u usacExtElementType;
    escapedValue(usacExtElementType, 4, 8, 16,                  "usacExtElementType");

    int32u usacExtElementConfigLength;
    escapedValue(usacExtElementConfigLength, 4, 8, 16,          "usacExtElementConfigLength");

    int32u usacExtElementDefaultLength=0;
    TEST_SB_SKIP(                                               "usacExtElementDefaultLengthPresent");
        escapedValue(usacExtElementDefaultLength, 8, 16, 0,     "usacExtElementDefaultLength"); //TODO: check if call is valid
        usacExtElementDefaultLength++;
    TEST_SB_END();

    Skip_SB(                                                    "usacExtElementPayloadFrag");

    size_t Remain_Before=BS->Remain();
    switch (usacExtElementType)
    {
    case ID_EXT_ELE_FILL: break; // No configuration element
    //case ID_EXT_ELE_MPEGS:
        //TODO: SpatialSpecificConfig(); // In IEC 23003‐1
        //break;
    //case ID_EXT_ELE_SAOC:
        //TODO: SAOCSpecificConfig(); // In IEC 23003‐2
        //break;
    case ID_EXT_ELE_AUDIOPREROLL: break; // No configuration element
    case ID_EXT_ELE_UNI_DRC:
        mpegh3daUniDrcConfig();
        break;
    case ID_EXT_ELE_OBJ_METADATA:
        ObjectMetadataConfig();
        break;
    //case ID_EXT_ELE_SAOC_3D:
        //TODO: SAOC3DSpecificConfig();
        //break;
    //case ID_EXT_ELE_HOA:
        //TODO: HOAConfig();
        //break;
    case ID_EXT_ELE_FMT_CNVRTR: break; // No configuration element
    //case ID_EXT_ELE_MCT:
        //TODO: MCTConfig();
        //break;
    case ID_EXT_ELE_TCC:
        TccConfig();
        break;
    //case ID_EXT_ELE_HOA_ENH_LAYER:
        //TODO: HOAEnhConfig();
        //break;
    //case ID_EXT_ELE_HREP:
        //TODO: HREPConfig(current_signal_group);
        //break;
    //case ID_EXT_ELE_ENHANCED_OBJ_METADATA:
        //EnhancedObjectMetadataConfig();
        //break;
    default:
        Skip_BS(usacExtElementConfigLength*8,                   "reserved");
        break;
    }
    if (BS->Remain()+usacExtElementConfigLength*8>Remain_Before)
    {
        size_t Size=BS->Remain()+usacExtElementConfigLength*8-Remain_Before;
        int8u Padding=1;
        if (Size<8)
            Peek_S1((int8u)Size, Padding);

        Skip_BS(Size, Padding?"(Unknown)":"Padding");
    }

    Element_End0();
}

//---------------------------------------------------------------------------
bool File_MpegHa::mpegh3daCoreConfig()
{
    bool ToReturn=false;
    Element_Begin1("mpegh3daCoreConfig");
    Skip_SB(                                                    "tw_mdct");
    Skip_SB(                                                    "fullbandLpd");
    Skip_SB(                                                    "noiseFilling");
    TEST_SB_SKIP(                                               "enhancedNoiseFilling");
        ToReturn=true;
        Skip_SB(                                                "igfUseEnf");
        Skip_SB(                                                "igfUseHighRes");
        Skip_SB(                                                "igfUseWhitening");
        Skip_SB(                                                "igfAfterTnsSynth");
        Skip_S1(5,                                              "igfStartIndex");
        Skip_S1(4,                                              "igfStopIndex");
    TEST_SB_END();
    Element_End0();

    return ToReturn;
}

//---------------------------------------------------------------------------
void File_MpegHa::mpegh3daUniDrcConfig()
{
    Element_Begin1("mpegh3daUniDrcConfig");
    int8u drcCoefficientsUniDrcCount;
    Get_S1(3, drcCoefficientsUniDrcCount,                       "drcCoefficientsUniDrcCount");

    int8u drcInstructionsUniDrcCount;
    Get_S1(6, drcInstructionsUniDrcCount,                       "drcInstructionsUniDrcCount");

    Element_Begin1("mpegh3daUniDrcChannelLayout");
    Skip_S1(7,                                                  "baseChannelCount");
    Element_End0();

    for (int8u Pos=0; Pos<drcCoefficientsUniDrcCount; Pos++)
        drcCoefficientsUniDrc(); // in File_USAC.cpp

    for (int8u Pos=0; Pos<drcInstructionsUniDrcCount; Pos++)
    {
        int8u drcInstructionsType;
        Get_S1(Peek_SB()?2:1, drcInstructionsType,              "drcInstructionsType");


        if (drcInstructionsType==2)
            Skip_S1(7,                                          "mae_groupID");
        else if (drcInstructionsType==3)
            Skip_S1(5,                                          "mae_groupPresetID");

        drcInstructionsUniDrc(); // in File_USAC.cpp
    }

    TEST_SB_SKIP(                                               "uniDrcConfigExtPresent");
        uniDrcConfigExtension(); // in File_USAC.cpp
    TEST_SB_END();

    TEST_SB_SKIP(                                               "loudnessInfoSetPresent");
        mpegh3daLoudnessInfoSet();
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mpegh3daLoudnessInfoSet()
{
    Element_Begin1("mpegh3daLoudnessInfoSet");
    int8u loudnessInfoCount;
    Get_S1(6, loudnessInfoCount,                                "loudnessInfoCount");
    for(int8u Pos=0; Pos<loudnessInfoCount; Pos++)
    {
        int8u loudnessInfoType;
        Get_S1(2, loudnessInfoType,                             "loudnessInfoType");
        if (loudnessInfoType==1 || loudnessInfoType==2)
            Skip_S1(7,                                          "mae_groupID");
        else if (loudnessInfoType==3)
            Skip_S1(5,                                          "mae_groupPresetID");

        loudnessInfo(false); // in File_USAC.cpp
    }

    TEST_SB_SKIP(                                               "loudnessInfoAlbumPresent");
        int8u loudnessInfoAlbumCount;
        Get_S1(6, loudnessInfoAlbumCount,                       "loudnessInfoAlbumCount");
        for (int8u Pos=0; Pos<loudnessInfoAlbumCount; Pos++)
        {
            loudnessInfo(true); // in File_USAC.cpp
        }
    TEST_SB_END();

    TEST_SB_SKIP(                                               "loudnessInfoSetExtensionPresent");
        loudnessInfoSetExtension(); // in File_USAC.cpp
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::ObjectMetadataConfig()
{
    Element_Begin1("ObjectMetadataConfig");
    Skip_SB(                                                    "lowDelayMetadataCoding");
    TESTELSE_SB_SKIP(                                           "hasCoreLength");
    TESTELSE_SB_ELSE(                                           "hasCoreLength");
        Skip_S1(6,                                              "frameLength");
    TESTELSE_SB_END();

    TEST_SB_SKIP("hasScreenRelativeObjects");
        // TODO: for (int16u Pos=0; Pos<num_objects; Pos++)
        //{
        //    Skip_SB(                                            "isScreenRelativeObject");
        //}
    TEST_SB_END();
    Skip_SB(                                                    "hasDynamicObjectPriority");
    Skip_SB(                                                    "hasUniformSpread");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::SAOC3DSpecificConfig()
{
    int8u bsSamplingFrequencyIndex, bsNumSaocDmxChannels, bsNumSaocDmxObjects, bsNumSaocObjects;
    int32u NumPremixedChannels=0,  NumSaocChannels=0, NumInputSignals=0;
    Element_Begin1("SAOC3DSpecificConfig");
    Get_S1(4, bsSamplingFrequencyIndex,                         "bsSamplingFrequencyIndex");
    if (bsSamplingFrequencyIndex==15)
        Skip_S3(24,                                             "bsSamplingFrequency");

    Skip_S1(3,                                                  "bsFreqRes");
    Skip_SB(                                                    "bsDoubleFrameLengthFlag");
    Get_S1(5, bsNumSaocDmxChannels,                             "bsNumSaocDmxChannels");
    Get_S1(5, bsNumSaocDmxObjects,                              "bsNumSaocDmxObjects");
    Skip_SB(                                                    "bsDecorrelationMethod");

    if (bsNumSaocDmxChannels>0)
    {
        speaker_layout saocChannelLayout;
        SpeakerConfig3d(saocChannelLayout);
       NumSaocChannels=SAOC3DgetNumChannels(saocChannelLayout);
       NumInputSignals+=NumSaocChannels;
    }

    Get_S1(8, bsNumSaocObjects ,                                "bsNumSaocObjects");
    NumInputSignals+=bsNumSaocObjects;

    for (int8u Pos=0; Pos<NumSaocChannels; Pos++)
    {
        for(int8u Pos2=Pos+1; Pos2<NumSaocChannels; Pos2++)
            Skip_SB(                                            "bsRelatedTo");
    }

    for (int8u Pos=NumSaocChannels; Pos<NumInputSignals; Pos++)
    {
        for(int8u Pos2=Pos+1; Pos2<NumInputSignals; Pos2++)
            Skip_SB(                                            "bsRelatedTo");
    }

    Skip_SB(                                                    "bsOneIOC");
    TEST_SB_SKIP(                                               "bsSaocDmxMethod");
        NumPremixedChannels=SAOC3DgetNumChannels(DefaultLayout);
    TEST_SB_END();

    TEST_SB_SKIP(                                               "bsDualMode");
        Skip_S1(5,                                              "bsBandsLow");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "bsDcuFlag");
        Skip_SB(                                                "bsDcuMandatory");
        TEST_SB_SKIP(                                           "bsDcuDynamic");
            Skip_SB(                                            "bsDcuMode");
            Skip_S1(4,                                          "bsDcuParam");
        TEST_SB_END();
    TEST_SB_END();
    Skip_S1(BS->Remain()%8,                                     "byte_align");
    //TODO: SAOC3DExtensionConfig(); // in IEC 23003‐2
    Element_End0();
}

//---------------------------------------------------------------------------
int32u File_MpegHa::SAOC3DgetNumChannels(speaker_layout Layout)
{
    int32u ToReturn=Layout.numSpeakers;

    for (int32u Pos=0; Pos<Layout.numSpeakers; Pos++)
    {
        if (Layout.SpeakersInfo.size()>Pos && Layout.SpeakersInfo[Pos].isLFE)
            ToReturn--;
    }

    return ToReturn;
}

//---------------------------------------------------------------------------
void File_MpegHa::TccConfig()
{
    for(int32u Pos=0; Pos<numElements; Pos++)
    {
        if(Elements.size()>Pos && (Elements[Pos].Type==ID_USAC_SCE || Elements[Pos].Type==ID_USAC_CPE))
            Skip_S1(2,                                       "tccMode");
    }
}

//---------------------------------------------------------------------------
void File_MpegHa::EnhancedObjectMetadataConfig()
{
    bool hasCommonGroupExcludedSectors=false;

    Element_Begin1("EnhancedObjectMetadataConfig");
    TEST_SB_SKIP(                                               "hasDiffuseness");
        Skip_SB(                                                "hasCommonGroupDiffuseness");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "hasExcludedSectors");
        TEST_SB_GET(hasCommonGroupExcludedSectors,              "hasCommonGroupExcludedSectors");
            Skip_SB(                                            "useOnlyPredefinedSectors");
        TEST_SB_END();
    TEST_SB_END();

    TEST_SB_SKIP(                                               "hasClosestSpeakerCondition");
        Skip_S1(7,                                              "closestSpeakerThresholdAngle");
    TEST_SB_END();

    //TODO: num_objects ?
    /*
    for (int8u Pos=0; Pos<num_objects; Pos++)
    {
        TEST_SB_SKIP(                                           "hasDivergence");
            Skip_S1(6,                                          "divergenceAzimuthRange");
        TEST_SB_END();

        if (!hasCommonGroupExcludedSectors)
            Skip_SB(                                            "useOnlyPredefinedSectors");
    }
    */
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mpegh3daConfigExtension()
{
    Element_Begin1("mpegh3daConfigExtension");
    int32u numConfigExtensions;
    escapedValue(numConfigExtensions, 2, 4, 8,                  "numConfigExtensions");
    numConfigExtensions++;

    for (int32u Pos=0; Pos<numConfigExtensions; Pos++)
    {
        int32u usacConfigExtType;
        escapedValue(usacConfigExtType, 4, 8, 16,               "usacConfigExtType");

        int32u usacConfigExtLength;
        escapedValue(usacConfigExtLength, 4, 8, 16,             "usacConfigExtLength");

        size_t Remain_Before=BS->Remain();
        switch ((UsacConfigExtType)usacConfigExtType)
        {
        case ID_CONFIG_EXT_FILL:
            while (usacConfigExtLength--)
                Skip_S1(8,                                      "fill_byte"); // should be '10100101'
            break;
        //case ID_CONFIG_EXT_DOWNMIX:
        //TODO:    downmixConfig();
        //    break;
        case ID_CONFIG_EXT_LOUDNESS_INFO:
            mpegh3daLoudnessInfoSet();
            break;
        case ID_CONFIG_EXT_AUDIOSCENE_INFO:
            mae_AudioSceneInfo();
            break;
        //case ID_CONFIG_EXT_HOA_MATRIX:
        //TODO:    HoaRenderingMatrixSet();
        //    break;
        case ID_CONFIG_EXT_ICG:
            ICGConfig();
            break;
        case ID_CONFIG_EXT_SIG_GROUP_INFO:
            SignalGroupInformation();
            break;
        default:
            Skip_BS(usacConfigExtLength*8,                      "reserved");
        }
        if (BS->Remain()+usacConfigExtLength*8>Remain_Before)
        {
            size_t Size=BS->Remain()+usacConfigExtLength*8-Remain_Before;
            int8u Padding=1;
            if (Size<8)
                Peek_S1((int8u)Size, Padding);

            Skip_BS(Size, Padding?"(Unknown)":"Padding");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::SignalGroupInformation()
{
    Element_Begin1("SignalGroupInformation");
    for (int8u Pos=0; Pos<bsNumSignalGroups+1; Pos++)
    {
        Skip_S1(3,                                              "groupPriority");
        Skip_SB(                                                "fixedPosition");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::ICGConfig()
{
    Element_Begin1("ICGConfig");
    TEST_SB_SKIP(                                               "ICPresent");
        for (int32u Pos=0; Pos<numElements; Pos++)
        {
            if (Elements.size()>Pos && Elements[Pos].Type==ID_USAC_CPE)
                Skip_SB(                                        "ICinCPE");
        }

        TEST_SB_SKIP(                                           "ICGPreAppliedPresent");
            for (int32u Pos=0; Pos<numElements; Pos++)
            {
                if (Elements.size()>Pos && Elements[Pos].Type==ID_USAC_CPE)
                    Skip_SB(                                     "ICGPreAppliedCPE");
            }
        TEST_SB_END();
    TEST_SB_END();
    Element_End0();
}

//***************************************************************************
// PACTYP_AUDIOSCENEINFO
//***************************************************************************

//---------------------------------------------------------------------------
void File_MpegHa::mae_AudioSceneInfo()
{
    GroupPresets.clear();

    Element_Begin1("mae_AudioSceneInfo");
    TESTELSE_SB_SKIP(                                           "mae_isMainStream");
        TEST_SB_SKIP(                                           "mae_audioSceneInfoIDPresent");
            Skip_S1(8,                                          "mae_audioSceneInfoID");
        TEST_SB_END();
        int8u mae_numGroups;
        Get_S1(7, mae_numGroups,                                "mae_numGroups");
        mae_GroupDefinition(mae_numGroups);
        int8u mae_numSwitchGroups;
        Get_S1(5, mae_numSwitchGroups,                          "mae_numSwitchGroups");
        mae_SwitchGroupDefinition(mae_numSwitchGroups);
        int8u mae_numGroupPresets;
        Get_S1(5, mae_numGroupPresets,                          "mae_numGroupPresets");
        mae_GroupPresetDefinition(mae_numGroupPresets);
        mae_Data(mae_numGroups, mae_numGroupPresets);
        Skip_S1(7,                                              "mae_metaDataElementIDmaxAvail");
    TESTELSE_SB_ELSE(                                           "mae_isMainStream");
        Skip_S1(7,                                              "mae_bsMetaDataElementIDoffset");
        Skip_S1(7,                                              "mae_metaDataElementIDmaxAvail");
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mae_GroupDefinition(int8u numGroups)
{
    Element_Begin1("mae_GroupDefinition");
    for (int8u Pos=0; Pos<numGroups; Pos++)
    {
        Skip_S1(7,                                              "mae_groupID");
        Skip_SB(                                                "mae_allowOnOff");
        Skip_SB(                                                "mae_defaultOnOff");

        TEST_SB_SKIP(                                           "mae_allowPositionInteractivity");
            Skip_S1(7,                                          "mae_interactivityMinAzOffset");
            Skip_S1(7,                                          "mae_interactivityMaxAzOffset");
            Skip_S1(5,                                          "mae_interactivityMinElOffset");
            Skip_S1(5,                                          "mae_interactivityMaxElOffset");
            Skip_S1(4,                                          "mae_interactivityMinDistFactor");
            Skip_S1(4,                                          "mae_interactivityMaxDistFactor");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "mae_allowGainInteractivity");
            Skip_S1(6,                                          "mae_interactivityMinGain");
            Skip_S1(5,                                          "mae_interactivityMaxGain");
        TEST_SB_END();

        int8u mae_bsGroupNumMembers;
        Get_S1(7, mae_bsGroupNumMembers,                        "mae_bsGroupNumMembers");
        TESTELSE_SB_SKIP(                                       "mae_hasConjunctMembers");
            Skip_S1(7,                                          "mae_startID");
        TESTELSE_SB_ELSE(                                       "mea_startID");
            for (int8u Pos2=0; Pos2<mae_bsGroupNumMembers+1; Pos2++)
                Skip_S1(7,                                      "mae_metaDataElementID");
        TESTELSE_SB_END();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mae_SwitchGroupDefinition(int8u numSwitchGroups)
{
    Element_Begin1("mae_SwitchGroupDefinition");
    for(int8u Pos=0; Pos<numSwitchGroups; Pos++)
    {
        Skip_S1(5,                                              "mae_switchGroupID");

        TEST_SB_SKIP(                                           "mae_switchGroupAllowOnOff");
            Skip_SB(                                            "mae_switchGroupDefaultOnOff");
        TEST_SB_END();

        int8u mae_bsSwitchGroupNumMembers;
        Get_S1(5, mae_bsSwitchGroupNumMembers,                  "mae_bsSwitchGroupNumMembers");
        for (int8u Pos2=0; Pos2<mae_bsSwitchGroupNumMembers+1; Pos2++)
            Skip_S1(7,                                          "mae_switchGroupMemberID");

        Skip_S1(7,                                              "mae_switchGroupDefaultGroupID");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mae_GroupPresetDefinition(int8u numGroupPresets)
{
    Element_Begin1("mae_GroupPresetDefinition");
    for (int8u Pos=0; Pos<numGroupPresets; Pos++)
    {
        Skip_S1(5,                                              "mae_groupPresetID");
        Skip_S1(5,                                              "mae_groupPresetKind");

        int8u mae_bsGroupPresetNumConditions;
        Get_S1(4, mae_bsGroupPresetNumConditions,               "mae_bsGroupPresetNumConditions");
        for (int8u Pos2=0; Pos2<mae_bsGroupPresetNumConditions+1; Pos2++)
        {
            Skip_S1(7,                                          "mae_groupPresetReferenceID");
            TEST_SB_SKIP(                                       "mae_groupPresetConditionOnOff");
                Skip_SB(                                        "mae_groupPresetDisableGainInteractivity");
                TEST_SB_SKIP(                                   "mae_groupPresetGainFlag");
                    Skip_S1(8,                                  "mae_groupPresetGain");
                TEST_SB_END();
                Skip_SB(                                        "mae_groupPresetDisablePositionInteractivity");
                TEST_SB_SKIP(                                   "mae_groupPresetPositionFlag");
                    Skip_S1(8,                                  "mae_groupPresetAzOffset");
                    Skip_S1(6,                                  "mae_groupPresetElOffset");
                    Skip_S1(4,                                  "mae_groupPresetDistFactor");
                TEST_SB_END();
            TEST_SB_END();
            GroupPresets.push_back(group_preset(mae_bsGroupPresetNumConditions));
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mae_Data(int8u numGroups, int8u numGroupPresets)
{
    Element_Begin1("mae_Data");
    int8u mae_numDataSets;
    Get_S1(4, mae_numDataSets,                                  "mae_numDataSets");
    for (int8u Pos=0; Pos<mae_numDataSets; Pos++)
    {
        int8u mae_dataType;
        Get_S1(4, mae_dataType,                                 "mae_dataType");
        int16u mae_dataLength;
        Get_S2(16, mae_dataLength,                              "mae_dataLength");

        size_t Remain_Before=BS->Remain();
        switch ((MaeDataType)mae_dataType)
        {
        case ID_MAE_GROUP_DESCRIPTION:
            mae_Description(ID_MAE_GROUP_DESCRIPTION);
            break;
        case ID_MAE_SWITCHGROUP_DESCRIPTION:
            mae_Description(ID_MAE_SWITCHGROUP_DESCRIPTION);
            break;
        case ID_MAE_GROUP_PRESET_DESCRIPTION:
            mae_Description(ID_MAE_GROUP_PRESET_DESCRIPTION);
            break;
        case ID_MAE_GROUP_CONTENT:
            mae_ContentData();
            break;
        case ID_MAE_GROUP_COMPOSITE:
            mae_CompositePair();
            break;
        case ID_MAE_SCREEN_SIZE:
            mae_ProductionScreenSizeData();
            break;
        case ID_MAE_DRC_UI_INFO:
            mae_DrcUserInterfaceInfo(mae_dataLength);
            break;
        case ID_MAE_SCREEN_SIZE_EXTENSION:
            mae_ProductionScreenSizeDataExtension();
            break;
        case ID_MAE_GROUP_PRESET_EXTENSION:
            mae_GroupPresetDefinitionExtension(numGroupPresets);
            break;
        case ID_MAE_LOUDNESS_COMPENSATION:
            mae_LoudnessCompensationData(numGroups, numGroupPresets);
            break;
        default:
            Skip_BS(mae_dataLength*8,                           "reserved");
        }
        if (BS->Remain()+mae_dataLength*8>Remain_Before)
        {
            size_t Size=BS->Remain()+mae_dataLength*8-Remain_Before;
            int8u Padding=1;
            if (Size<8)
                Peek_S1((int8u)Size, Padding);

            Skip_BS(Size, Padding?"(Unknown)":"Padding");
        }

    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mae_Description(MaeDataType type)
{
    Element_Begin1("mae_Description");
    int8u mae_bsNumDescriptionBlocks;
    Get_S1(7, mae_bsNumDescriptionBlocks,                       "mae_bsNumDescriptionBlocks");
    for (int8u Pos=0; Pos<mae_bsNumDescriptionBlocks+1; Pos++)
    {
        if (type==ID_MAE_GROUP_DESCRIPTION)
            Skip_S1(7,                                          "mae_descriptionGroupID");
        else if (type==ID_MAE_SWITCHGROUP_DESCRIPTION)
            Skip_S1(5,                                          "mae_descriptionSwitchGroupID");
        else if (type==ID_MAE_GROUP_PRESET_DESCRIPTION)
            Skip_S1(5,                                          "mae_descriptionGroupPresetID");

        int8u mae_bsNumDescLanguages;
        Get_S1(4, mae_bsNumDescLanguages,                       "mae_bsNumDescLanguages");
        for (int8u Pos2=0; Pos2<mae_bsNumDescLanguages+1; Pos2++)
        {
            Skip_S3(24,                                         "mae_bsDescriptionLanguage");

            int8u mae_bsDescriptionDataLength;
            Get_S1(8, mae_bsDescriptionDataLength,              "mae_bsDescriptionDataLength");
            for (int8u Pos3=0; Pos3<mae_bsDescriptionDataLength+1; Pos3++)
                Skip_S1(8,                                      "mae_descriptionData");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mae_ContentData()
{
    Element_Begin1("mae_ContentData");
    int8u mae_bsNumContentDataBlocks;
    Get_S1(7, mae_bsNumContentDataBlocks,                       "mae_bsNumContentDataBlocks");
    for (int8u Pos=0; Pos<mae_bsNumContentDataBlocks+1; Pos++)
    {
        Skip_S1(7,                                              "mae_ContentDataGroupID");
        Skip_S1(4,                                              "mae_contentKind");
        TEST_SB_SKIP(                                           "mae_hasContentLanguage");
            Skip_S3(24,                                         "mae_contentLanguage");
        TEST_SB_END();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mae_CompositePair()
{
    Element_Begin1("mae_CompositePair");
    int8u mae_bsNumCompositePairs;
    Get_S1(7, mae_bsNumCompositePairs,                          "mae_bsNumCompositePairs");
    for (int8u Pos=0; Pos<mae_bsNumCompositePairs+1; Pos++)
    {
        Skip_S1(7,                                              "mae_CompositeElementID0");
        Skip_S1(7,                                              "mae_CompositeElementID1");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mae_ProductionScreenSizeData()
{
    Element_Begin1("mae_ProductionScreenSizeData");
    TEST_SB_SKIP(                                               "hasNonStandardScreenSize");
        Skip_S2(9,                                              "bsScreenSizeAz");
        Skip_S2(9,                                              "bsScreenSizeTopEl");
        Skip_S2(9,                                              "bsScreenSizeBottomEl");
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mae_DrcUserInterfaceInfo(int16u dataLength)
{
    Element_Begin1("mae_DrcUserInterfaceInfo");
    int8u version;
    Get_S1(2, version,                                          "version");
    if (version==0)
    {
        int8u bsNumTargetLoudnessConditions;
        Get_S1(3, bsNumTargetLoudnessConditions,                "bsNumTargetLoudnessConditions");
        for (int8u Pos=0; Pos<bsNumTargetLoudnessConditions; Pos++)
        {
            Skip_S1(6,                                          "bsTargetLoudnessValueUpper");
            Skip_S2(16,                                         "drcSetEffectAvailable");
        }
    }
    else
    {
        Skip_BS((dataLength-2)*8,                               "reserved");
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mae_ProductionScreenSizeDataExtension()
{
    Element_Begin1("mae_ProductionScreenSizeDataExtension");
    TEST_SB_SKIP(                                               "mae_overwriteProductionScreenSizeData");
        Skip_S2(10,                                             "bsScreenSizeLeftAz");
        Skip_S2(10,                                             "bsScreenSizeRightAz");
    TEST_SB_END();
    int8u mae_NumPresetProductionScreens;
    Get_S1(5, mae_NumPresetProductionScreens,                   "mae_NumPresetProductionScreens");
    for (int8u Pos=0; Pos< mae_NumPresetProductionScreens; Pos++)
    {
        Skip_S1(5,                                              "mae_productionScreenGroupPresetID");
        TEST_SB_SKIP(                                           "mae_hasNonStandardScreenSize");
            TESTELSE_SB_SKIP(                                   "isCenteredInAzimuth");
                Skip_S2(9,                                      "bsScreenSizeAz");
            TESTELSE_SB_ELSE(                                   "isCenteredInAzimuth");
                Skip_S2(10,                                     "bsScreenSizeLeftAz");
                Skip_S2(10,                                     "bsScreenSizeRightAz");
            TESTELSE_SB_END();
            Skip_S2(9,                                          "bsScreenSizeTopEl");
            Skip_S2(9,                                          "bsScreenSizeBottomEl");
        TEST_SB_END();
    }
    Element_End0();
}


//---------------------------------------------------------------------------
void File_MpegHa::mae_GroupPresetDefinitionExtension(int8u numGroupPresets)
{
    Element_Begin1("mae_GroupPresetDefinitionExtension");
    for (int8u Pos=0; Pos<numGroupPresets; Pos++)
    {
        TEST_SB_SKIP(                                           "mae_hasSwitchGroupConditions");
            int8u Temp=Pos<GroupPresets.size()?GroupPresets[Pos].bsGroupPresetNumConditions:0;
            for(int8u Pos2=0; Pos2<Temp; Pos2++)
                Skip_SB(                                        "mae_isSwitchGroupCondition");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "mae_hasDownmixIdGroupPresetExtensions");
            int8u mae_numDownmixIdGroupPresetExtensions;
            Get_S1(5, mae_numDownmixIdGroupPresetExtensions,    "mae_numDownmixIdGroupPresetExtensions");
            for (int8u Pos2=1; Pos2<mae_numDownmixIdGroupPresetExtensions+1; Pos2++)
            {
                Skip_S1(7,                                      "mae_groupPresetDownmixId");

                int8u mae_bsGroupPresetNumConditions;
                Get_S1(4, mae_bsGroupPresetNumConditions,       "mae_bsGroupPresetNumConditions");
                for (int8u Pos3=0; Pos3<mae_bsGroupPresetNumConditions+1; Pos3++)
                {
                    TESTELSE_SB_SKIP(                           "mae_isSwitchGroupCondition");
                        Skip_S1(5,                              "mae_groupPresetSwitchGroupID");
                    TESTELSE_SB_ELSE(                           "mae_isSwitchGroupCondition");
                        Skip_S1(7,                              "mae_groupPresetGroupID");
                    TESTELSE_SB_END();
                    TEST_SB_SKIP(                               "mae_groupPresetConditionOnOff");
                        Skip_SB(                                "mae_groupPresetDisableGainInteractivity");
                        TEST_SB_SKIP(                           "mae_groupPresetGainFlag");
                            Skip_S1(8,                          "mae_groupPresetGain");
                        TEST_SB_END();
                        Skip_SB(                                "mae_groupPresetDisablePositionInteractivity");
                        TEST_SB_SKIP(                           "mae_groupPresetPositionFlag");
                            Skip_S1(8,                          "mae_groupPresetAzOffset");
                            Skip_S1(6,                          "mae_groupPresetElOffset");
                            Skip_S1(4,                          "mae_groupPresetDistFactor");
                        TEST_SB_END();
                    TEST_SB_END();
                }
            }
        TEST_SB_END();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_MpegHa::mae_LoudnessCompensationData(int8u numGroups, int8u numGroupPresets)
{
    Element_Begin1("mae_LoudnessCompensationData");
    TEST_SB_SKIP(                                               "mae_loudnessCompGroupLoudnessPresent");
        for (int8u Pos=0; Pos<numGroups; Pos++)
            Skip_S1(8,                                          "mae_bsLoudnessCompGroupLoudness");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "mae_loudnessCompDefaultParamsPresent");
        for (int8u Pos=0; Pos<numGroups; Pos++)
            Skip_SB(                                            "mae_loudnessCompDefaultIncludeGroup");

        TEST_SB_SKIP(                                           "mae_loudnessCompDefaultMinMaxGainPresent");
            Skip_S1(4,                                          "mae_bsLoudnessCompDefaultMinGain");
            Skip_S1(4,                                          "mae_bsLoudnessCompDefaultMaxGain");
        TEST_SB_END();
    TEST_SB_END();

    for (int8u Pos=0; Pos<numGroupPresets; Pos++)
    {
        TEST_SB_SKIP(                                           "mae_loudnessCompPresetParamsPresent");
            for (int8u Pos2=0; Pos2<numGroups; Pos2++)
                Skip_SB(                                        "mae_loudnessCompPresetIncludeGroup");

            TEST_SB_SKIP(                                       "mae_loudnessCompPresetMinMaxGainPresent");
                Skip_S1(4,                                      "mae_bsLoudnessCompPresetMinGain");
                Skip_S1(4,                                      "mae_bsLoudnessCompPresetMaxGain");
            TEST_SB_END();
        TEST_SB_END();
    }
    Element_End0();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_MPEGHA_YES

