/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

 // audioProgramme
 // - audioContent
 // - - audioObject
 // - - - audioPackFormat
 // - - - - audioChannelFormat
 // - - - audioTrackUID
 // - - - - audioChannelFormat +
 // - - - - audioPackFormat
 // - - - - - audioChannelFormat +
 // - - - - audioTrackFormat
 // - - - - - audioStreamFormat
 // - - - - - - audioChannelFormat +
 // - - - - - - audioPackFormat +
 // - - - - - - audioTrackFormat +
 
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
#if defined(MEDIAINFO_ADM_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Adm.h"
#include "ThirdParty/tfsxml/tfsxml.h"
#include <algorithm>
#include <bitset>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

#define IncoherencyMessage "Incoherency between enums and message strings"
    
//---------------------------------------------------------------------------
static int32u pow10(int i)
{
    static int pow10[10] =
    {
        1,
        10,
        100,
        1000,
        10000, 
        100000,
        1000000,
        10000000,
        100000000,
        1000000000,
    };
    return pow10[i];
}

static float32 TimeCodeToFloat(string v)
{
    if (v.size() < 8 || v[2] != ':' || v[5] != ':')
        return 0;
    for (int i = 0; i < 8; i++)
    {
        switch (i)
        {
        case 2:
        case 5:
            if (v[i] != ':')
                return 0;
            break;
        default:
            if (v[i] < '0' || v[i] > '9')
                return 0;
        }
    }
    int32u  Value = (v[0] - '0') * 10 * 6 * 10 * 6 * 10
                  + (v[1] - '0')      * 6 * 10 * 6 * 10
                  + (v[3] - '0')          * 10 * 6 * 10
                  + (v[4] - '0')               * 6 * 10
                  + (v[6] - '0')                   * 10
                  + (v[7] - '0')                       ;
    if (v.size() < 9 || v[8] != '.')
        return Value;
    int i = 9;
    int32u ValueF = 0;
    int ValueF_Exponent = 0;
    const int Exponent_Max = 9;
    while (i < v.size() && v[i] >= '0' && v[i] <= '9')
    {
        ValueF_Exponent++;
        if (ValueF_Exponent <= Exponent_Max)
        {
            ValueF *= 10;
            ValueF += v[i] - '0';
        }
        i++;
    }
    if (i >= v.size() || v[i] != 'S')
        return Value + (float32)ValueF / pow10(ValueF_Exponent <= Exponent_Max ? ValueF_Exponent : Exponent_Max);
    int32u SampleRate = 0;
    int SampleRate_Exponent = 0;
    i++;
    while (i < v.size() && v[i] >= '0' && v[i] <= '9')
    {
        SampleRate_Exponent++;
        if (SampleRate_Exponent <= Exponent_Max)
        {
            SampleRate *= 10;
            SampleRate += v[i] - '0';
        }
        i++;
    }
    while (SampleRate_Exponent > Exponent_Max)
    {
        if (ValueF_Exponent < Exponent_Max)
            ValueF_Exponent /= 10;
        if (ValueF_Exponent)
            ValueF_Exponent--;
        SampleRate_Exponent--;
    }
    return Value + (float32)ValueF / SampleRate;
}

//---------------------------------------------------------------------------
static const char* profile_names[]=
{
    "profileName",
    "profileVersion",
    "profileID",
    "levelID",
};
static const int profile_names_size=(int)sizeof(profile_names)/sizeof(const char*);
static const char* profile_names_InternalID[profile_names_size]=
{
    "Format",
    "Version",
    "Profile",
    "Level",
};
struct profile_info
{
    string Strings[4];
    string profile_info_build(size_t Max=profile_names_size)
    {
        bool HasParenthsis=false;
        string ToReturn;
        for (size_t i=0; i<Max; i++)
        {
            if (!Strings[i].empty())
            {
                if (!ToReturn.empty())
                {
                    if (i==1)
                        ToReturn+=", Version";
                    if (!HasParenthsis)
                        ToReturn+=' ';
                }
                if (i>=2)
                {
                    if (!HasParenthsis)
                    {
                        ToReturn+='(';
                        HasParenthsis=true;
                    }
                    else
                    {
                        ToReturn+=',';
                        ToReturn+=' ';
                    }
                }
                if (i>=2)
                {
                    ToReturn+=profile_names[i];
                    ToReturn+='=';
                }
                ToReturn+=Strings[i];
            }
        }
        if (HasParenthsis)
            ToReturn+=')';
        return ToReturn;
    }
};

static bool IsHexaDigit(char Value)
{
    return (Value >= '0' && Value <= '9')
        || (Value >= 'A' && Value <= 'F')
        || (Value >= 'a' && Value <= 'f');
}

static bool IsHexaDigit(const string& Value, size_t pos, size_t len)
{
    len += pos;
    for (; pos < len; pos++) {
        if (!IsHexaDigit(Value[pos]))
            return false;
    }
    return true;
}

enum check_flags_items {
    Version0,
    Version1,
    Version2,
    Count0,
    Count2,
    flags_Max
};
class check_flags : public bitset<flags_Max> {
public:
    constexpr check_flags(unsigned long val) : bitset(val) {}
    constexpr check_flags(unsigned long val0, unsigned long val1, unsigned long val2, unsigned long val3, unsigned long val4)
    : check_flags((val0 << 0) | (val1 << 1) | (val2 << 2) | (val3 << 3) | (val4 << 4)) {}
};

struct attribute_item {
    const char*         Name;
    check_flags         Flags;
};

struct element_item {
    const char*         Name;
    check_flags         Flags;
};

enum item {
    item_audioTrack,
    item_audioProgramme,
    item_audioContent,
    item_audioObject,
    item_audioPackFormat,
    item_audioChannelFormat,
    item_audioTrackUID,
    item_audioTrackFormat,
    item_audioStreamFormat,
    item_audioBlockFormat,
    item_alternativeValueSet,
    item_Max
};

struct item_info
{
    const char* Name;
    const char* ID_Begin;
    enum flags {
        Flags_ID_W,
        Flags_ID_YX,
        Flags_ID_Z1, // Z width x2
        Flags_ID_Z2, // Z width x2
        Flags_Max,
    };
    int8u       ID_Flags;
};
#define D(_1,_2,_3) {_1,_2,_3}
#define F(_1) (1 << item_info::Flags_##_1)
static item_info item_Info[] = {
    D("TrackIndex", nullptr, 0),
    D("Programme", "APR", F(ID_W)),
    D("Content", "ACO", F(ID_W)),
    D("Object", "AO", F(ID_W)),
    D("PackFormat", "AP", F(ID_YX)),
    D("ChannelFormat", "AC", F(ID_YX)),
    D("TrackUID", "ATU", 0),
    D("TrackFormat", "AT", F(ID_YX) | F(ID_Z1)),
    D("StreamFormat", "AS", F(ID_YX)),
    D("BlockFormat", "AB", F(ID_YX) | F(ID_Z1) | F(ID_Z2)),
    D("alternativeValueSet", "AVS", F(ID_W) | F(ID_Z2)),
};
#undef D
#undef F
static_assert(sizeof(item_Info) / sizeof(item_info) == item_Max, IncoherencyMessage);

enum audioTrack_String {
    audioTrack_audioTrackID,
    audioTrack_Summary,
    audioTrack_String_Max
};

enum audioTrack_StringVector {
    audioTrack_audioTrackUIDRef,
    audioTrack_StringVector_Max
};

enum audioProgramme_String {
    audioProgramme_audioProgrammeID,
    audioProgramme_audioProgrammeName,
    audioProgramme_audioProgrammeLanguage,
    audioProgramme_start,
    audioProgramme_end,
    audioProgramme_maxDuckingDepth,
    audioProgramme_String_Max
};

enum audioProgramme_StringVector {
    audioProgramme_audioProgrammeLabel,
    audioProgramme_audioContentIDRef,
    audioProgramme_loudnessMetadata,
    audioProgramme_loudnessMetadata_integratedLoudness,
    audioProgramme_audioProgrammeReferenceScreen,
    audioProgramme_authoringInformation,
    audioProgramme_authoringInformation_referenceLayout,
    audioProgramme_authoringInformation_referenceLayout_audioPackFormatIDRef,
    audioProgramme_alternativeValueSetIDRef,
    audioProgramme_StringVector_Max
};

enum audioContent_String {
    audioContent_audioContentID,
    audioContent_audioContentName,
    audioContent_audioContentLanguage,
    audioContent_typeLabel,
    audioContent_String_Max
};

enum audioContent_StringVector {
    audioContent_audioContentLabel,
    audioContent_audioObjectIDRef,
    audioContent_loudnessMetadata,
    audioContent_loudnessMetadata_integratedLoudness,
    audioContent_dialogue,
    audioContent_alternativeValueSetIDRefe,
    audioContent_StringVector_Max
};

enum audioObject_String {
    audioObject_audioObjectID,
    audioObject_audioObjectName,
    audioObject_start,
    audioObject_duration,
    audioObject_dialogue,
    audioObject_importance,
    audioObject_interact,
    audioObject_disableDucking,
    audioObject_String_Max
};

enum audioObject_StringVector {
    audioObject_audioPackFormatIDRef,
    audioObject_audioObjectIDRef,
    audioObject_audioObjectLabel,
    audioObject_audioComplementaryObjectGroupLabel,
    audioObject_audioComplementaryObjectIDRef,
    audioObject_audioTrackUIDRef,
    audioObject_audioObjectInteraction,
    audioObject_gain,
    audioObject_headLocked,
    audioObject_positionOffset,
    audioObject_mute,
    audioObject_alternativeValueSet,
    audioObject_StringVector_Max
};

enum audioPackFormat_String {
    audioPackFormat_audioPackFormatID,
    audioPackFormat_audioPackFormatName,
    audioPackFormat_typeDefinition,
    audioPackFormat_typeLabel,
    audioPackFormat_String_Max
};

enum audioPackFormat_StringVector {
    audioPackFormat_audioChannelFormatIDRef,
    audioPackFormat_audioPackFormatIDRef,
    audioPackFormat_absoluteDistance,
    audioPackFormat_encodePackFormatIDRef,
    audioPackFormat_decodePackFormatIDRef,
    audioPackFormat_inputPackFormatIDRef,
    audioPackFormat_outputPackFormatIDRef,
    audioPackFormat_normalization,
    audioPackFormat_nfcRefDist,
    audioPackFormat_screenRef,
    audioPackFormat_StringVector_Max
};

enum audioChannelFormat_String {
    audioChannelFormat_audioChannelFormatID,
    audioChannelFormat_audioChannelFormatName,
    audioChannelFormat_typeDefinition,
    audioChannelFormat_typeLabel,
    audioChannelFormat_String_Max
};

enum audioChannelFormat_StringVector {
    audioChannelFormat_StringVector_Max
};

enum audioTrackUID_String {
    audioTrackUID_UID,
    audioTrackUID_sampleRate,
    audioTrackUID_bitDepth,
    audioTrackUID_String_Max
};

