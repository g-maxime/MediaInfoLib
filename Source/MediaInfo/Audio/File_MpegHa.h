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
    //Info
    enum SignalGroupType
    {
        SignalGroupTypeChannels,
        SignalGroupTypeObject,
        SignalGroupTypeSAOC,
        SignalGroupTypeHOA
    };

    enum UsacElementType
    {
        ID_USAC_SCE,
        ID_USAC_CPE,
        ID_USAC_LFE,
        ID_USAC_EXT
    };

    enum UsacExtElementType
    {
        ID_EXT_ELE_FILL,
        ID_EXT_ELE_MPEGS,
        ID_EXT_ELE_SAOC,
        ID_EXT_ELE_AUDIOPREROLL,
        ID_EXT_ELE_UNI_DRC,
        ID_EXT_ELE_OBJ_METADATA,
        ID_EXT_ELE_SAOC_3D,
        ID_EXT_ELE_HOA,
        ID_EXT_ELE_FMT_CNVRTR,
        ID_EXT_ELE_MCT,
        ID_EXT_ELE_TCC,
        ID_EXT_ELE_HOA_ENH_LAYER,
        ID_EXT_ELE_HREP,
        ID_EXT_ELE_ENHANCED_OBJ_METADATA
    };

    enum UsacConfigExtType
    {
        ID_CONFIG_EXT_FILL,
        ID_CONFIG_EXT_DOWNMIX,
        ID_CONFIG_EXT_LOUDNESS_INFO,
        ID_CONFIG_EXT_AUDIOSCENE_INFO,
        ID_CONFIG_EXT_HOA_MATRIX,
        ID_CONFIG_EXT_ICG,
        ID_CONFIG_EXT_SIG_GROUP_INFO
    };

    enum MaeDataType
    {
        ID_MAE_GROUP_DESCRIPTION,
        ID_MAE_SWITCHGROUP_DESCRIPTION,
        ID_MAE_GROUP_CONTENT,
        ID_MAE_GROUP_COMPOSITE,
        ID_MAE_SCREEN_SIZE,
        ID_MAE_GROUP_PRESET_DESCRIPTION,
        ID_MAE_DRC_UI_INFO,
        ID_MAE_SCREEN_SIZE_EXTENSION,
        ID_MAE_GROUP_PRESET_EXTENSION,
        ID_MAE_LOUDNESS_COMPENSATION
    };

    struct speaker_info
    {
        bool angularPrecision;
        int16u AzimuthAngle;
        bool AzimuthDirection;
        int16u ElevationAngle;
        bool ElevationDirection;
        bool isLFE;

        speaker_info(bool angularPrecision) :
            angularPrecision(angularPrecision),
            AzimuthAngle(0),
            AzimuthDirection(false),
            ElevationAngle(0),
            ElevationDirection(false),
            isLFE(false)
        {};
    };

    struct speaker_layout
    {
        int32u numSpeakers;
        vector<speaker_info> SpeakersInfo;
        speaker_layout() :
            numSpeakers(0)
        {};
    };

    struct group_preset
    {
        int8u bsGroupPresetNumConditions;
        group_preset(int8u bsGroupPresetNumConditions) :
            bsGroupPresetNumConditions(bsGroupPresetNumConditions)
        {};
    };

    speaker_layout DefaultLayout;
    vector<group_preset> GroupPresets;

    int16u numAudioChannels;
    int16u numAudioObjects;
    int16u numSAOCTransportChannels;
    int16u numHOATransportChannels;

    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Sync();

    void escapedValue(int32u& Value, int8u nBits1, int8u nBits2, int8u nBits3, const char* Name);
    void SbrConfig();
    void SbrDlftHeader();
    void Mps212Config(int8u StereoConfigindex);

    void mpegh3daConfig();
    void SpeakerConfig3d(speaker_layout& Layout);
    void mpegh3daFlexibleSpeakerConfig(speaker_layout& Layout);
    void mpegh3daSpeakerDescription(speaker_layout& Layout);
    void FrameworkConfig3d();
    void mpegh3daDecoderConfig();
    void mpegh3daSingleChannelElementConfig(int8u sbrRatioIndex);
    void mpegh3daChannelPairElementConfig(int8u sbrRatioIndex);
    void mpegh3daLfeElementConfig();
    void mpegh3daExtElementConfig();
    bool mpegh3daCoreConfig();
    void mpegh3daUniDrcConfig();
    void ObjectMetadataConfig();
    void SAOC3DSpecificConfig();

    void mpegh3daConfigExtension();

    void mae_AudioSceneInfo();
    void mae_GroupDefinition(int8u numGroups);
    void mae_SwitchGroupDefinition(int8u numSwitchGroups);
    void mae_GroupPresetDefinition(int8u numGroupPresets);
    void mae_Data(int8u numGroups, int8u numGroupPresets);
    void mae_Description(MaeDataType type);
    void mae_ContentData();
    void mae_CompositePair();
    void mae_ProductionScreenSizeData();
    void mae_DrcUserInterfaceInfo(int16u dataLength);
    void mae_ProductionScreenSizeDataExtension();
    void mae_GroupPresetDefinitionExtension(int8u numGroupPresets);
    void mae_LoudnessCompensationData(int8u numGroups, int8u numGroupPresets);
};

} //NameSpace

#endif