enum audioTrackUID_StringVector {
    audioTrackUID_audioChannelFormatIDRef,
    audioTrackUID_audioPackFormatIDRef,
    audioTrackUID_audioTrackFormatIDRef,
    audioTrackUID_StringVector_Max
};

enum audioTrackFormat_String {
    audioTrackFormat_audioTrackFormatID,
    audioTrackFormat_audioTrackFormatName,
    audioTrackFormat_formatLabel,
    audioTrackFormat_formatDefinition,
    audioTrackFormat_String_Max
};

enum audioTrackFormat_StringVector {
    audioTrackFormat_audioStreamFormatIDRef,
    audioTrackFormat_StringVector_Max
};

enum audioStreamFormat_String {
    audioStreamFormat_audioStreamFormatID,
    audioStreamFormat_audioStreamFormatName,
    audioStreamFormat_formatLabel,
    audioStreamFormat_formatDefinition,
    audioStreamFormat_String_Max
};

enum audioStreamFormat_StringVector {
    audioStreamFormat_audioChannelFormatIDRef,
    audioStreamFormat_audioPackFormatIDRef,
    audioStreamFormat_audioTrackFormatIDRef,
    audioStreamFormat_StringVector_Max
};

enum error_Type {
    Error,
    Warning,
    error_Type_Max,
};

static const char* error_Type_String[] = {
    "Errors",
    "Warnings",
};
                                                // Version |Count
                                                // 0  1  2 |0  1  2+
                                                // 0  1  2 |0  1  2+
static attribute_item audioTrack_Attributes[] =
{
    { "audioTrackID"                            , {1, 1, 1, 1, 0} },
    { "Summary"                                 , {1, 1, 1, 1, 0} },
};
static_assert(sizeof(audioTrack_Attributes) / sizeof(attribute_item) == audioTrack_String_Max, IncoherencyMessage);

static element_item audioTrack_Elements[] =
{
    { "audioTrackUIDRef"                        , {1, 1, 1, 1, 1} },
};
static_assert(sizeof(audioTrack_Elements) / sizeof(attribute_item) == audioTrack_StringVector_Max, IncoherencyMessage);

static attribute_item audioProgramme_Attributes[] =
{
    { "audioProgrammeID"                        , {1, 1, 1, 0, 0} },
    { "audioProgrammeName"                      , {1, 1, 1, 0, 0} },
    { "audioProgrammeLanguage"                  , {1, 1, 1, 1, 0} },
    { "start"                                   , {1, 1, 1, 1, 0} },
    { "end"                                     , {1, 1, 1, 1, 0} },
    { "maxDuckingDepth"                         , {1, 1, 1, 1, 0} },
};
static_assert(sizeof(audioProgramme_Attributes) / sizeof(attribute_item) == audioProgramme_String_Max, IncoherencyMessage);

static element_item audioProgramme_Elements[] =
{
    { "audioProgrammeLabel"                     , {0, 0, 1, 1, 1} },
    { "audioContentIDRef"                       , {1, 1, 1, 0, 1} },
    { "loudnessMetadata"                        , {1, 1, 1, 1, 1} },
    { "integratedLoudness"                      , {1, 1, 1, 1, 1} },
    { "audioProgrammeReferenceScreen"           , {1, 1, 1, 1, 0} },
    { "authoringInformation"                    , {0, 0, 1, 1, 0} },
    { "referenceLayout"                         , {0, 0, 1, 1, 0} },
    { "audioPackFormatIDRef"                    , {0, 0, 1, 1, 0} },
    { "alternativeValueSetIDRef"                , {0, 0, 1, 1, 1} },
};
static_assert(sizeof(audioProgramme_Elements) / sizeof(attribute_item) == audioProgramme_StringVector_Max, IncoherencyMessage);

static attribute_item audioContent_Attributes[] =
{
    { "audioContentID"                          , {1, 1, 1, 0, 0} },
    { "audioContentName"                        , {1, 1, 1, 0, 0} },
    { "audioContentLanguage"                    , {1, 1, 1, 1, 0} },
    { "typeLabel"                               , {1, 1, 1, 1, 0} },
};
static_assert(sizeof(audioContent_Attributes) / sizeof(attribute_item) == audioContent_String_Max, IncoherencyMessage);

static element_item audioContent_Elements[] =
{
    { "audioContentLabel"                       , {1, 1, 1, 1, 1} },
    { "audioObjectIDRef"                        , {1, 1, 1, 0, 1} },
    { "loudnessMetadata"                        , {1, 1, 1, 1, 1} },
    { "loudnessMetadata_integratedLoudness"     , {1, 1, 1, 1, 1} },
    { "dialogue"                                , {1, 1, 1, 1, 0} },
    { "alternativeValueSetIDRef"                , {1, 1, 1, 1, 1} },
};
static_assert(sizeof(audioContent_Elements) / sizeof(attribute_item) == audioContent_StringVector_Max, IncoherencyMessage);

static attribute_item audioObject_Attributes[] =
{
    { "audioObjectID"                           , {1, 1, 1, 0, 0} },
    { "audioObjectName"                         , {1, 1, 1, 0, 0} },
    { "start"                                   , {1, 1, 1, 1, 0} },
    { "duration"                                , {1, 1, 1, 1, 0} },
    { "dialogue"                                , {1, 1, 1, 1, 0} },
    { "importance"                              , {1, 1, 1, 1, 0} },
    { "interact"                                , {1, 1, 1, 1, 0} },
    { "disableDucking"                          , {1, 1, 1, 1, 0} },
};
static_assert(sizeof(audioObject_Attributes) / sizeof(attribute_item) == audioObject_String_Max, IncoherencyMessage);

static element_item audioObject_Elements[] =
{
    { "audioPackFormatIDRef"                    , {1, 1, 1, 1, 1} },
    { "audioTrackUIDRef"                        , {1, 1, 1, 1, 1} },
    { "audioObjectLabel"                        , {1, 1, 1, 1, 1} },
    { "audioComplementaryObjectGroupLabel"      , {1, 1, 1, 1, 1} },
    { "audioComplementaryObjectIDRef"           , {1, 1, 1, 1, 1} },
    { "audioTrackUIDRef"                        , {1, 1, 1, 1, 1} },
    { "audioObjectInteraction"                  , {1, 1, 1, 1, 0} },
    { "gain"                                    , {1, 1, 1, 1, 0} },
    { "headLocked"                              , {1, 1, 1, 1, 0} },
    { "positionOffset"                          , {1, 1, 1, 1, 0} },
    { "mute"                                    , {1, 1, 1, 1, 0} },
    { "alternativeValueSet"                     , {1, 1, 1, 1, 1} },
};
static_assert(sizeof(audioObject_Elements) / sizeof(attribute_item) == audioObject_StringVector_Max, IncoherencyMessage);

static attribute_item audioPackFormat_Attributes[] =
{
    { "audioPackFormatID"                       , {1, 1, 1, 0, 0} },
    { "audioPackFormatName"                     , {1, 1, 1, 0, 0} },
    { "typeDefinition"                          , {1, 1, 1, 1, 0} },
    { "typeLabel"                               , {1, 1, 1, 1, 0} },
};
static_assert(sizeof(audioPackFormat_Attributes) / sizeof(attribute_item) == audioPackFormat_String_Max, IncoherencyMessage);

static element_item audioPackFormat_Elements[] =
{
    { "audioChannelFormatIDRef"                 , {1, 1, 1, 1, 1} },
    { "audioPackFormatIDRef"                    , {1, 1, 1, 1, 1} },
    { "absoluteDistance"                        , {1, 1, 1, 1, 0} },
    { "encodePackFormatIDRef"                   , {1, 1, 1, 1, 1} },
    { "decodePackFormatIDRef"                   , {1, 1, 1, 1, 1} },
    { "inputPackFormatIDRef"                    , {1, 1, 1, 1, 0} },
    { "outputPackFormatIDRef"                   , {1, 1, 1, 1, 0} },
    { "normalization"                           , {1, 1, 1, 1, 0} },
    { "nfcRefDist"                              , {1, 1, 1, 1, 0} },
    { "screenRef"                               , {1, 1, 1, 1, 0} },
};
static_assert(sizeof(audioPackFormat_Elements) / sizeof(attribute_item) == audioPackFormat_StringVector_Max, IncoherencyMessage);

static attribute_item audioChannelFormat_Attributes[] =
{
    { "audioChannelFormatID"                    , {1, 1, 1, 0, 0} },
    { "audioChannelFormatName"                  , {1, 1, 1, 0, 0} },
    { "typeDefinition"                          , {1, 1, 1, 1, 0} },
    { "typeLabel"                               , {1, 1, 1, 1, 0} },
};
static_assert(sizeof(audioChannelFormat_Attributes) / sizeof(attribute_item) == audioChannelFormat_String_Max, IncoherencyMessage);

static element_item audioChannelFormat_Elements[] =
{
    { nullptr                                   , {0, 0, 0, 0, 0} },
};
static_assert(sizeof(audioChannelFormat_Elements) / sizeof(attribute_item) == audioChannelFormat_StringVector_Max + 1, IncoherencyMessage);

static attribute_item audioTrackUID_Attributes[] =
{
    { "UID"                                     , {1, 1, 1, 0, 0} },
    { "sampleRate"                              , {1, 1, 1, 1, 0} },
    { "bitDepth"                                , {1, 1, 1, 1, 0} },
};
static_assert(sizeof(audioTrackUID_Attributes) / sizeof(attribute_item) == audioTrackUID_String_Max, IncoherencyMessage);

static element_item audioTrackUID_Elements[] =
{
    { "audioChannelFormatIDRef"                 , {1, 1, 1, 1, 0} },
    { "audioPackFormatIDRef"                    , {1, 1, 1, 1, 0} },
    { "audioTrackFormatIDRef"                   , {1, 1, 1, 1, 0} },
};
static_assert(sizeof(audioTrackUID_Elements) / sizeof(attribute_item) == audioTrackUID_StringVector_Max, IncoherencyMessage);

static attribute_item audioTrackFormat_Attributes[] =
{
    { "audioTrackFormatID"                      , {1, 1, 1, 0, 0} },
    { "audioTrackFormatName"                    , {1, 1, 1, 0, 0} },
    { "formatLabel"                             , {1, 1, 1, 1, 0} },
    { "formatDefinition"                        , {1, 1, 1, 1, 0} },
};
static_assert(sizeof(audioTrackFormat_Attributes) / sizeof(attribute_item) == audioTrackFormat_String_Max, IncoherencyMessage);

static element_item audioTrackFormat_Elements[] =
{
    { "audioStreamFormatIDRef"                  , {1, 1, 1, 0, 0} },
};
static_assert(sizeof(audioTrackFormat_Elements) / sizeof(attribute_item) == audioTrackFormat_StringVector_Max, IncoherencyMessage);

static attribute_item audioStreamFormat_Attributes[] =
{
    { "audioStreamFormatID"                     , {1, 1, 1, 0, 0} },
    { "audioStreamFormatName"                   , {1, 1, 1, 0, 0} },
    { "formatLabel"                             , {0, 1, 1, 1, 0} },
    { "formatDefinition"                        , {0, 0, 1, 1, 0} },
};
static_assert(sizeof(audioStreamFormat_Attributes) / sizeof(attribute_item) == audioStreamFormat_String_Max, IncoherencyMessage);

static element_item audioStreamFormat_Elements[] =
{
    { "audioChannelFormatIDRef"                 , {1, 1, 1, 1, 1} },
    { "audioPackFormatIDRef"                    , {1, 1, 1, 1, 1} },
    { "audioTrackFormatIDRef"                   , {1, 1, 1, 1, 1} },
};
static_assert(sizeof(audioStreamFormat_Elements) / sizeof(attribute_item) == audioStreamFormat_StringVector_Max, IncoherencyMessage);

struct Item_Struct {
    vector<string> Strings;
    vector<vector<string> > StringVectors;
    map<string, string> Extra;
    vector<string> Errors[error_Type_Max];

    void AddError(error_Type Type, const string& NewValue)
    {
        if (find(Errors[Type].begin(), Errors[Type].end(), NewValue) == Errors[Type].end()) {
            Errors[Type].push_back(NewValue);
        }
    }
};

struct Items_Struct {
    void Init(size_t Strings_Size_, size_t StringVectors_Size_) {
        Strings_Size = Strings_Size_;
        StringVectors_Size =StringVectors_Size_;
    }

    Item_Struct& New(bool Init = true)
    {
        if (!Init) {
            return Items.back();
        }
        Items.resize(Items.size() + 1);
        Item_Struct& Item = Items[Items.size() - 1];
        Item.Strings.resize(Strings_Size);
        Item.StringVectors.resize(StringVectors_Size);
        return Item;
    }

    Item_Struct& Last()
    {
        Item_Struct& Item = Items[Items.size() - 1];
        return Item;
    }

    vector<Item_Struct> Items;
    size_t Strings_Size;
    size_t StringVectors_Size;
};

static string Apply_Init(File__Analyze& F, const char* Name, size_t i, const Items_Struct& audioProgramme_List, Ztring Summary) {
    const Item_Struct& audioProgramme = audioProgramme_List.Items[i];
    string P = Name + to_string(i);
    F.Fill(Stream_Audio, 0, P.c_str(), Summary.empty() ? __T("Yes") : Summary);
    F.Fill(Stream_Audio, 0, (P + " Pos").c_str(), i);
    F.Fill_SetOptions(Stream_Audio, 0, (P + " Pos").c_str(), "N NIY");
    return P;
}

static void Apply_SubStreams(File__Analyze& F, const string& P_And_LinkedTo, Item_Struct& Source, size_t i, Items_Struct& Dest) {
    ZtringList SubstreamPos, SubstreamNum;
    for (size_t j = 0; j < Source.StringVectors[i].size(); j++) {
        const string& ID = Source.StringVectors[i][j];
        size_t Pos = -1;
        for (size_t k = 0; k < Dest.Items.size(); k++) {
            if (Dest.Items[k].Strings[0] == ID) {
                Pos = k;
                break;
            }
        }
        if (Pos == -1) {
            auto LinkedTo_Pos = P_And_LinkedTo.find(" LinkedTo_TrackUID_Pos");
            auto HasTransport = !F.Retrieve_Const(Stream_Audio, 0, "Transport0").empty();
            if (HasTransport && LinkedTo_Pos != string::npos) { // TODO: better way to avoid common definitions
                string Message;
                if (LinkedTo_Pos) {
                    auto Sub_Pos = P_And_LinkedTo.rfind(' ', LinkedTo_Pos - 1);
                    if (Sub_Pos != string::npos) {
                        Message += ":transportTrackFormat:audioTrack:audioTrackUIDRef:\"";
                        Message += ID;
                        Message += "\" is referenced but its description is missing";
                        Source.Errors[Warning].push_back(Message);
                    }
                }
            }
            continue;
        }
        SubstreamPos.push_back(Ztring::ToZtring(Pos));
        SubstreamNum.push_back(Ztring::ToZtring(Pos + 1));
    }
    if (SubstreamPos.empty())
        return;
    SubstreamPos.Separator_Set(0, __T(" + "));
    F.Fill(Stream_Audio, 0, P_And_LinkedTo.c_str(), SubstreamPos.Read());
    F.Fill_SetOptions(Stream_Audio, 0, P_And_LinkedTo.c_str(), "N NIY");
    SubstreamNum.Separator_Set(0, __T(" + "));
    F.Fill(Stream_Audio, 0, (P_And_LinkedTo + "/String").c_str(), SubstreamNum.Read());
    F.Fill_SetOptions(Stream_Audio, 0, (P_And_LinkedTo + "/String").c_str(), "Y NIN");
}


//***************************************************************************
// Private class
//***************************************************************************

static void CheckErrors_Formating(file_adm_private* File_Adm_Private, const string& ID, const item_info& ID_Start, vector<string>& Errors, const char* Sub = nullptr);
static void CheckErrors_Formating(file_adm_private* File_Adm_Private, Items_Struct& MainItem, const item_info& ID_Start, const char* Sub = nullptr);

class tfsxml
{
public:
    void Enter();
    void Leave();
    int Init(const void* const Buffer, size_t Buffer_Size);
    int NextElement();
    int Attribute();
    int Value();
    bool IsInit() { return IsInit_; }
    size_t Remain() { return p.len < 0 ? 0 : (size_t)p.len; }
    void SetSavedAttribute(int Pos) { Attributes_[Level].insert({Pos, tfsxml_decode(v)}); }
    const map<int, string>::iterator GetSavedAttribute(int Pos, int UpperLevel = 0) { return Attributes_[Level - UpperLevel].find(Pos); }
    const map<int, string>::iterator NoSavedAttribute(int UpperLevel = 0) { return Attributes_[Level - UpperLevel].end(); }

private:
    tfsxml_string p;

public:
    tfsxml_string b, v;

//private:
    string Code[16];
    map<int, string> Attributes_[16];
    int8u Level = 0;
    int8u Level_Max = 0;
    bool IsInit_ = false;
    bool MustEnter = false;
    bool ParsingAttr = false;
};

class file_adm_private : public tfsxml
{
public:
    // In
    bool IsSub;

    // Out
    Items_Struct Items[item_Max];
    int Version = 0;
    int Version_S = -1; // S-ADM
    bool DolbyProfileCanNotBeVersion1 = false;
    vector<profile_info> profileInfos;
    map<string, string> More;
    float32 FrameRate_Sum = 0;
    float32 FrameRate_Den = 0;

    file_adm_private()
    {
    }

    // Actions
    int parse(const void* const Buffer, size_t Buffer_Size);
    void clear();

    // Helpers
    void chna_Add(int32u Index, const string& TrackUID)
    {
        if (Index >= 0x10000)
            return;
        if (Items[item_audioTrack].Items.empty())
            Items[item_audioTrack].Init(audioTrack_String_Max, audioTrack_StringVector_Max);
        while (Items[item_audioTrack].Items.size() <= Index)
            Items[item_audioTrack].New();
        Item_Struct& Item = Items[item_audioTrack].Items[Items[item_audioTrack].Items.size() - 1];
        Item.StringVectors[audioTrack_audioTrackUIDRef].push_back(TrackUID);
    }

    void Check_Attributes_NotPartOfSpecs(size_t Type, size_t Pos, const tfsxml_string& b, Item_Struct& Content)
    {
        Content.AddError(Warning, ":audio" + string(item_Info[Type].Name) + to_string(Pos) + ":XmlAttributes:\"" + tfsxml_decode(b) + "\" is not part of specs");
    }

    void Check_Elements_NotPartOfSpecs(size_t Type, size_t Pos, const tfsxml_string& b, Item_Struct& Content)
    {
        Content.AddError(Warning, ":audio" + string(item_Info[Type].Name) + to_string(Pos) + ":XmlElements:\"" + tfsxml_decode(b) + "\" is not part of specs"); \
    }

    void Check_Attributes_ShallBePresent(size_t Type, size_t Pos, const vector<size_t>& Attributes_Counts, const attribute_item* const Attributes, Item_Struct& Content)
    {
        for (size_t i = 0; i < Attributes_Counts.size(); ++i) {
            if (Attributes_Counts[i] == 0 && !(Attributes[i].Flags[Count0])) {
                Content.AddError(Error, ":audio" + string(item_Info[Type].Name) + to_string(Pos) + ":XmlAttributes:\"" + string(Attributes[i].Name) + "\" attribute shall be present");
            }
            if (Attributes_Counts[i] > 1) {
                Content.AddError(Error, ":audio" + string(item_Info[Type].Name) + to_string(Pos) + ":XmlAttributes:\"" + string(Attributes[i].Name) + "\" attribute shall be unique");
            }
        }
    }

    void Check_Elements_ShallBePresent(size_t Type, size_t Pos, const vector<size_t>& Elements_Counts, const element_item* const Elements, Item_Struct& Content)
    {
        for (size_t i = 0; i < Elements_Counts.size(); ++i) {
            if (Elements_Counts[i] == 0 && !(Elements[i].Flags[Count0])) {
                Content.AddError(Error, ":audio" + string(item_Info[Type].Name) + to_string(Pos) + ":XmlElements:\"" + string(Elements[i].Name) + "\" element shall be present");
            }
            if (Elements_Counts[i] > 1 && !(Elements[i].Flags[Count2])) {
                Content.AddError(Error, ":audio" + string(item_Info[Type].Name) + to_string(Pos) + ":XmlElements:\"" + string(Elements[i].Name) + "\" element shall be unique");
            }
        }
    }

//private:

    int coreMetadata();
    int format();
    int audioFormatExtended();
    int frameHeader();
    int transportTrackFormat();

    // Common definitions
    vector<string> audioChannelFormatIDRefs;

    // Temp
    vector<size_t> Elements_Counts[16];
    vector<size_t> Attributes_Counts[16];
};

void tfsxml::Enter() {
    if (Level == Level_Max) {
        if (MustEnter) {
            return;
        }
        MustEnter = true;
        Level++;
        Level_Max = Level;
    }
    else {
        Level++;
    }
}

void tfsxml::Leave() {
    IsInit_ = false;
    MustEnter = false;
    if (Level != (decltype(Level))-1) {
        Level--;
        Level_Max = Level;
    }
}


int tfsxml::Init(const void* const Buffer, size_t Buffer_Size) {
    if (!p.buf)
    {
        int Result = tfsxml_init(&p, Buffer, Buffer_Size, 0);
        if (Result) {
            return Result;
        }
    }
    else
    {
        p.buf = (const char*)Buffer;
        p.len = Buffer_Size;
    }
    return 0;
}

int tfsxml::NextElement() {
    IsInit_ = false;
    if (MustEnter && Level == Level_Max + ParsingAttr) {
        int Result;
        if (Level > 1)
            Result = tfsxml_enter(&p);
        else
            Result = 0;
        if (Result > 0) {
            Level = 0;
            return Result;
        }
        MustEnter = false;
        if (Result) {
            return Result;
        }
    }
    if (Level == Level_Max + ParsingAttr) {
        auto Result = tfsxml_next(&p, &b);
        if (Result < 0) {
            return Result;
        }
        if (Result > 0) {
            Level = 0;
            return Result;
        }
        IsInit_ = true;
        Attributes_[Level].clear();
        Code[Level] = tfsxml_decode(b);
        Code[Level + 1].clear();
    }
    else {
        b.buf = Code[Level].c_str();
        b.len = Code[Level].size();
    }
    return 0;
}

int tfsxml::Attribute() {
    IsInit_ = false;
    if (Level == Level_Max) {
        auto Result = tfsxml_attr(&p, &b, &v);
        if (Result > 0) {
            ParsingAttr = true;
            Level = 0;
            return Result;
        }
        ParsingAttr = false;
        return Result;
    }
    return -1;
}

int tfsxml::Value() {
    auto Result = tfsxml_value(&p, &b);
    if (Result > 0) {
        ParsingAttr = true;
        Level = 0;
        return Result;
    }
    ParsingAttr = false;
    return Result;
}

int file_adm_private::parse(const void* const Buffer, size_t Buffer_Size)
{
    # define STRUCTS(NAME) \
        Items[item_##NAME].Init(NAME##_String_Max, NAME##_StringVector_Max);

    STRUCTS(audioTrack);
    STRUCTS(audioProgramme);
    STRUCTS(audioContent);
    STRUCTS(audioObject);
    STRUCTS(audioPackFormat);
    STRUCTS(audioChannelFormat);
    STRUCTS(audioTrackUID);
    STRUCTS(audioTrackFormat);
    STRUCTS(audioStreamFormat);

 
    #define XML_BEGIN \

    #define XML_ATTR_START \
        for (;;) { \
            { \
                int Result = Attribute(); \
                if (Result < 0) { \
                    break; \
                } \
                if (Result > 0) { \
                    return Result; \
                } \
            } \
            if (false) { \
            } \

    #define XML_ATTR_END \
        } \

    #define XML_VALUE \
        { \
            auto Result = Value(); \
            if (Result > 0) { \
                return Result; \
            } \
        } \

    #define XML_ELEM_START \
        Enter(); \
        for (;;) { \
            { \
                int Result = NextElement(); \
                if (Result < 0) { \
                    break; \
                } \
                if (Result > 0) { \
                    return Result; \
                } \
            } \
            if (false) { \
            } \

    #define XML_ELEM_END \
        } \
        Leave(); \

    #define XML_SUB(_A) \
        { \
            int Result = _A; \
            if (Result > 0) { \
                return Result; \
            } \
        } \

    #define XML_END \
        return -1;
   
    #define ELEMENT_START(NAME) \
        if (!tfsxml_strcmp_charp(b, #NAME)) \
        { \
            Item_Struct& NAME##_Content = Items[item_##NAME].New(IsInit()); \
            if (IsInit()) { \
                Attributes_Counts[Level].clear(); \
                Attributes_Counts[Level].resize(NAME##_String_Max); \
            } \
            XML_ATTR_START \

    #define ELEMENT_MIDDLE(NAME) \
                else if (!Items[item_##NAME].Items.empty() && &NAME##_Content == &Items[item_##NAME].Items.back()) { \
                    Check_Attributes_NotPartOfSpecs(item_##NAME, Items[item_##NAME].Items.size() - 1, b, NAME##_Content); \
                } \
            XML_ATTR_END \
            Check_Attributes_ShallBePresent(item_##NAME, Items[item_##NAME].Items.size() - 1, Attributes_Counts[Level], NAME##_Attributes, NAME##_Content); \
            if (Level == Level_Max) { \
                Elements_Counts[Level + 1].clear(); \
                Elements_Counts[Level + 1].resize(NAME##_StringVector_Max); \
            } \
            XML_ELEM_START \

    #define ELEMENT_END(NAME) \
                else if (!Items[item_##NAME].Items.empty() && &NAME##_Content == &Items[item_##NAME].Items.back()) { \
                    Check_Elements_NotPartOfSpecs(item_##NAME, Items[item_##NAME].Items.size() - 1, b, NAME##_Content); \
                } \
            XML_ELEM_END \
            if (Level == Level_Max) { \
                Check_Elements_ShallBePresent(item_##NAME, Items[item_##NAME].Items.size() - 1, Elements_Counts[Level + 1], NAME##_Elements, NAME##_Content); \
            } \
        } \

    #define ATTRIBUTE(NAME,ATTR) \
        else if (!tfsxml_strcmp_charp(b, #ATTR)) { \
            NAME##_Content.Strings[NAME##_##ATTR].assign(tfsxml_decode(v)); \
            Attributes_Counts[Level][NAME##_##ATTR]++; \
        } \

    #define ATTRIB_ID(NAME,ATTR) \
        else if (!tfsxml_strcmp_charp(b, #ATTR)) { \
            CheckErrors_Formating(this, tfsxml_decode(v), item_Info[item_##NAME], *this->Items[item_##NAME].Items.back().Errors, #NAME":"#ATTR); \
            NAME##_Content.Strings[NAME##_##ATTR].assign(tfsxml_decode(v)); \
            Attributes_Counts[Level][NAME##_##ATTR]++; \
        } \

    #define ATTRIBUTE_I(NAME,ATTR) \
        else if (!tfsxml_strcmp_charp(b, #ATTR)) { \
            Attributes_Counts[NAME##_##ATTR]++; \
        } \

    #define ELEMENT(NAME,ELEM) \
        else if (!tfsxml_strcmp_charp(b, #ELEM)) { \
            XML_VALUE \
            NAME##_Content.StringVectors[NAME##_##ELEM].push_back(tfsxml_decode(b)); \
            Elements_Counts[Level][NAME##_##ELEM]++; \
        } \

    #define ELEMENT_I(NAME,ELEM) \
        else if (!tfsxml_strcmp_charp(b, #ELEM)) { \
            Elements_Counts[Level][NAME##_##ELEM]++; \
        } \

    if (auto Result = Init(Buffer, Buffer_Size)) {
        return Result;
    }

    XML_BEGIN
    XML_ELEM_START
        if (!tfsxml_strcmp_charp(b, "audioFormatExtended"))
        {
            XML_SUB(audioFormatExtended())
        }
        if (!tfsxml_strcmp_charp(b, "ebuCoreMain"))
        {
            XML_ATTR_START
                if (!tfsxml_strcmp_charp(b, "xmlns") || !tfsxml_strcmp_charp(b, "xsi:schemaLocation")) {
                    DolbyProfileCanNotBeVersion1 = false;
                    if (!tfsxml_strstr_charp(v, "ebuCore_2014").len && !tfsxml_strstr_charp(v, "ebuCore_2016").len) {
                        DolbyProfileCanNotBeVersion1 = true;
                    }
                }
            XML_ATTR_END
            XML_ELEM_START
                    if (!tfsxml_strcmp_charp(b, "coreMetadata"))
                    {
                        XML_SUB(coreMetadata());
                    }
            XML_ELEM_END
        }
        if (!tfsxml_strcmp_charp(b, "frame"))
        {
            if (IsInit()) {
                Version_S = 0;
            }
            XML_ATTR_START
                if (!tfsxml_strcmp_charp(b, "version")) {
                    if (!tfsxml_strcmp_charp(v, "ITU-R_BS.2125-1")) {
                        Version_S = 1;
                    }
                    if (!tfsxml_strcmp_charp(v, "ITU-R_BS.2125-2")) {
                        Version_S = 2;
                    }
                }
            XML_ATTR_END
            XML_SUB(format());
        }
        if (!tfsxml_strcmp_charp(b, "format"))
        {
            XML_SUB(format());
        }
    XML_ELEM_END
    XML_END
}


void file_adm_private::clear()
{
    for (auto& Item : Items)
        Item = Items_Struct();
    profileInfos.clear();
}

int file_adm_private::coreMetadata()
{
    XML_BEGIN
    XML_ELEM_START
        if (!tfsxml_strcmp_charp(b, "format"))
        {
            XML_SUB(format());
        }
    XML_ELEM_END
    XML_END
}

int file_adm_private::format()
{
    XML_BEGIN
    XML_ELEM_START
        if (!tfsxml_strcmp_charp(b, "audioFormatCustom")) {
            XML_ELEM_START
                if (!tfsxml_strcmp_charp(b, "audioFormatCustomSet")) {
                    XML_ELEM_START
                        if (!tfsxml_strcmp_charp(b, "admInformation")) {
                            if (IsInit()) {
                                profileInfos.clear();
                            }
                            XML_ELEM_START
                                if (!tfsxml_strcmp_charp(b, "profile")) {
                                    if (IsInit()) {
                                        profileInfos.resize(profileInfos.size() + 1);
                                    }
                                    profile_info& profileInfo = profileInfos[profileInfos.size() - 1];
                                    XML_ATTR_START
                                        for (size_t i = 0; i < profile_names_size; i++)
                                        {
                                            if (!tfsxml_strcmp_charp(b, profile_names[i])) {
                                                profileInfo.Strings[i] = tfsxml_decode(v);
                                                if (!i && profileInfo.Strings[0].size() >= 12 && !profileInfo.Strings[0].compare(profileInfo.Strings[0].size() - 12, 12, " ADM Profile"))
                                                    profileInfo.Strings[0].resize(profileInfo.Strings[0].size() - 12);
                                            }
                                        }
                                    XML_ATTR_END
                                }
                            XML_ELEM_END
                        }
                    XML_ELEM_END
                }
            XML_ELEM_END
        }
        if (!tfsxml_strcmp_charp(b, "audioFormatExtended")) {
            XML_SUB(audioFormatExtended());
        }
        if (!tfsxml_strcmp_charp(b, "frameHeader")) {
            XML_SUB(frameHeader());
        }
    XML_ELEM_END
    XML_END
}

int file_adm_private::audioFormatExtended()
{
    if (IsInit()) {
        for (size_t i = 0; i < item_Max; i++) {
            if (i == item_audioTrack) {
                continue;
            }
            auto& Item = Items[i];
            Item.Items.clear();
        }
    }

    XML_BEGIN
    XML_ATTR_START
        if (!tfsxml_strcmp_charp(b, "version")) {
            if (!tfsxml_strcmp_charp(v, "ITU-R_BS.2076-1")) {
                Version = 1;
            }
            if (!tfsxml_strcmp_charp(v, "ITU-R_BS.2076-2")) {
                Version = 2;
            }
        }
    XML_ATTR_END
    XML_ELEM_START
        ELEMENT_START(audioProgramme)
            ATTRIB_ID(audioProgramme, audioProgrammeID)
            ATTRIBUTE(audioProgramme, audioProgrammeName)
            ATTRIBUTE(audioProgramme, audioProgrammeLanguage)
            ATTRIBUTE(audioProgramme, start)
            ATTRIBUTE(audioProgramme, end)
            ATTRIBUTE(audioProgramme, maxDuckingDepth)
        ELEMENT_MIDDLE(audioProgramme)
            else if (!tfsxml_strcmp_charp(b, "audioProgrammeLabel")) {
                XML_ATTR_START
                    if (!tfsxml_strcmp_charp(b, "language")) {
                        SetSavedAttribute(0);
                    }
                XML_ATTR_END
                XML_VALUE
                string Value = tfsxml_decode(b);
                if (!Value.empty() && GetSavedAttribute(0) != NoSavedAttribute()) {
                    Value.insert(0, '(' + GetSavedAttribute(0)->second + ')');
                }
                audioProgramme_Content.StringVectors[audioProgramme_audioProgrammeLabel].push_back(Value);
            }
            ELEMENT(audioProgramme, audioContentIDRef)
            else if (!tfsxml_strcmp_charp(b, "loudnessMetadata")) {
                XML_ELEM_START
                        if (!tfsxml_strcmp_charp(b, "integratedLoudness")) {
                            XML_VALUE
                            audioProgramme_Content.StringVectors[audioProgramme_loudnessMetadata_integratedLoudness].push_back(tfsxml_decode(b));
                        }
                XML_ELEM_END
            }
            ELEMENT(audioProgramme, audioProgrammeReferenceScreen)
            else if (!tfsxml_strcmp_charp(b, "authoringInformation")) {
                XML_ELEM_START
                        if (!tfsxml_strcmp_charp(b, "referenceLayout")) {
                            XML_ELEM_START
                                    if (!tfsxml_strcmp_charp(b, "audioPackFormatIDRef")) {
                                        XML_VALUE
                                        audioProgramme_Content.StringVectors[audioProgramme_authoringInformation_referenceLayout_audioPackFormatIDRef].push_back(tfsxml_decode(b));
                                    }
                            XML_ELEM_END
                        }
                XML_ELEM_END
            }
            ELEMENT(audioProgramme, alternativeValueSetIDRef)
        ELEMENT_END(audioProgramme)
        ELEMENT_START(audioContent)
            ATTRIB_ID(audioContent, audioContentID)
            ATTRIBUTE(audioContent, audioContentName)
            ATTRIBUTE(audioContent, audioContentLanguage)
            ATTRIBUTE(audioContent, typeLabel)
        ELEMENT_MIDDLE(audioContent)
            ELEMENT(audioContent, audioObjectIDRef)
            else if (!tfsxml_strcmp_charp(b, "dialogue")) {
                XML_ATTR_START
                    if (!tfsxml_strcmp_charp(b, "nonDialogueContentKind")
                        || !tfsxml_strcmp_charp(b, "dialogueContentKind")
                        || !tfsxml_strcmp_charp(b, "mixedContentKind")) {
                        SetSavedAttribute(0);
                    }
                XML_ATTR_END
                XML_VALUE
                string Value;
                string Kind;
                auto KindP = GetSavedAttribute(0);
                if (KindP != NoSavedAttribute())
                    Kind = KindP->second;
                if (!tfsxml_strcmp_charp(b, "0")) {
                    if (Kind == "1") {
                        Value = "Music";
                    }
                    else if (Kind == "2") {
                        Value = "Effect";
                    }
                    else {
                        Value = "No Dialogue";
                        if (!Kind.empty() && Kind != "0") {
                            Value += " (" + Kind + ')';
                        }
                    }
                }
                else if (!tfsxml_strcmp_charp(b, "1")) {
                    if (Kind == "1") {
                        Value = "Music";
                    }
                    else if (Kind == "2") {
                        Value = "Effect";
                    }
                    else if (Kind == "3") {
                        Value = "Spoken Subtitle";
                    }
                    else if (Kind == "4") {
                        Value = "Visually Impaired";
                    }
                    else if (Kind == "5") {
                        Value = "Commentary";
                    }
                    else if (Kind == "6") {
                        Value = "Emergency";
                    }
                    else {
                        Value = "Dialogue";
                        if (!Kind.empty() && Kind != "0") {
                            Value += " (" + Kind + ')';
                        }
                    }
                }
                else if (!tfsxml_strcmp_charp(b, "2")) {
                    if (Kind == "1") {
                        Value = "Complete Main";
                    }
                    else if (Kind == "2") {
                        Value = "Mixed (Mixed)";
                    }
                    else if (Kind == "3") {
                        Value = "Hearing Impaired";
                    }
                    else {
                        Value = "Mixed";
                        if (!Kind.empty() && Kind != "0") {
                            Value += " (" + Kind + ')';
                        }
                    }
                }
                else {
                    Value = tfsxml_decode(b);
                    if (!Kind.empty()) {
                        Value += " (" + Kind + ')';
                    }
                }
                audioContent_Content.StringVectors[audioContent_dialogue].push_back(Value);
            }
            else if (!tfsxml_strcmp_charp(b, "audioContentLabel")) {
                XML_ATTR_START
                    if (!tfsxml_strcmp_charp(b, "language")) {
                        SetSavedAttribute(0);
                    }
                XML_ATTR_END
                XML_VALUE
                string Value = tfsxml_decode(b);
                if (!Value.empty() && GetSavedAttribute(0) != NoSavedAttribute()) {
                    Value.insert(0, '(' + GetSavedAttribute(0)->second + ')');
                }
                audioContent_Content.StringVectors[audioContent_audioContentLabel].push_back(Value);
            }
            else if (!tfsxml_strcmp_charp(b, "loudnessMetadata")) {
                XML_ELEM_START
                        if (!tfsxml_strcmp_charp(b, "integratedLoudness")) {
                            XML_VALUE
                            audioContent_Content.StringVectors[audioContent_loudnessMetadata_integratedLoudness].push_back(tfsxml_decode(b));
                        }
                XML_ELEM_END
            }
            ELEMENT(audioContent, dialogue)
        ELEMENT_END(audioContent)
        ELEMENT_START(audioObject)
            ATTRIB_ID(audioObject, audioObjectID)
            ATTRIBUTE(audioObject, audioObjectName)
            ATTRIBUTE(audioObject, duration)
            ATTRIBUTE(audioObject, start)
            ATTRIBUTE(audioObject, duration)
            ATTRIBUTE(audioObject, dialogue)
            ATTRIBUTE(audioObject, importance)
            ATTRIBUTE(audioObject, interact)
            ATTRIBUTE(audioObject, disableDucking)
        ELEMENT_MIDDLE(audioObject)
            ELEMENT(audioObject, audioPackFormatIDRef)
            ELEMENT(audioObject, audioObjectIDRef)
            ELEMENT(audioObject, audioObjectLabel)
            ELEMENT(audioObject, audioComplementaryObjectGroupLabel)
            ELEMENT(audioObject, audioComplementaryObjectIDRef)
            ELEMENT(audioObject, audioTrackUIDRef)
            ELEMENT(audioObject, audioObjectInteraction)
            ELEMENT(audioObject, gain)
            ELEMENT(audioObject, headLocked)
            ELEMENT(audioObject, positionOffset)
            ELEMENT(audioObject, mute)
            ELEMENT(audioObject, alternativeValueSet)
        ELEMENT_END(audioObject)
        ELEMENT_START(audioPackFormat)
            ATTRIB_ID(audioPackFormat, audioPackFormatID)
            ATTRIBUTE(audioPackFormat, audioPackFormatName)
            ATTRIBUTE(audioPackFormat, typeDefinition)
            ATTRIBUTE(audioPackFormat, typeLabel)
        ELEMENT_MIDDLE(audioPackFormat)
            ELEMENT(audioPackFormat, audioChannelFormatIDRef)
            ELEMENT(audioPackFormat, audioPackFormatIDRef)
            ELEMENT(audioPackFormat, absoluteDistance)
            ELEMENT(audioPackFormat, encodePackFormatIDRef)
            ELEMENT(audioPackFormat, decodePackFormatIDRef)
            ELEMENT(audioPackFormat, inputPackFormatIDRef)
            ELEMENT(audioPackFormat, outputPackFormatIDRef)
            ELEMENT(audioPackFormat, normalization)
            ELEMENT(audioPackFormat, nfcRefDist)
            ELEMENT(audioPackFormat, screenRef)
        ELEMENT_END(audioPackFormat)
        ELEMENT_START(audioChannelFormat)
            ATTRIB_ID(audioChannelFormat, audioChannelFormatID)
            ATTRIBUTE(audioChannelFormat, audioChannelFormatName)
            ATTRIBUTE(audioChannelFormat, typeDefinition)
            ATTRIBUTE(audioChannelFormat, typeLabel)
        ELEMENT_MIDDLE(audioChannelFormat)
            else if (!tfsxml_strcmp_charp(b, "audioBlockFormat")) {
                XML_ATTR_START
                    else if (!tfsxml_strcmp_charp(b, "audioBlockFormatID")) {
                        CheckErrors_Formating(this, tfsxml_decode(v), item_Info[item_audioBlockFormat], *this->Items[item_audioChannelFormat].Items.back().Errors, "audioBlockFormat:audioBlockFormatID");
                    }
                XML_ATTR_END
                XML_ELEM_START
                        if (!tfsxml_strcmp_charp(b, "speakerLabel")) {
                            XML_VALUE
                            string SpeakerLabel = tfsxml_decode(b);
                            if (SpeakerLabel.rfind("RC_", 0) == 0) {
                                SpeakerLabel.erase(0, 3);
                            }
                            map<string, string>::iterator Extra_SpeakerLabel = audioChannelFormat_Content.Extra.find("ChannelLayout");
                            map<string, string>::iterator Extra_Type = audioChannelFormat_Content.Extra.find("Type");
                            if (Extra_SpeakerLabel == audioChannelFormat_Content.Extra.end() || Extra_SpeakerLabel->second == SpeakerLabel) {
                                audioChannelFormat_Content.Extra["ChannelLayout"] = SpeakerLabel;
                                audioChannelFormat_Content.Extra["Type"] = "Static";
                            }
                            else if (Extra_Type != audioChannelFormat_Content.Extra.end()) {
                                audioChannelFormat_Content.Extra.clear();
                                audioChannelFormat_Content.Extra["Type"] = "Dynamic";
                            }
                    }
                XML_ELEM_END
            }
        ELEMENT_END(audioChannelFormat)
        ELEMENT_START(audioTrackUID)
            ATTRIBUTE(audioTrackUID, UID)
            ATTRIBUTE(audioTrackUID, sampleRate)
            ATTRIBUTE(audioTrackUID, bitDepth)
        ELEMENT_MIDDLE(audioTrackUID)
            ELEMENT(audioTrackUID, audioChannelFormatIDRef)
            ELEMENT(audioTrackUID, audioPackFormatIDRef)
            ELEMENT(audioTrackUID, audioTrackFormatIDRef)
        ELEMENT_END(audioTrackUID)
        ELEMENT_START(audioTrackFormat)
            ATTRIB_ID(audioTrackFormat, audioTrackFormatID)
            ATTRIBUTE(audioTrackFormat, audioTrackFormatName)
            ATTRIBUTE(audioTrackFormat, formatLabel)
            ATTRIBUTE(audioTrackFormat, formatDefinition)
        ELEMENT_MIDDLE(audioTrackFormat)
            ELEMENT(audioTrackFormat, audioStreamFormatIDRef)
        ELEMENT_END(audioTrackFormat)
        ELEMENT_START(audioStreamFormat)
            ATTRIB_ID(audioStreamFormat, audioStreamFormatID)
            ATTRIBUTE(audioStreamFormat, audioStreamFormatName)
            ATTRIBUTE(audioStreamFormat, formatLabel)
            ATTRIBUTE(audioStreamFormat, formatDefinition)
            ELEMENT_MIDDLE(audioStreamFormat)
            ELEMENT(audioStreamFormat, audioChannelFormatIDRef)
            ELEMENT(audioStreamFormat, audioPackFormatIDRef)
            ELEMENT(audioStreamFormat, audioTrackFormatIDRef)
        ELEMENT_END(audioStreamFormat)
    XML_ELEM_END
    XML_END
}

int file_adm_private::frameHeader()
{
    XML_BEGIN
    XML_ELEM_START
        if (!tfsxml_strcmp_charp(b, "frameFormat")) {
            XML_ATTR_START
                if (!tfsxml_strcmp_charp(b, "duration")) {
                    auto Duration = TimeCodeToFloat(tfsxml_decode(v));
                    if (FrameRate_Den < 5) // Handling of 1.001 frames rates
                    {
                        FrameRate_Sum += Duration;
                        FrameRate_Den++;
                    }
                    Duration = FrameRate_Sum / FrameRate_Den;
                    if (IsSub)
                        More["FrameRate"] = Ztring().From_Number(1 / Duration).To_UTF8();
                    else
                        More["Duration"] = Ztring().From_Number(Duration * 1000, 0).To_UTF8();
                }
                if (!tfsxml_strcmp_charp(b, "flowID")) {
                    More["FlowID"] = tfsxml_decode(v);
                }
                if (!tfsxml_strcmp_charp(b, "start") && More["TimeCode_FirstFrame"].empty()) {
                    More["TimeCode_FirstFrame"] = tfsxml_decode(v);
                }
                if (!tfsxml_strcmp_charp(b, "type")) {
                    More["Metadata_Format_Type"] = tfsxml_decode(v);
                }
            XML_ATTR_END
        }
        if (!tfsxml_strcmp_charp(b, "transportTrackFormat")) {
            XML_SUB(transportTrackFormat());
        }
    XML_ELEM_END
    XML_END
}

int file_adm_private::transportTrackFormat()
{
    if (IsInit()) {
        Items[item_audioTrack].Items.clear();
    }
    XML_BEGIN
    XML_ELEM_START
        ELEMENT_START(audioTrack)
            else if (!tfsxml_strcmp_charp(b, "trackID")) {
                SetSavedAttribute(0);
            }
        ELEMENT_MIDDLE(audioTrack)
            else if (!tfsxml_strcmp_charp(b, "audioTrackUIDRef")) {
                XML_VALUE
                auto trackIDP = GetSavedAttribute(0, 1);
                if (trackIDP != NoSavedAttribute(1)) {
                    auto trackID = Ztring().From_UTF8(trackIDP->second.c_str()).To_int32u();
                    chna_Add(trackID, tfsxml_decode(b));
                }
            }
        ELEMENT_END(audioTrack)
    XML_ELEM_END
    XML_END
}

//---------------------------------------------------------------------------
static void FillErrors(file_adm_private* File_Adm_Private, const item Item, size_t i, const char* Name, vector<string>* Errors_Field, vector<string>* Errors_Value, bool WarningError)
{
    for (size_t k = 0; k < error_Type_Max; k++) {
        if (!File_Adm_Private->Items[Item].Items[i].Errors[k].empty()) {
            for (size_t j = 0; j < File_Adm_Private->Items[Item].Items[i].Errors[k].size(); j++) {
                string Field = Name;
                string Value = File_Adm_Private->Items[Item].Items[i].Errors[k][j];
                if (!Value.empty() && Value[0] == '!') {
                    auto End = Value.find('!', 1);
                    if (End != string::npos)
                    {
                        Field.insert(0, Value.substr(1, End - 1) + ' ');
                        Value.erase(0, End + 1);
                    }
                }
                if (!Value.empty() && Value[0] == ':') {
                    Field.clear();
                    auto End = Value.rfind(':');
                    if (End)
                    {
                        if (!Field.empty())
                            Field += ' ';
                        Field += Value.substr(1, End - 1);
                        Value.erase(0, End + 1);
                        for (;;)
                        {
                            auto Next = Field.find(':');
                            if (Next == string::npos)
                                break;
                            Field[Next] = ' ';
                        }
                    }
                }
                Errors_Field[WarningError ? Error : k].push_back(Field);
                Errors_Value[WarningError ? Error : k].push_back(Value);
            }
        }
    }
}

//---------------------------------------------------------------------------
static void CheckErrors_Formating(file_adm_private* File_Adm_Private, const string& ID, const item_info& ID_Start, vector<string>& Errors, const char* Sub)
{
    auto BeginSize = strlen(ID_Start.ID_Begin);
    bitset<item_info::Flags_Max> Flags(ID_Start.ID_Flags);
    auto MiddleSize = Flags[item_info::Flags_ID_YX] ? 8 : (Flags[item_info::Flags_ID_W] ? 4 : 0);
    static const int end_size []  = {0, 2, 4, 8};
    auto EndSize = end_size[Flags[item_info::Flags_ID_Z1] + Flags[item_info::Flags_ID_Z2] * 2];
    if (!MiddleSize && !EndSize && ID.size() > BeginSize) {
        EndSize = ID.size() - BeginSize - 1;
    }
    auto TotalMiddleSize = MiddleSize ? (1 + MiddleSize) : 0;
    auto TotalEndSize = EndSize ? (1 + EndSize) : 0;
    if (ID.size() != BeginSize + TotalMiddleSize + TotalEndSize
        || ID.compare(0, BeginSize, ID_Start.ID_Begin, 0, BeginSize)
        || ID[BeginSize] != '_'
        || !IsHexaDigit(ID, BeginSize + 1, MiddleSize)
        || (EndSize && ID[BeginSize + TotalMiddleSize] != '_')
        || !IsHexaDigit(ID, BeginSize + TotalMiddleSize + 1, EndSize)
        ) {
        string Middle;
        if (Flags[item_info::Flags_ID_YX]) {
            Middle.append(4, 'y');
            Middle.append(4, 'x');
        }
        else if (Flags[item_info::Flags_ID_W]) {
            Middle.append(4, 'w');
        }
        string End;
        if (EndSize) {
            End.append(EndSize, 'z');
        }
        string Message = (Sub ? (':' + string(Sub) + ':') : string()) + '"' + ID + '"';
        Message += " is not a valid form (";
        if (BeginSize) {
            Message.append(ID_Start.ID_Begin, BeginSize);
        }
        Message += '_';
        if (!Middle.empty()) {
            Message += Middle;
        }
        if (!Middle.empty() && !End.empty()) {
            Message += '_';
        }
        if (!End.empty()) {
            Message += End;
        }
        Message += " form, ";
        if (!Middle.empty()) {
            Message += Middle;
        }
        if (!Middle.empty() && !End.empty()) {
            Message += " and ";
        }
        if (!End.empty()) {
            Message += End;
        }
        Message += " being hexadecimal digits)";
        Errors.push_back(Message);
    }
}

//---------------------------------------------------------------------------
static void CheckErrors_Formating(file_adm_private* File_Adm_Private, Items_Struct& MainItem, const item_info& ID_Start, const char* Sub)
{
    if (!ID_Start.ID_Begin)
        return;
    for (size_t i = 0; i < MainItem.Items.size(); i++) {
        CheckErrors_Formating(File_Adm_Private, MainItem.Items[i].Strings[audioProgramme_audioProgrammeID], ID_Start, *MainItem.Items[i].Errors, Sub);
    }
}

static void CheckErrors(file_adm_private* File_Adm_Private)
{
    for (size_t t=0; t<item_Max; t++) {
        //CheckErrors_Formating(File_Adm_Private, File_Adm_Private->Items[t], item_Info[t]);
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Adm::File_Adm()
:File__Analyze()
{
    //Configuration
    Buffer_MaximumSize = 256 * 1024 * 1024;

    File_Adm_Private = new file_adm_private();
}

//---------------------------------------------------------------------------
File_Adm::~File_Adm()
{
    delete File_Adm_Private;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Adm::Streams_Fill()
{
    // Clean up
    if (!File_Adm_Private->Items[item_audioTrack].Items.empty())
    {
        if (File_Adm_Private->Items[item_audioTrack].Items[0].StringVectors[audioTrack_audioTrackUIDRef].empty())
            File_Adm_Private->Items[item_audioTrack].Items.erase(File_Adm_Private->Items[item_audioTrack].Items.begin());
    }

    #define FILL_COUNT(NAME) \
        if (!File_Adm_Private->Items[item_##NAME].Items.empty()) \
            Fill(Stream_Audio, 0, ("NumberOf" + string(item_Info[item_##NAME].Name) + 's').c_str(), File_Adm_Private->Items[item_##NAME].Items.size());

    #define FILL_START(NAME,ATTRIBUTE) \
        for (size_t i = 0; i < File_Adm_Private->Items[item_##NAME].Items.size(); i++) { \
            Ztring Summary = Ztring().From_UTF8(File_Adm_Private->Items[item_##NAME].Items[i].Strings[NAME##_##ATTRIBUTE]); \
            string P = Apply_Init(*this, item_Info[item_##NAME].Name, i, File_Adm_Private->Items[item_##NAME], Summary); \

    #define FILL_START_SUB(NAME,ATTRIBUTE,FIELD) \
        for (size_t i = 0; i < File_Adm_Private->Items[item_##NAME].Items.size(); i++) { \
            Ztring Summary = Ztring().From_UTF8(File_Adm_Private->Items[item_##NAME].Items[i].Strings[NAME##_##ATTRIBUTE]); \
            string P = Apply_Init(*this, FIELD, i, File_Adm_Private->Items[item_##NAME], Summary); \

#define FILL_A(NAME,ATTRIBUTE,FIELD) \
        Fill(Stream_Audio, StreamPos_Last, (P + ' ' + FIELD).c_str(), File_Adm_Private->Items[item_##NAME].Items[i].Strings[NAME##_##ATTRIBUTE].c_str(), Unlimited, true, true); \

    #define FILL_E(NAME,ATTRIBUTE,FIELD) \
        { \
        ZtringList List; \
        List.Separator_Set(0, " + "); \
        for (size_t j = 0; j < File_Adm_Private->Items[item_##NAME].Items[i].StringVectors[NAME##_##ATTRIBUTE].size(); j++) { \
            List.push_back(Ztring().From_UTF8(File_Adm_Private->Items[item_##NAME].Items[i].StringVectors[NAME##_##ATTRIBUTE][j].c_str())); \
        } \
        string FieldName = P; \
        if (FIELD[0]) \
        { \
            FieldName += ' '; \
            FieldName += FIELD; \
        } \
        Fill(Stream_Audio, StreamPos_Last, FieldName.c_str(), List.Read(), true); \
        } \

    #define LINK(NAME,FIELD,VECTOR,TARGET) \
        Apply_SubStreams(*this, P + " LinkedTo_" FIELD "_Pos", File_Adm_Private->Items[item_##NAME].Items[i], NAME##_##VECTOR, File_Adm_Private->Items[item_##TARGET]); \

    //Filling
    Stream_Prepare(Stream_Audio);
    if (!IsSub)
        Fill(Stream_Audio, StreamPos_Last, Audio_Format, "ADM");

    if (File_Adm_Private->Version_S >= 0)
        Fill(Stream_Audio, StreamPos_Last, "Metadata_Format", "S-ADM, Version " + Ztring::ToZtring(File_Adm_Private->Version_S).To_UTF8());
    Fill(Stream_Audio, StreamPos_Last, "Metadata_Format", "ADM, Version " + Ztring::ToZtring(File_Adm_Private->Version).To_UTF8() + File_Adm_Private->More["Metadata_Format"]);
    if (!MuxingMode.empty())
        Fill(Stream_Audio, StreamPos_Last, "Metadata_MuxingMode", MuxingMode);
    for (map<string, string>::iterator It = File_Adm_Private->More.begin(); It != File_Adm_Private->More.end(); ++It)
        //if (It->first != "Metadata_Format")
            Fill(Stream_Audio, StreamPos_Last, It->first.c_str(), It->second);
    if (File_Adm_Private->Items[item_audioProgramme].Items.size() == 1 && File_Adm_Private->Items[item_audioProgramme].Items[0].Strings[audioProgramme_audioProgrammeName] == "Atmos_Master") {
        if (!File_Adm_Private->DolbyProfileCanNotBeVersion1 && File_Adm_Private->Version>1)
            File_Adm_Private->DolbyProfileCanNotBeVersion1=true;
        Fill(Stream_Audio, 0, "AdmProfile", (!File_Adm_Private->DolbyProfileCanNotBeVersion1)?"Dolby Atmos Master, Version 1":"Dolby Atmos Master");
        Fill(Stream_Audio, 0, "AdmProfile_Format", "Dolby Atmos Master");
        Fill_SetOptions(Stream_Audio, 0, "AdmProfile_Format", "N NTY");
        if (!File_Adm_Private->DolbyProfileCanNotBeVersion1)
        {
            Fill(Stream_Audio, 0, "AdmProfile_Version", "1");
            Fill_SetOptions(Stream_Audio, 0, "AdmProfile_Version", "N NTY");
        }
    }
    vector<profile_info>& profileInfos = File_Adm_Private->profileInfos;
    if (!profileInfos.empty())
    {
        // Find what is in common
        int PosCommon = profile_names_size;
        for (int i = 0; i < PosCommon; i++)
            for (size_t j = 1; j < profileInfos.size(); j++)
                if (profileInfos[j].Strings[i] != profileInfos[0].Strings[i])
                    PosCommon = i;

        Fill(Stream_Audio, 0, "AdmProfile", PosCommon ? profileInfos[0].profile_info_build(PosCommon) : string("Multiple"));
        if (profileInfos.size() > 1)
        {
            for (size_t i = 0; i < profileInfos.size(); i++)
            {
                Fill(Stream_Audio, 0, ("AdmProfile AdmProfile" + Ztring::ToZtring(i).To_UTF8()).c_str(), profileInfos[i].profile_info_build());
                for (size_t j = 0; j < profile_names_size; j++)
                {
                    Fill(Stream_Audio, 0, ("AdmProfile AdmProfile" + Ztring::ToZtring(i).To_UTF8() + ' ' + profile_names_InternalID[j]).c_str(), profileInfos[i].Strings[j]);
                    Fill_SetOptions(Stream_Audio, 0, ("AdmProfile AdmProfile" + Ztring::ToZtring(i).To_UTF8() + ' ' + profile_names_InternalID[j]).c_str(), "N NTY");
                }
            }
        }
        for (size_t j = 0; j < (PosCommon == 0 ? 1 : PosCommon); j++)
        {
            Fill(Stream_Audio, 0, (string("AdmProfile_") + profile_names_InternalID[j]).c_str(), j < PosCommon ? profileInfos[0].Strings[j] : "Multiple");
            Fill_SetOptions(Stream_Audio, 0, (string("AdmProfile_") + profile_names_InternalID[j]).c_str(), "N NTY");
        }
    }
    size_t TotalCount = 0;
    for (size_t i = 0; i < item_Max; i++)
        TotalCount += File_Adm_Private->Items[i].Items.size();
    bool Full = TotalCount < 0x1000 ? true : false;
    FILL_COUNT(audioProgramme);
    FILL_COUNT(audioContent);
    FILL_COUNT(audioObject);
    FILL_COUNT(audioPackFormat);
    FILL_COUNT(audioChannelFormat);
    if (Full)
    {
        FILL_COUNT(audioTrackUID);
        FILL_COUNT(audioTrackFormat);
        FILL_COUNT(audioStreamFormat);
    }
    #if MEDIAINFO_CONFORMANCE
    vector<string> Errors_Field[error_Type_Max];
    vector<string> Errors_Value[error_Type_Max];
    MediaInfo_Config::adm_profile Config_AdmProfile=MediaInfoLib::Config.AdmProfile();
    bool WarningError=MediaInfoLib::Config.WarningError();
    #endif

    // Common definitions
    for (size_t i = 0; i < File_Adm_Private->Items[item_audioPackFormat].Items.size(); i++) {
        const Item_Struct& Source = File_Adm_Private->Items[item_audioPackFormat].Items[i];
        for (size_t j = 0; j < Source.StringVectors[audioPackFormat_audioChannelFormatIDRef].size(); j++) {

        }
    }

    FILL_START(audioProgramme, audioProgrammeName)
        if (Full)
            FILL_A(audioProgramme, audioProgrammeID, "ID");
        FILL_A(audioProgramme, audioProgrammeName, "Title");
        FILL_E(audioProgramme, audioProgrammeLabel, "Label");
        FILL_A(audioProgramme, audioProgrammeLanguage, "Language");
        FILL_A(audioProgramme, start, "Start");
        FILL_A(audioProgramme, start, "Start/String");
        FILL_A(audioProgramme, start, "Start/TimeCode");
        FILL_A(audioProgramme, start, "Start/TimeCodeSubFrames");
        FILL_A(audioProgramme, start, "Start/TimeCodeSamples");
        FILL_A(audioProgramme, end, "End");
        FILL_A(audioProgramme, end, "End/String");
        FILL_A(audioProgramme, end, "End/TimeCode");
        FILL_A(audioProgramme, end, "End/TimeCodeSubFrames");
        FILL_A(audioProgramme, end, "End/TimeCodeSamples");
        FILL_E(audioProgramme, loudnessMetadata_integratedLoudness, "IntegratedLoudness");
        LINK(audioProgramme, "Content", audioContentIDRef, audioContent);
        LINK(audioProgramme, "PackFormat", authoringInformation_referenceLayout_audioPackFormatIDRef, audioPackFormat);
        const Ztring& Label = Retrieve_Const(StreamKind_Last, StreamPos_Last, (P + " Label").c_str());
        if (!Label.empty()) {
            Summary += __T(' ');
            Summary += __T('(');
            Summary += Label;
            Summary += __T(')');
            Fill(StreamKind_Last, StreamPos_Last, P.c_str(), Summary, true);
        }

        #if MEDIAINFO_CONFORMANCE
        //TODO: expand this proof of concept
        if (Config_AdmProfile.Ebu3392==1)
        {
            const string& audioProgrammeName = File_Adm_Private->Items[item_audioProgramme].Items[i].Strings[audioProgramme_audioProgrammeName];
            if (Ztring().From_UTF8(audioProgrammeName).size() > 32)
            {
                Errors_Field[Error].push_back("audioProgramme");
                Errors_Value[Error].push_back("audioProgrammeName length is more than EBU Tech 3392 constraint");
            }
        }
        #endif // MEDIAINFO_CONFORMANCE
    }

    FILL_START(audioContent, audioContentName)
        if (Full)
            FILL_A(audioContent, audioContentID, "ID");
        FILL_A(audioContent, audioContentName, "Title");
        FILL_E(audioContent, audioContentLabel, "Label");
        FILL_A(audioContent, audioContentLanguage, "Language");
        FILL_E(audioContent, dialogue, "Mode");
        FILL_E(audioContent, loudnessMetadata_integratedLoudness, "IntegratedLoudness");
        LINK(audioContent, "Object", audioObjectIDRef, audioObject);
        const Ztring& Label = Retrieve_Const(StreamKind_Last, StreamPos_Last, (P + " Label").c_str());
        if (!Label.empty()) {
            Summary += __T(' ');
            Summary += __T('(');
            Summary += Label;
            Summary += __T(')');
            Fill(StreamKind_Last, StreamPos_Last, P.c_str(), Summary, true);
        }
    }

    FILL_START(audioObject, audioObjectName)
        if (Full)
            FILL_A(audioObject, audioObjectID, "ID");
        FILL_A(audioObject, audioObjectName, "Title");
        FILL_A(audioObject, start, "Start");
        FILL_A(audioObject, duration, "Duration");
        LINK(audioObject, "PackFormat", audioPackFormatIDRef, audioPackFormat);
        LINK(audioObject, "Object", audioObjectIDRef, audioObject);
        LINK(audioObject, "ComplementaryObject", audioComplementaryObjectIDRef, audioObject);
        if (Full)
            LINK(audioObject, "TrackUID", audioTrackUIDRef, audioTrackUID);
    }

    FILL_START(audioPackFormat, audioPackFormatName)
        if (Full)
            FILL_A(audioPackFormat, audioPackFormatID, "ID");
        FILL_A(audioPackFormat, audioPackFormatName, "Title");
        FILL_A(audioPackFormat, typeDefinition, "TypeDefinition");
        const Item_Struct& Source = File_Adm_Private->Items[item_audioPackFormat].Items[i];
        const Items_Struct& Dest = File_Adm_Private->Items[item_audioChannelFormat];
        string SpeakerLabel;
        for (size_t j = 0; j < Source.StringVectors[audioPackFormat_audioChannelFormatIDRef].size(); j++) {
            const string& ID = Source.StringVectors[audioPackFormat_audioChannelFormatIDRef][j];
            for (size_t k = 0; k < Dest.Items.size(); k++) {
                if (Dest.Items[k].Strings[audioChannelFormat_audioChannelFormatID] != ID) {
                    continue;
                }
                for (map<string, string>::const_iterator Extra = Dest.Items[k].Extra.begin(); Extra != Dest.Items[k].Extra.end(); ++Extra) {
                    if (Extra->first == "ChannelLayout") {
                        if (!SpeakerLabel.empty()) {
                            SpeakerLabel += ' ';
                        }
                        SpeakerLabel += Extra->second;
                    }
                }
            }
        }
        if (!SpeakerLabel.empty()) {
            Fill(StreamKind_Last, StreamPos_Last, (P + " ChannelLayout").c_str(), SpeakerLabel, true, true);
        }

        LINK(audioPackFormat, "ChannelFormat", audioChannelFormatIDRef, audioChannelFormat);
    }

    FILL_START(audioChannelFormat, audioChannelFormatName)
        if (Full)
            FILL_A(audioChannelFormat, audioChannelFormatID, "ID");
        FILL_A(audioChannelFormat, audioChannelFormatName, "Title");
        FILL_A(audioChannelFormat, typeDefinition, "TypeDefinition");
        for (map<string, string>::iterator Extra = File_Adm_Private->Items[item_audioChannelFormat].Items[i].Extra.begin(); Extra != File_Adm_Private->Items[item_audioChannelFormat].Items[i].Extra.end(); ++Extra) {
            Fill(Stream_Audio, StreamPos_Last, (P + ' ' + Extra->first).c_str(), Extra->second);
        }
    }

    if (Full) {
        FILL_START(audioTrackUID, UID)
            FILL_A(audioTrackUID, UID, "ID");
            FILL_A(audioTrackUID, bitDepth, "BitDepth");
            FILL_A(audioTrackUID, sampleRate, "SamplingRate");
            string& ID = File_Adm_Private->Items[item_audioTrackUID].Items[i].Strings[audioTrackUID_UID];
            for (size_t j = 0; j < File_Adm_Private->Items[item_audioTrack].Items.size(); j++)
            {
                Item_Struct& Item = File_Adm_Private->Items[item_audioTrack].Items[j];
                vector<string>& Ref = Item.StringVectors[audioTrack_audioTrackUIDRef];
                for (size_t k = 0; k < Ref.size(); k++)
                {
                    if (Ref[k] == ID)
                    {
                        Fill(Stream_Audio, 0, (P + " TrackIndex/String").c_str(), j + 1);
                        Fill_SetOptions(Stream_Audio, 0, (P + " TrackIndex/String").c_str(), "Y NIN");
                    }
                }
            }
            LINK(audioTrackUID, "ChannelFormat", audioChannelFormatIDRef, audioChannelFormat);
            LINK(audioTrackUID, "PackFormat", audioPackFormatIDRef, audioPackFormat);
            LINK(audioTrackUID, "TrackFormat", audioTrackFormatIDRef, audioTrackFormat);
        }

        FILL_START(audioTrackFormat, audioTrackFormatName)
            FILL_A(audioTrackFormat, audioTrackFormatID, "ID");
            FILL_A(audioTrackFormat, audioTrackFormatName, "Title");
            FILL_A(audioTrackFormat, formatLabel, "formatLabel");
            FILL_A(audioTrackFormat, formatDefinition, "FormatDefinition");
            LINK(audioTrackFormat, "StreamFormat", audioStreamFormatIDRef, audioStreamFormat);
        }

        FILL_START(audioStreamFormat, audioStreamFormatName)
            FILL_A(audioStreamFormat, audioStreamFormatID, "ID");
            FILL_A(audioStreamFormat, audioStreamFormatName, "Title");
            FILL_A(audioStreamFormat, formatDefinition, "Format");
            LINK(audioStreamFormat, "ChannelFormat", audioChannelFormatIDRef, audioChannelFormat);
            LINK(audioStreamFormat, "PackFormat", audioPackFormatIDRef, audioPackFormat);
            LINK(audioStreamFormat, "TrackFormat", audioTrackFormatIDRef, audioTrackFormat);
        }
    }

    if (!File_Adm_Private->Items[item_audioTrack].Items.empty())
    {
        Fill(Stream_Audio, 0, "Transport0", "Yes");
        FILL_START_SUB(audioTrack, Summary, "Transport0 TrackIndex")
            FILL_E(audioTrack, audioTrackUIDRef, "");
            if (Full)
                LINK(audioTrack, "TrackUID", audioTrackUIDRef, audioTrackUID);
        }
    }

    if (!Full)
        Fill(Stream_Audio, 0, "PartialDisplay", "Yes");

    // Errors
    #if MEDIAINFO_CONFORMANCE
    CheckErrors(File_Adm_Private);
    if (!File_Adm_Private->Items[item_audioProgramme].Items.empty() && File_Adm_Private->Items[item_audioProgramme].Items[0].Strings[audioProgramme_audioProgrammeName]=="Atmos_Master") {
        #define CheckMaxCount(_NAME, _MAX) \
            auto& _NAME##s = File_Adm_Private->Items[item_audio##_NAME].Items; \
            if (_NAME##s.size() > _MAX) { \
                _NAME##s[0].AddError(Error, ":audio"#_NAME":Count:Dolby Atmos Master ADM Profile v1.0 does not permit audio"#_NAME" element count " + to_string(_NAME##s.size()) + ", max is "#_MAX); \
            }
        CheckMaxCount(Programme, 1);
        CheckMaxCount(Content, 123);
        CheckMaxCount(Object, 123);
        CheckMaxCount(PackFormat, 123);
        CheckMaxCount(ChannelFormat, 128);
        CheckMaxCount(StreamFormat, 128);
        CheckMaxCount(TrackFormat, 128);
        CheckMaxCount(TrackUID, 128);
        size_t PackFormat_Objects_Count = 0;
        for (const auto& PackFormat : PackFormats) {
            if (PackFormat.Strings[audioPackFormat_typeDefinition] == "Objects") {
                PackFormat_Objects_Count++;
            }
            else if (PackFormat.Strings[audioPackFormat_typeDefinition].empty()) {
                PackFormats[0].AddError(Error, ":audioPackFormat:XmlAttributes:\"typeDefinition\" attribute shall be present (additional presence constraint from Dolby Atmos Master ADM Profile v1.0)");
            }
            else if (PackFormat.Strings[audioPackFormat_typeDefinition] != "DirectSpeakers") {
                PackFormats[0].AddError(Error, ":audioPackFormat:typeDefinition:Dolby Atmos Master ADM Profile v1.0 does not permit audioPackFormat@typeDefinition \"" + string(PackFormat.Strings[audioPackFormat_typeDefinition]) + "\", permitted values are \"DirectSpeakers\" or \"Objects\"");
            }
            if (PackFormat.Strings[audioPackFormat_typeLabel] == "0003") {
                if (PackFormat.Strings[audioPackFormat_typeDefinition] != "Objects") {
                    PackFormat_Objects_Count++;
                }
            }
            else if (PackFormat.Strings[audioPackFormat_typeLabel].empty()) {
                PackFormats[0].AddError(Error, ":audioPackFormat:XmlAttributes:\"typeLabel\" attribute shall be present (additional presence constraint from Dolby Atmos Master ADM Profile v1.0)");
            }
            else if (PackFormat.Strings[audioPackFormat_typeLabel] != "0001") {
                PackFormats[0].AddError(Error, ":audioPackFormat:typeLabel:Dolby Atmos Master ADM Profile v1.0 does not permit audioPackFormat@typeLabel \"" + string(PackFormat.Strings[audioPackFormat_typeLabel]) + "\", permitted values are \"0001\" or \"0003\"");
            }
        }
        if (PackFormat_Objects_Count > 18) {
            PackFormats[0].AddError(Error, ":audioPackFormat:Count::Dolby Atmos Master ADM Profile v1.0 does not permit audioPackFormat@typeDefinition==\"Objects\" element count " + to_string(PackFormat_Objects_Count) + ", max is 118");
        }
    }
    for (size_t t = 0; t < item_Max; t++) {
        for (size_t i = 0; i < File_Adm_Private->Items[t].Items.size(); i++) {
            FillErrors(File_Adm_Private, (item)t, i, item_Info[t].Name, &Errors_Field[0], &Errors_Value[0], WarningError); \
        }
    }

    //Conformance
    for (size_t k = 0; k < error_Type_Max; k++) {
        if (!Errors_Field[k].empty()) {
            auto FieldPrefix = "Conformance" + string(error_Type_String[k]);
            Fill(StreamKind_Last, StreamPos_Last, FieldPrefix.c_str(), Errors_Field[k].size());
            FieldPrefix += ' ';
            for (size_t i = 0; i < Errors_Field[k].size(); i++) {
                size_t Space = 0;
                for (;;)
                {
                    Space = Errors_Field[k][i].find(' ', Space + 1);
                    if (Space == string::npos) {
                        break;
                    }
                    const auto Field = FieldPrefix + Errors_Field[k][i].substr(0, Space);
                    const auto& Value = Retrieve_Const(StreamKind_Last, StreamPos_Last, Field.c_str());
                    if (Value.empty()) {
                        Fill(StreamKind_Last, StreamPos_Last, Field.c_str(), "Yes");
                    }
                }
                const auto Field = FieldPrefix + Errors_Field[k][i];
                const auto& Value = Retrieve_Const(StreamKind_Last, StreamPos_Last, Field.c_str());
                if (Value == __T("Yes")) {
                    Clear(StreamKind_Last, StreamPos_Last, Field.c_str());
                }
                Fill(StreamKind_Last, StreamPos_Last, Field.c_str(), Errors_Value[k][i]);
            }
        }
    }
   #endif // MEDIAINFO_CONFORMANCE

    Element_Offset=File_Size;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Adm::FileHeader_Begin()
{
    // File must be fully loaded
    if (!IsSub && Buffer_Size < File_Size)
    {
        if (Buffer_Size && Buffer[0] != '<')
        {
            Reject();
            return false;
        }
    }

    return true;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Adm::Read_Buffer_Init()
{
    File_Adm_Private->IsSub = IsSub;
}

//---------------------------------------------------------------------------
void File_Adm::Read_Buffer_Continue()
{
    Accept("ADM");
    auto Result = File_Adm_Private->parse((void*)Buffer, Buffer_Size);
    if (!Status[IsAccepted] && !File_Adm_Private->Items[item_audioContent].Items.empty())
    {
        Accept("ADM");
    }
    if (Result > 0 && File_Offset + Buffer_Size < File_Size)
    {
        Buffer_Offset = Buffer_Size - File_Adm_Private->Remain();
        Element_WaitForMoreData();
        return;
    }
}

//***************************************************************************
// In
//***************************************************************************

void File_Adm::chna_Add(int32u Index, const string& TrackUID)
{
    File_Adm_Private->chna_Add(Index, TrackUID);
}

void* File_Adm::chna_Move()
{
    if (!File_Adm_Private)
        return NULL;
    return &File_Adm_Private->Items[item_audioTrack];
}

void File_Adm::chna_Move(File_Adm* Adm)
{
    if (!Adm)
        return;
    if (!File_Adm_Private)
        File_Adm_Private = new file_adm_private();
    File_Adm_Private->Items[item_audioTrack] = Adm->File_Adm_Private->Items[item_audioTrack];
}

} //NameSpace

#endif //MEDIAINFO_ADM_YES
