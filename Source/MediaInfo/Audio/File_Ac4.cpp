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
#include "cfloat"

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

typedef const float sized_array_float[];
string Value(const sized_array_float Array, size_t Pos, size_t AfterComma=3)
{
    if (Pos++>=(size_t)Array[0] || !Array[Pos])
        return "Index "+Ztring::ToZtring(Pos).To_UTF8();
    if (Array[Pos]==-FLT_MAX)
        return "-inf";
    if (Array[Pos]==FLT_MAX)
        return "inf";
    return Ztring(Ztring::ToZtring(Array[Pos], AfterComma)).To_UTF8();
}
typedef const char* sized_array_string[];
string Value(const sized_array_string Array, size_t Pos)
{
    if (Pos++>=(size_t)Array[0] || !Array[Pos])
        return Ztring::ToZtring(Pos).To_UTF8();
    return Array[Pos];
}

static const int16u Ac4_fs_index[2]=
{
    44100,
    48000,
};

static const variable_size Ac4_channel_mode[]=
{
    {13, 0}, // Total size
    {1, 0b0},
    {1, 0b10},
    {2, 0b1100},
    {0, 0b1101},
    {0, 0b1110},
    {3, 0b1111000},
    {0, 0b1111001},
    {0, 0b1111010},
    {0, 0b1111011},
    {0, 0b1111100},
    {0, 0b1111101},
    {0, 0b1111110},
    {0, 0b1111111},
};

static const variable_size Ac4_channel_mode2[]=
{
    {17, 0}, // Total size
    {1, 0b0},
    {1, 0b10},
    {2, 0b1100},
    {0, 0b1101},
    {0, 0b1110},
    {3, 0b1111000},
    {0, 0b1111001},
    {0, 0b1111010},
    {0, 0b1111011},
    {0, 0b1111100},
    {0, 0b1111101},
    {1, 0b11111100},
    {0, 0b11111101},
    {1, 0b111111100},
    {0, 0b111111101},
    {0, 0b111111110},
    {0, 0b111111111},
};

static const sized_array_string Ac4_content_classifier=
{
(const char*)8,
"CM",
"ME",
"VI",
"HI",
"D",
"C",
"E",
"VO",
};

static const sized_array_string Ac4_ch_mode=
{
(const char*)16,
"Mono",
"Stereo",
"3.0",
"5.0",
"5.1",
"7.0 3/4/0",
"7.1 3/4/0",
"7.0 5/2/0",
"7.1 5/2/0",
"7.0 3/2/2",
"7.1 3/2/2",
"7.0.4",
"7.1.4",
"9.0.4",
"9.1.4",
"22.2",
};
/*
static const sized_array Ac4_ch_mode=
{
16,
{
"M",
"L, R",
"3.0",
"5.0",
"5.1",
"L,C,R,Ls,Rs,Lrs,Rrs",
"L,C,R,Ls,Rs,Lrs,Rrs,LFE",
"L,C,R,Lw,Rw,Ls,Rs",
"L,C,R,Lw,Rw,Ls,Rs,LFE",
"L,C,R,Ls,Rs,Vhl,Vhr",
"L,C,R,Ls,Rs,Vhl,Vhr,LFE",
"7.0.4",
"7.1.4",
"9.0.4",
"9.1.4",
"22.2",
};
*/

static const sized_array_string Ac4_presentation_config=
{
(const char*)7,
"Music and Effects + Dialogue",
"Main + Dialogue Enhancement",
"Main + Associate",
"Music and Effects + Dialogue + Associate",
"Main + Dialogue Enhancement + Associate",
"Arbitrary Substream Groups",
"EMDF Only",
};

static const sized_array_string Ac4_drc_eac3_profile=
{
(const char*)6,
"None",
"Film standard",
"Film light",
"Music standard",
"Music light",
"Speech",
};

static const sized_array_string Ac4_loud_prac_type=
{
(const char*)16,
NULL,
"ATSC A/85",
"EBU R128",
"ARIB TR-B32",
"FreeTV OP-59",
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
"Manual",
"Consumer leveller",
};

static const sized_array_string Ac4_loud_dialgate_prac_type=
{
(const char*)4,
NULL,
"Automated C or L+R",
"Automated individual front channels",
"Manual",
};

static const sized_array_string Ac4_lra_prac_type=
{
(const char*)2,
"EBU Tech 3342 v1",
"EBU Tech 3342 v2",
};

static const sized_array_string Ac4_drc_decoder_mode_id=
{
(const char*)4,
"HomeTheaterAvr",
"FlatPanelTv",
"PortableSpeakers",
"PortableHeadphones",
};

static const sized_array_float Ac4_loro_centre_mixgain=
{
8,
3.0,
1.5,
0.0,
-1.5,
-3.0,
-4.5,
-6.0,
-FLT_MAX,
};

static const sized_array_string Ac4_preferred_dmx_method=
{
(const char*)4,
"",
"LoRo",
"LtRt",
"Pro Logic II",
};

static const sized_array_string Ac4_pre_dmixtyp_2ch=
{
(const char*)4,
"",
"LoRo",
"Pro Logic",
"Pro Logic II",
};

static const sized_array_string Ac4_phase90_info_2ch=
{
(const char*)3,
"",
"Applied",
"Not applied",
};

static const sized_array_string Ac4_pre_dmixtyp_5ch=
{
(const char*)4,
"Pro Logic IIx",
"Pro Logic IIx Movie",
"Pro Logic IIx Music",
"Pro Logic IIz",
};

static const sized_array_string Ac4_phase90_info_mc=
{
(const char*)3,
"",
"Applied",
"Not applied",
};

static const sized_array_string Ac4_de_channel_config=
{
(const char*)8,
"",
"M",
"R",
"R C",
"L",
"L C",
"L R",
"L R C",
};

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_Lfe(int8u ch_mode)
{
    return (ch_mode==4 || ch_mode==6 || ch_mode==8 || ch_mode==10 || ch_mode==12 || ch_mode==14 || ch_mode==15)?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_C(int8u ch_mode)
{
    return (ch_mode==0 || (ch_mode>=2 && ch_mode<=15))?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_Lr(int8u ch_mode)
{
    return (ch_mode>=1 && ch_mode<=15)?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_LsRs(int8u ch_mode)
{
    return (ch_mode>=3 && ch_mode<=15)?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_LrsRrs(int8u ch_mode)
{
    return (ch_mode==5 || ch_mode==6 || (ch_mode>=11 && ch_mode<=15))?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_LwRw(int8u ch_mode)
{
    return (ch_mode==7 || ch_mode==8 || ch_mode==15)?true:false;
}

//---------------------------------------------------------------------------
static inline bool Channel_Mode_Contains_VhlVhr(int8u ch_mode)
{
    return (ch_mode==9 || ch_mode==10)?true:false;
}

//---------------------------------------------------------------------------
void File_Ac4::Streams_Fill()
{
    //
    bool IFrames_IsVariable=false;
    size_t IFrames_Value=0;
    if (IFrames.size()>1)
    {
        IFrames_Value=IFrames[1]-IFrames[0];
        for (size_t i=2; i<IFrames.size(); i++)
            if (IFrames[i]-IFrames[i-1]!=IFrames_Value)
            {
                IFrames_IsVariable=true;
                break;
            }
    }

    Fill(Stream_General, 0, General_Format, "AC-4");

    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "AC-4");
    Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, "Dolby AC-4");
    Fill(Stream_Audio, 0, Audio_Format_Version, __T("Version ")+Ztring::ToZtring(bitstream_version));
    Fill(Stream_Audio, 0, Audio_SamplingRate, fs_index?48000:44100);
    Fill(Stream_Audio, 0, Audio_FrameRate, Ac4_frame_rate[fs_index][frame_rate_index]);
    if (IFrames_Value)
        Fill(Stream_Audio, 0, "IFrameInterval", IFrames_IsVariable?Ztring(__T("Variable")):(Ztring::ToZtring(IFrames_Value)+__T(" frames")));
    Fill(Stream_Audio, 0, "NumberOfPresentations", Presentations.size());
    Fill(Stream_Audio, 0, "NumberOfSubstreams", AudioSubstreams.size());

    for (size_t p=0; p<Presentations.size(); p++)
    {
        const presentation& Presentation_Current=Presentations[p];
        string Language;
        string Text;
        int8u ch_mode=0;//TODO pres_ch_mode
        for (size_t g=0; g<Presentation_Current.substream_group_info_specifiers.size(); g++)
        {
            const group& Group=Groups[Presentation_Current.substream_group_info_specifiers[g]];
            if (!Group.ContentInfo.language_tag_bytes.empty() && (Group.ContentInfo.content_classifier==0 || Group.ContentInfo.content_classifier==1 || Group.ContentInfo.content_classifier==4))
            {
                if (!Language.empty())
                    Language+=" / ";
                Language+=Group.ContentInfo.language_tag_bytes;
            }
            for (size_t s=0; s<Group.Substreams.size(); s++)
            {
                const group_substream& Substream=Group.Substreams[s];
                if (Substream.ch_mode!=(int8u)-1)
                {
                    //if (!Text.empty())
                    //    Text += " / "; //TODO pres_ch_mode
                    //Text+=Value(Ac4_ch_mode, Substream.ch_mode);
                    if (ch_mode<Substream.ch_mode)
                        ch_mode=Substream.ch_mode; //TODO pres_ch_mode
                }
            }
        }
        Text+=Value(Ac4_ch_mode, ch_mode);

        string Summary=Text;
        if (!Summary.empty())
            Summary+=' ';
        Summary+=Presentation_Current.presentation_config==(int8u)-1?"Main":Value(Ac4_presentation_config, Presentation_Current.presentation_config);
        if (!Language.empty())
        {
            Summary+=" (";
            Summary+=MediaInfoLib::Config.Language_Get(Ztring().From_UTF8("Language_"+Language)).To_UTF8();;
            Summary+=')';
        }

        string P=Ztring(__T("Presentation")+Ztring::ToZtring(p)).To_UTF8();
        Fill(Stream_Audio, 0, P.c_str(), Summary);
        if (Presentation_Current.LoudnessInfo.dialnorm_bits!=(int8u)-1) //TODO: not LoudnessInfo
            Fill(Stream_Audio, 0, (P+" DialogueNormalization").c_str(), -0.25*Presentation_Current.LoudnessInfo.dialnorm_bits, 2);
        if (!Language.empty())
        {
            Fill(Stream_Audio, 0, (P+" Language").c_str(), Language);
            Fill(Stream_Audio, 0, (P+" Language/String").c_str(), MediaInfoLib::Config.Language_Get(Ztring().From_UTF8("Language_"+Language)));
            Fill_SetOptions(Stream_Audio, 0, (P+" Language").c_str(), "N NTY");
            Fill_SetOptions(Stream_Audio, 0, (P+" Language/String").c_str(), "Y NTN");
        }
        if (Presentation_Current.b_multi_pid_PresentAndValue!=(int8u)-1)
            Fill(Stream_Audio, 0, (P+" MultipleStream").c_str(), Presentation_Current.b_multi_pid_PresentAndValue?"Yes":"No");
        {
            const loudness_info& L=Presentation_Current.LoudnessInfo;
            if (L.loudspchgat!=(int16u)-1 || L.loudrelgat!=(int16u)-1 || (L.loud_prac_type!=(int8u)-1 && L.loud_prac_type) || L.truepk!=(int16u)-1 || L.lra!=(int16u)-1)
            {
                Fill(Stream_Audio, 0, (P+" Loudness").c_str(), "Yes");
                if (L.loudspchgat!=(int16u)-1)
                {
                    string IntegratedLoudness_Speech=Ztring::ToZtring((L.loudspchgat-1024)/10.0, 1).To_UTF8()+" LKFS";
                    if (L.loud_dialgate_prac_type)
                        IntegratedLoudness_Speech+=" ("+Value(Ac4_loud_dialgate_prac_type, L.loud_dialgate_prac_type)+')';
                    Fill(Stream_Audio, 0, (P+" Loudness IntegratedLoudness_Speech").c_str(), IntegratedLoudness_Speech);
                }
                if (L.loudrelgat!=(int16u)-1)
                    Fill(Stream_Audio, 0, (P+" Loudness IntegratedLoudness_Level").c_str(), Ztring::ToZtring((L.loudrelgat-1024)/10.0, 1)+__T(" LKFS"));
                if (L.loud_prac_type!=(int8u)-1 && L.loud_prac_type)
                {
                    Fill(Stream_Audio, 0, (P+" Loudness AudioLoudnessStandard").c_str(), Value(Ac4_loud_prac_type, L.loud_prac_type));
                    Fill(Stream_Audio, 0, (P+" Loudness RealtimeLoudnessCorrected").c_str(), L.b_loudcorr_type?"Yes":"No");
                    string DialogueCorrected=L.loud_dialgate_prac_type!=(int8u)-1?"Yes":"No";
                    if (L.loud_dialgate_prac_type!=(int8u)-1 && L.loud_dialgate_prac_type)
                        DialogueCorrected+=" ("+Value(Ac4_loud_dialgate_prac_type, L.loud_dialgate_prac_type)+')';
                    Fill(Stream_Audio, 0, (P+" Loudness DialogueCorrected").c_str(), DialogueCorrected);
                }
                if (L.truepk!=(int16u)-1)
                    Fill(Stream_Audio, 0, (P+" Loudness TruePeak").c_str(), Ztring::ToZtring((L.truepk-1024)/10.0, 1)+__T(" dBTP"));
                if (L.max_loudmntry!=(int16u)-1)
                    Fill(Stream_Audio, 0, (P+" Loudness MaximumMomentaryLoudness").c_str(), Ztring::ToZtring((L.max_loudmntry-1024)/10.0, 1)+__T(" LUFS"));
                if (L.lra!=(int16u)-1)
                    Fill(Stream_Audio, 0, (P+" Loudness Loudness_Range").c_str(), Ztring::ToZtring(L.lra/10.0, 1).To_UTF8()+" LU ("+Value(Ac4_lra_prac_type, L.lra_prac_type)+')');
            }
        }
        {
            const drc_info& D=Presentation_Current.DrcInfo;
            if (D.drc_eac3_profile!=(int8u)-1 || !D.Decoders.empty())
            {
                Fill(Stream_Audio, 0, (P + " DynamicRangeControl").c_str(), "Yes");
                if (D.drc_eac3_profile!=(int8u)-1)
                    Fill(Stream_Audio, 0, (P+" DynamicRangeControl Eac3DrcProfile").c_str(), Value(Ac4_drc_eac3_profile, D.drc_eac3_profile));
                for (size_t i=0; i<D.Decoders.size(); i++)
                {
                    drc_decoder_config C=D.Decoders[i];
                    string V;
                    if (C.drc_default_profile_flag)
                        V=Value(Ac4_drc_eac3_profile, D.drc_eac3_profile);
                    else if (C.drc_compression_curve_flag)
                        V="Custom";
                    else
                        V="DRC gains";
                    Fill(Stream_Audio, 0, (P+" DynamicRangeControl "+Value(Ac4_drc_decoder_mode_id, C.drc_decoder_mode_id)).c_str(), V);
                }
            }
        }
        {
            const dmx& D=Presentation_Current.Dmx;
            if (D.loro_centre_mixgain!=(int8u)-1 || D.ltrt_centre_mixgain!=(int8u)-1 || D.lfe_mixgain!=(int8u)-1 || D.preferred_dmx_method!=(int8u)-1)
            {
                Fill(Stream_Audio, 0, (P+" Downmix").c_str(), "Yes");
                if (D.loro_centre_mixgain!=(int8u)-1)
                {
                    Fill(Stream_Audio, 0, (P+" Downmix LoRoCenterMixGain").c_str(), Value(Ac4_loro_centre_mixgain, D.loro_centre_mixgain, 1)+" dB");
                    Fill(Stream_Audio, 0, (P+" Downmix LoRoSurroundMixGain").c_str(), Value(Ac4_loro_centre_mixgain, D.loro_surround_mixgain, 1)+" dB");
                }
                if (D.ltrt_centre_mixgain!=(int8u)-1)
                {
                    Fill(Stream_Audio, 0, (P+" Downmix LtRtCenterMixGain").c_str(), Value(Ac4_loro_centre_mixgain, D.ltrt_centre_mixgain, 1)+" dB");
                    Fill(Stream_Audio, 0, (P+" Downmix LtRtSurroundMixGain").c_str(), Value(Ac4_loro_centre_mixgain, D.ltrt_surround_mixgain, 1)+" dB");
                }
                if (D.lfe_mixgain!=(int8u)-1)
                    Fill(Stream_Audio, 0, (P+" Downmix LfeMixGain").c_str(), Value(Ac4_loro_centre_mixgain, D.lfe_mixgain, 1)+" dB");
                if (D.preferred_dmx_method!=(int8u)-1)
                    Fill(Stream_Audio, 0, (P+" Downmix PreferredDownmix").c_str(), Value(Ac4_preferred_dmx_method, D.preferred_dmx_method));
            }
        }

        for (size_t s=0; s<Presentation_Current.substream_group_info_specifiers.size(); s++)
        {
            Fill(Stream_Audio, 0, (P+" GroupIDs").c_str(), Presentation_Current.substream_group_info_specifiers[s]);
        }
    }
    for (size_t g=0; g<Groups.size(); g++)
    {
        string G=Ztring(__T("Group")+Ztring::ToZtring(g)).To_UTF8();
        const group& Group=Groups[g];
        string Summary;
        for (size_t p=0; p<Presentations.size(); p++)
        {
            const presentation& Presentation_Current=Presentations[p];
            for (size_t s=0; s<Presentation_Current.substream_group_info_specifiers.size(); s++)
            {
                const size_t& Specifier=Presentation_Current.substream_group_info_specifiers[s];
                if (Specifier==g)
                {
                    string Summary2=Presentation_Current.presentation_config==(int8u)-1?"Main":Value(Ac4_presentation_config, Presentation_Current.presentation_config);
                    if (Summary2!=Summary)
                    {
                        if (!Summary.empty())
                            Summary+=" / ";
                        Summary+=Summary2;
                    }
                }
            }
        }
        if (!Group.ContentInfo.language_tag_bytes.empty())
        {
            Summary+=" (";
            Summary+=MediaInfoLib::Config.Language_Get(Ztring().From_UTF8("Language_"+Group.ContentInfo.language_tag_bytes)).To_UTF8();
            Summary+=')';
        }
        Fill(Stream_Audio, 0, Ztring(__T("Group")+Ztring::ToZtring(g)).To_UTF8().c_str(), Summary);
        if (Group.ContentInfo.content_classifier!=(int8u)-1)
            Fill(Stream_Audio, 0, (G+" Classifier").c_str(), Value(Ac4_content_classifier, Group.ContentInfo.content_classifier));
        if (!Group.ContentInfo.language_tag_bytes.empty())
        {
            Fill(Stream_Audio, 0, (G+" Language").c_str(), Group.ContentInfo.language_tag_bytes);
            Fill(Stream_Audio, 0, (G+" Language/String").c_str(), MediaInfoLib::Config.Language_Get(Ztring().From_UTF8("Language_"+Group.ContentInfo.language_tag_bytes)));
            Fill_SetOptions(Stream_Audio, 0, (G+" Language").c_str(), "N NTY");
            Fill_SetOptions(Stream_Audio, 0, (G+" Language/String").c_str(), "Y NTN");
        }
        Fill(Stream_Audio, 0, (G+" ChannelCoded").c_str(), Group.b_channel_coded?"Yes":"No");
        Fill(Stream_Audio, 0, (G+" NumberOfSubstreams").c_str(), Group.Substreams.size());
        for (size_t s=0; s<Group.Substreams.size(); s++)
        {
            const group_substream& Substream=Group.Substreams[s];
            if (Substream.ch_mode!=(int8u)-1)
                Fill(Stream_Audio, 0, (G+" SubstreamIDs").c_str(), Substream.substream_index);
        }
    }
    for (map<int8u, audio_substream>::iterator Substream_Info=AudioSubstreams.begin(); Substream_Info!=AudioSubstreams.end(); Substream_Info++)
    {
        string Summary;
        for (size_t g=0; g<Groups.size(); g++)
        {
            const group& Group=Groups[g];
            for (size_t s=0; s<Group.Substreams.size(); s++)
            {
                const group_substream& Substream=Group.Substreams[s];
                if (Substream.substream_index==Substream_Info->first && Substream.ch_mode!=(int8u)-1)
                {
                    string Summary2=Value(Ac4_ch_mode, Substream.ch_mode);
                    if (Summary2!=Summary)
                    {
                        if (!Summary.empty())
                            Summary+=" / ";
                        Summary+=Summary2;
                    }
                }
            }
        }
        if (Summary.empty())
        {
            Summary="?";
        }
        string S=Ztring(__T("Substream")+Ztring::ToZtring(Substream_Info->first)).To_UTF8();
        Fill(Stream_Audio, 0, S.c_str(), Summary);
        Fill(Stream_Audio, 0, (S+" ChannelLayout").c_str(), Summary); //TODO layout
        if (Substream_Info->second.LoudnessInfo.dialnorm_bits!=(int8u)-1)
            Fill(Stream_Audio, 0, (S+" dialnorm").c_str(), -0.25*Substream_Info->second.LoudnessInfo.dialnorm_bits, 2);
        if (Substream_Info->second.LoudnessInfo.truepk!=(int16u)-1)
            Fill(Stream_Audio, 0, (S+" TruePeak").c_str(), (Substream_Info->second.LoudnessInfo.truepk-1024)/10.0, 1);
        {
            const preprocessing& P=Substream_Info->second.Preprocessing;
            if (P.pre_dmixtyp_2ch!=(int8u)-1 || P.pre_dmixtyp_5ch!=(int8u)-1 || P.phase90_info_mc!=(int8u)-1)
            {
                Fill(Stream_Audio, 0, (S+" Preprocessing").c_str(), "Yes");
                if (P.pre_dmixtyp_2ch!=(int8u)-1)
                {
                    Fill(Stream_Audio, 0, (S+" Preprocessing PreviousMixType2ch").c_str(), Value(Ac4_pre_dmixtyp_2ch, P.pre_dmixtyp_2ch));
                    Fill(Stream_Audio, 0, (S+" Preprocessing Phase90FilterInfo2ch").c_str(), Value(Ac4_phase90_info_2ch, P.phase90_info_2ch));
                }
                if (P.pre_dmixtyp_5ch!=(int8u)-1)
                {
                    Fill(Stream_Audio, 0, (S+" Preprocessing PreviousDownmixType5ch").c_str(), Value(Ac4_pre_dmixtyp_5ch, P.pre_dmixtyp_5ch));
                }
                if (P.phase90_info_mc!=(int8u)-1)
                {
                    Fill(Stream_Audio, 0, (S+" Preprocessing Phase90FilterInfo").c_str(), Value(Ac4_phase90_info_mc, P.phase90_info_mc));
                    Fill(Stream_Audio, 0, (S+" Preprocessing SurroundAttenuationKnown").c_str(), P.b_surround_attenuation_known?"Yes":"No");
                    Fill(Stream_Audio, 0, (S+" Preprocessing LfeAttenuationKnown").c_str(), P.b_lfe_attenuation_known?"Yes":"No");
                }
            }
        }
        {
            const de_info& D=Substream_Info->second.DeInfo;
            if (D.b_de_data_present)
            {
                Fill(Stream_Audio, 0, (S+" DialogueEnhancement").c_str(), "Yes");
                Fill(Stream_Audio, 0, (S+" DialogueEnhancement Enabled").c_str(), "Yes");
                if (D.Config.de_method!=(int8u)-1)
                {
                    Fill(Stream_Audio, 0, (S+" DialogueEnhancement MaxGain").c_str(), Ztring::ToZtring((D.Config.de_max_gain+1)*3)+__T(" dB"));
                    Fill(Stream_Audio, 0, (S+" DialogueEnhancement ChannelConfiguration").c_str(), Value(Ac4_de_channel_config, D.Config.de_channel_config));
                }
            }
        }
    }
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

    Substream_Size.clear();

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
void File_Ac4::raw_ac4_frame()
{
    Element_Begin1("raw_ac4_frame");
    BS_Begin();
    ac4_toc();

    if (Element_Offset==Element_Size)
    {
        Element_End0();
        return; //Not parsing this frame
    }

    size_t byte_align=BS->Remain()%8;
    if (byte_align)
        Skip_S1(byte_align,                                     "byte_align");
    BS_End();
    if (payload_base)
        Skip_XX(payload_base,                                   "fill_area");
    int64u Substreams_StartOffset=Element_Offset;

    //Check integrity
    if (Substream_Size.empty())
        Substream_Size.push_back(Element_Size-Substreams_StartOffset); // 1 substream only
    size_t Substreams_EndOffset=Substreams_StartOffset;
    for (size_t i=0; i<Substream_Size.size(); i++)
        Substreams_EndOffset+=Substream_Size[i];
    if (Substreams_EndOffset>Element_Size)
    {
        Fill(Stream_Audio, 0, "NOK", "index", -1, true, true);//TODO remove
        Skip_XX(Element_Size-Element_Offset,                    "?");
        return;
    }

    //Parsing presentation substreams first
    if (bitstream_version>=2)
        for (size_t i=0; i<Presentations.size(); i++)
        {
            if (Presentations[i].presentation_version>=1)
            {
                int8u substream_index=Presentations[i].substream_index;
                if (substream_index>=Substream_Size.size())
                {
                    Fill(Stream_Audio, 0, "NOK", "index", -1, true, true);//TODO remove
                    Skip_XX(Element_Size-Element_Offset,        "?");
                    return;
                }
                Element_Offset=Substreams_StartOffset;
                for (size_t i=0; i<substream_index; i++)
                    Element_Offset+=Substream_Size[i];
                int64u Element_Size_Save=Element_Size;
                Element_Size=Element_Offset+Substream_Size[substream_index];

                ac4_presentation_substream(substream_index, i);

                if (Element_Offset<Element_Size)
                {
                    Fill(Stream_Audio, 0, "NOK", "presentation_substream too much", -1, true, true);//TODO remove
                    Skip_XX(Element_Size-Element_Offset,                "?");
                }
                Element_Size=Element_Size_Save;
            }
        }

    //Parsing other substreams
    for (int8u substream_index=0; substream_index<n_substreams; substream_index++)
    {
        Element_Offset=Substreams_StartOffset;
        for (size_t i=0; i<substream_index; i++)
            Element_Offset+=Substream_Size[i];
        int64u Element_Size_Save=Element_Size;
        Element_Size=Element_Offset+Substream_Size[substream_index];

        switch(Substream_Type[substream_index])
        {
            case Type_Ac4_Substream:
                ac4_substream(substream_index);
                break;
            case Type_Ac4_Hsf_Ext_Substream:
                Skip_XX(Substream_Size[substream_index],        "ac4_hsf_ext_substream");
                Param_Info1(substream_index);
                break;
            case Type_Emdf_Payloads_Substream:
                Skip_XX(Substream_Size[substream_index],        "emdf_payloads_substream");
                Param_Info1(substream_index);
                break;
            case Type_Ac4_Presentation_Substream:
                  Element_Offset=Element_Size; // Previously parsed, skip
                  break;
            case Type_Oamd_Substream:
                Skip_XX(Substream_Size[substream_index],        "oamd_substream");
                Param_Info1(substream_index);
                break;
            default:
                Skip_XX(Substream_Size[substream_index],        "substream_data");
                Param_Info1(substream_index);
        }

        if (Element_Offset<Element_Size)
        {
            Fill(Stream_Audio, 0, "NOK", "substream too much", -1, true, true);//TODO remove
            Skip_XX(Element_Size-Element_Offset,                "?");
        }
        Element_Size=Element_Size_Save;
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_toc()
{
    int16u sequence_counter, n_presentations;
    bool b_iframe_global;
    Element_Begin1("raw_ac4_toc");
    Get_S1 (2, bitstream_version,                               "bitstream_version");
    if (bitstream_version==3)
    {
        int32u bitstream_version32; 
        Get_V4 (2, bitstream_version32,                         "bitstream_version");
        bitstream_version32+=3;
        bitstream_version=(int8u)bitstream_version32;
    }
    Fill(Stream_Audio, 0, "bitstream_version", bitstream_version, 10, true);//TODO remove
    Get_S2 (10, sequence_counter,                               "sequence_counter");
    TEST_SB_SKIP(                                               "b_wait_frames");
        int8u wait_frames;
        Get_S1 (3, wait_frames,                                 "wait_frames");
        if (wait_frames)
            Skip_S1(2,                                          "br_code");
    TEST_SB_END();
    Get_SB (   fs_index,                                        "fs_index"); Param_Info1(Ztring::ToZtring(Ac4_fs_index[fs_index])+__T(" Hz"));
    Get_S1 (4, frame_rate_index,                                "frame_rate_index"); Param_Info2(Ac4_frame_rate[fs_index][frame_rate_index], " fps");
    Get_SB (   b_iframe_global,                                 "b_iframe_global");
    if (!b_iframe_global)
    {
        //We parse only Iframes
        BS_End();
        Element_Offset=Element_Size;
        return;
    }

    IFrames.push_back(Frame_Count); //TODO
    Presentations.clear();
    Presentation_Current=NULL;
    Groups.clear();
    Group_Current=NULL;
    AudioSubstreams.clear();
    max_group_index=0;

    TESTELSE_SB_SKIP(                                           "b_single_presentation");
        n_presentations=1;
    TESTELSE_SB_ELSE(                                           "b_single_presentation");
        TESTELSE_SB_SKIP(                                       "b_more_presentations");
            int32u n_presentations32;
            Get_V4 (2, n_presentations32,                       "n_presentations_minus2");
            n_presentations32+=2;
            n_presentations=(int8u)n_presentations32;
            Param_Info1(n_presentations);
        TESTELSE_SB_ELSE(                                       "b_more_presentations");
            n_presentations=0;
        TESTELSE_SB_END();
    TESTELSE_SB_END();

    payload_base=0;
    TEST_SB_SKIP(                                               "b_payload_base");
        Get_S1 (5, payload_base,                                "payload_base_minus1");
        payload_base++;
        if (payload_base==32)
        {
            int32u payload_base32;
            Get_V4 (3, payload_base32,                          "payload_base");
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

    size_t byte_align=BS->Remain()%8;
    if (byte_align)
        Skip_S1(byte_align,                                     "byte_align");

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_info()
{
    Presentations.resize(Presentations.size()+1);
    Presentation_Current=&Presentations.back();

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
        Param_Info1(Value(Ac4_presentation_config, presentation_config));
    }

    Get_VB (Presentation_Current->presentation_version,         "presentation_version");
    Fill(Stream_Audio, 0, "presentation_version", Presentation_Current->presentation_version, 10, true);//TODO remove

    if (!b_single_substream && presentation_config==6)
    {
        b_add_emdf_substreams=true;
    }
    else
    {
        Skip_S1(3,                                              "mdcompat");

        TEST_SB_SKIP(                                           "b_presentation_id");
            Skip_V4(2,                                          "presentation_id");
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
    Presentations.resize(Presentations.size()+1);
    Presentation_Current=&Presentations.back();

    bool b_single_substream_group, b_add_emdf_substreams=false;
    int8u presentation_config, n_substream_groups=0, b_multi_pid_PresentAndValue=(int8u)-1;

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
        Param_Info1(Value(Ac4_presentation_config, presentation_config));
    }

    if (bitstream_version!=1)
    {
        Get_VB (Presentation_Current->presentation_version,     "presentation_version");
        Fill(Stream_Audio, 0, "presentation_version", Presentation_Current->presentation_version, 10, true);//TODO remove
    }
    else
        Presentation_Current->presentation_version=0;

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
            bool b_multi_pid;
            Get_SB(b_multi_pid,                                 "b_multi_pid");
            b_multi_pid_PresentAndValue=b_multi_pid;
            switch (presentation_config) // TODO: Symplify
            {
            case 0: // Music and Effects + Dialogue
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                n_substream_groups=2;
            break;
            case 1: // Main + DE
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                n_substream_groups=1;
            break;
            case 2: // Main + Associated Audio
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                n_substream_groups=2;
            break;
            case 3: // Music and Effects + Dialogue + Associated Audio
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                n_substream_groups=3;
            break;
            case 4: // Main + DE + Associated Audio
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                ac4_sgi_specifier();
                n_substream_groups=2;
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
            break;
            default: // EMDF and other data
                presentation_config_ext_info(presentation_config);
            break;
            }
        }
        Skip_SB(                                                "b_pre_virtualized");
        Get_SB(b_add_emdf_substreams,                           "b_add_emdf_substreams");
        ac4_presentation_substream_info();
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

    if (Presentation_Current)
    {
        Presentation_Current->n_substream_groups=n_substream_groups;
        Presentation_Current->b_multi_pid_PresentAndValue=b_multi_pid_PresentAndValue;
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

        if (Presentation_Current)
            Presentation_Current->substream_group_info_specifiers.push_back(group_index);
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info()
{
    int8u ch_mode, substream_index;
    Element_Begin1(                                             "ac4_substream_info");
        bool b_content_type;
        content_info ContentInfo;
        Get_V4(Ac4_channel_mode, ch_mode,                       "channel_mode");
        if (ch_mode==12)
        {
            int32u channel_mode32;
            Get_V4(2, channel_mode32,                           "channel_mode");
            ch_mode+=(int8u)channel_mode32;
        }
        Param_Info1(Value(Ac4_ch_mode, ch_mode));

        if (fs_index)
        {
            TEST_SB_SKIP(                                       "b_sf_multiplier");
                Skip_SB(                                        "sf_multiplier");
            TEST_SB_END();
        }

        TEST_SB_SKIP(                                           "b_bitrate_info");
            Skip_V4(3, 5, 1,                                    "bitrate_indicator");
        TEST_SB_END();
        if (ch_mode>=7 && ch_mode<=10)
            Skip_SB(                                            "add_ch_base");

        TEST_SB_GET(b_content_type,                             "b_content_type");
            content_type(ContentInfo);
        TEST_SB_END();

        vector<bool> b_iframes;
        for (int8u Pos=0; Pos<frame_rate_factor; Pos++)
        {
            bool b_iframe;
            Get_SB (b_iframe,                                   "b_iframe");
            b_iframes.push_back(b_iframe);
        }

        Get_S1(2, substream_index,                              "substream_index");
        if (substream_index==3)
        {
            int32u substream_index32;
            Get_V4(2, substream_index32,                        "substream_index");
            substream_index32+=3;
            substream_index=(int8u)substream_index32;
        }
        
        for (size_t i=0; i<frame_rate_factor; i++)
        {
            Substream_Type[substream_index]=Type_Ac4_Substream;

            audio_substream* AudioSubstream_Current=&AudioSubstreams[substream_index];
            AudioSubstream_Current->Sus_Ver=false;
            AudioSubstream_Current->Channel_Coded=true; /* TODO: correct value ? */
            AudioSubstream_Current->ch_mode=ch_mode;
            if (b_content_type)
                AudioSubstream_Current->ContentInfo=ContentInfo;
            AudioSubstream_Current->b_iframe=b_iframes[i];
            
            substream_index++;
        }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_group_info()
{
    Groups.resize(Groups.size()+1);
    Group_Current=&Groups.back();

    bool sus_ver, b_channel_coded, b_substreams_present, b_hsf_ext, b_single_substream;
    int8u n_lf_substreams;

    Element_Begin1(                                             "ac4_substream_group_info");
    Get_SB(b_substreams_present,                                "b_substreams_present");
    Get_SB(b_hsf_ext,                                           "b_hsf_ext");
    TESTELSE_SB_GET(b_single_substream,                         "b_single_substream");
        n_lf_substreams=1;
    TESTELSE_SB_ELSE(                                           "b_single_substream");
        Get_S1(2, n_lf_substreams,                              "n_lf_substreams_minus2");
        n_lf_substreams+=2;
        if (n_lf_substreams==5)
        {
            int32u n_lf_substreams32;
            Get_V4(2, n_lf_substreams32,                        "n_lf_substreams");
            n_lf_substreams+=(int8u)n_lf_substreams32;
        }
    TESTELSE_SB_END();

    TESTELSE_SB_GET(b_channel_coded,                            "b_channel_coded");
        for (int8u Pos=0; Pos<n_lf_substreams; Pos++)
        {
            if (bitstream_version==1)
                Get_SB(sus_ver,                                 "sus_ver");
            else
                sus_ver=1;

            ac4_substream_info_chan(sus_ver);
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
                obj Obj;
				ac4_substream_info_obj(Obj, b_substreams_present);
            TESTELSE_SB_END();
            if (b_hsf_ext)
            {
                ac4_hsf_ext_substream_info(b_substreams_present);
            }
        }
    TESTELSE_SB_END();

    TEST_SB_SKIP(                                               "content_type");
        content_type(Group_Current->ContentInfo);
    TEST_SB_END();

    if (Group_Current)
    {
        Group_Current->b_channel_coded=b_channel_coded;
        Group_Current->b_hsf_ext=b_hsf_ext;
    }

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

        Substream_Type[substream_index]=Type_Ac4_Hsf_Ext_Substream;

        if (Group_Current)
        {
            Group_Current->Substreams.resize(Group_Current->Substreams.size()+1);
            Group_Current->Substreams.back().substream_type=Type_Ac4_Hsf_Ext_Substream;
            Group_Current->Substreams.back().substream_index=substream_index;
            Group_Current->Substreams.back().ch_mode=(int8u)-1;
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info_chan(bool sus_ver)
{
    Fill(Stream_Audio, 0, "sus_ver", sus_ver, 10, true);//TODO remove
    bool b_4_back_channels_present=false;
    int8u top_channels_present=0;
    int8u substream_index;
    int8u ch_mode;
    Element_Begin1(                                             "ac4_substream_info_chan");
    Get_V4(Ac4_channel_mode2, ch_mode,                          "channel_mode");
    if (ch_mode==16)
    {
        int32u channel_mode32;
        Get_V4(2, channel_mode32,                               "channel_mode");
        ch_mode+=(int8u)channel_mode32;
    }
    Param_Info1(Value(Ac4_ch_mode, ch_mode));

    if (ch_mode>=11 && ch_mode<=14)
    {
        Get_SB (b_4_back_channels_present,                      "b_4_back_channels_present");
        Skip_SB(                                                "b_centre_present");
        Get_S1 (2, top_channels_present,                        "top_channels_present");
    }

    if (fs_index==1)
    {
        TEST_SB_SKIP(                                           "b_sf_multiplier");
            Skip_SB(                                            "sf_multiplier");
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_bitrate_info");
        Skip_V4(3, 5, 1,                                        "bitrate_indicator");
    TEST_SB_END();

    if (ch_mode>=7 && ch_mode<=10)
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

    Substream_Type[substream_index]=Type_Ac4_Substream;

    audio_substream* AudioSubstream_Current=&AudioSubstreams[substream_index];
    AudioSubstream_Current->Sus_Ver=sus_ver;
    AudioSubstream_Current->Channel_Coded=true;
    AudioSubstream_Current->ch_mode=ch_mode;
    AudioSubstream_Current->b_iframe=true; //TODO if b_global_iframe is 0

    if (Group_Current)
    {
        Group_Current->Substreams.resize(Group_Current->Substreams.size()+1);
        Group_Current->Substreams.back().substream_type=Type_Ac4_Substream;
        Group_Current->Substreams.back().substream_index=substream_index;
        Group_Current->Substreams.back().ch_mode=ch_mode;
        switch (Group_Current->Substreams.back().ch_mode)
        {
            case 11:
            case 13:
                        Group_Current->Substreams.back().ch_mode_core=5;
                        break;
            case 12:
            case 14:
                        Group_Current->Substreams.back().ch_mode_core=6;
                        break;
        }
        Group_Current->Substreams.back().b_4_back_channels_present=b_4_back_channels_present;
        Group_Current->Substreams.back().top_channels_present=top_channels_present;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info_ajoc(bool b_substreams_present)
{
    bool b_lfe, b_static_dmx;
    int8u n_fullband_dmx_signals, n_fullband_upmix_signals, substream_index;
    Element_Begin1(                                             "ac4_substream_info_ajoc");
    Get_SB(b_lfe,                                               "b_lfe");
    TESTELSE_SB_GET(b_static_dmx,                               "b_static_dmx");
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
        Skip_V4(3, 5, 1,                                        "bitrate_indicator");
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

        Substream_Type[substream_index]=Type_Ac4_Substream;

        audio_substream* AudioSubstream_Current=&AudioSubstreams[substream_index];
        AudioSubstream_Current->Sus_Ver=true;
        AudioSubstream_Current->Channel_Coded=false;
        AudioSubstream_Current->ch_mode=(int8u)-1;

        if (Group_Current)
        {
            Group_Current->Substreams.resize(Group_Current->Substreams.size()+1);
            Group_Current->Substreams.back().substream_type=Type_Ac4_Substream;
            Group_Current->Substreams.back().substream_index=substream_index;
            Group_Current->Substreams.back().b_static_dmx=b_static_dmx;
            Group_Current->Substreams.back().ch_mode=(int8u)-1;
            if (b_static_dmx)
                Group_Current->Substreams.back().ch_mode_core=b_lfe?4:3;
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_substream_info_obj(obj& O, bool b_substreams_present)
{
    int8u substream_index;
    Element_Begin1(                                             "ac4_substream_info_obj");
    Get_S1 (3, O.n_objects_code,                                "n_objects_code");
    TESTELSE_SB_GET (O.b_dynamic_objects,                       "b_dynamic_objects");
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
        Skip_V4(3, 5, 1,                                        "bitrate_indicator");
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

        Substream_Type[substream_index]=Type_Ac4_Substream;

        audio_substream* AudioSubstream_Current=&AudioSubstreams[substream_index];
        AudioSubstream_Current->Sus_Ver=true;
        AudioSubstream_Current->Channel_Coded=false;
        AudioSubstream_Current->ch_mode=(int8u)-1;

        if (Group_Current)
        {
            Group_Current->Substreams.resize(Group_Current->Substreams.size()+1);
            Group_Current->Substreams.back().substream_type=Type_Ac4_Substream;
            Group_Current->Substreams.back().substream_index=substream_index;
            Group_Current->Substreams.back().ch_mode=(int8u)-1;
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_substream_info()
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

    if (Presentation_Current)
    {
        Presentation_Current->substream_index=substream_index;
        Presentation_Current->b_alternative=b_alternative;
        Presentation_Current->b_pres_ndot=b_pres_ndot;
    }

    Substream_Type[substream_index]=Type_Ac4_Presentation_Substream;
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::presentation_config_ext_info(int8u presentation_config)
{
    Element_Begin1(                                             "presentation_config_ext_info");
    int16u n_skip_bytes; // TODO: verify max size

    Get_S2 (5, n_skip_bytes,                                    "n_skip_bytes");
    TEST_SB_SKIP(                                               "b_more_skip_bytes");
        int32u n_skip_bytes32;
        Get_V4 (2, n_skip_bytes32,                              "n_skip_bytes");
        n_skip_bytes=(int8u)n_skip_bytes32<<5;
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
void File_Ac4::content_type(content_info& ContentInfo)
{
    Element_Begin1(                                             "content_type");
        int8u content_classifier;
        Get_S1 (3, content_classifier,                          "content_classifier");
        TEST_SB_SKIP(                                           "b_language_indicator");
            TESTELSE_SB_SKIP(                                   "b_serialized_language_tag");
                Skip_SB(                                        "b_start_tag");
                Skip_S2(16,                                     "language_tag_chunk");
            TESTELSE_SB_ELSE(                                   "b_serialized_language_tag");
                int8u n_language_tag_bytes;
                Get_S1(6, n_language_tag_bytes,                 "n_language_tag_bytes");
                for (int8u Pos=0; Pos<n_language_tag_bytes; Pos++)
                {
                    int8u language_tag_bytes;
                    Get_S1 (8, language_tag_bytes,              "language_tag_bytes");
                    ContentInfo.language_tag_bytes+=(language_tag_bytes<0x80?language_tag_bytes:'?');
                }
            TESTELSE_SB_END();
        TEST_SB_END();

        ContentInfo.content_classifier=content_classifier;
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
    Substream_Type[substream_index]=Type_Emdf_Payloads_Substream;

    if (Group_Current)
    {
        Group_Current->Substreams.resize(Group_Current->Substreams.size()+1);
        Group_Current->Substreams.back().substream_type=Type_Emdf_Payloads_Substream;
        Group_Current->Substreams.back().substream_index=substream_index;
        Group_Current->Substreams.back().ch_mode=(int8u)-1;
    }
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
                substream_size+=(int16u)substream_size32<<10;
                Param_Info1(substream_size);
            }
            Substream_Size.push_back(substream_size);
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
        
        Substream_Type[substream_index]=Type_Oamd_Substream;
        
        if (Group_Current)
        {
            Group_Current->Substreams.resize(Group_Current->Substreams.size()+1);
            Group_Current->Substreams.back().substream_type=Type_Oamd_Substream;
            Group_Current->Substreams.back().substream_index=substream_index;
            Group_Current->Substreams.back().ch_mode=(int8u)-1;
        }
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
void File_Ac4::ac4_substream(size_t substream_index)
{
    int32u audio_size;

    Element_Begin1("ac4_substream");
    Element_Info1(Ztring::ToZtring(substream_index));
    BS_Begin();
    size_t Pos_Before=Data_BS_Remain();
    Get_S4(15, audio_size,                                      "audio_size_value");
    TEST_SB_SKIP(                                               "b_more_bits");
        int32u audio_size32;
        Get_V4(7, audio_size32,                                 "audio_size_value");
        audio_size+=audio_size32<<15;
    TEST_SB_END();

    // Skip audio
    Skip_BS(audio_size*8,                                       "audio_data");

    metadata(substream_index);

    size_t Pos_After=Data_BS_Remain();
    if (Pos_Before-Pos_After<(Substream_Size[substream_index]*8))
    {
        size_t Bits=(Substream_Size[substream_index]*8)-(Pos_Before-Pos_After);
        bool Align=false;
        if (Bits<8)
        {
            int8u Value;
            Peek_S1((int8u)Bits, Value);
            if (!Value)
                Align=true;
        }
        Skip_BS(Bits, Align?"byte_align":"?");
    }
    BS_End();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::ac4_presentation_substream(size_t substream_index, size_t Substream_Index)
{
    presentation& Presentation=Presentations[Substream_Index];
    loudness_info& L=Presentation.LoudnessInfo;

    int8u name_length=32, n_targets, add_data_bytes;

    int8u pres_ch_mode=(int8u)-1, pres_ch_mode_core=(int8u)-1, pres_top_channel_pairs=0, n_substreams_in_presentation=0;
    bool b_obj_or_ajoc=false, b_obj_or_ajoc_adaptive=false, b_pres_4_back_channels_present=false, b_pres_has_lfe=false;

    for (size_t Pos=0; Pos<Presentation.substream_group_info_specifiers.size(); Pos++)
    {
        int8u Group_Index=Presentation.substream_group_info_specifiers[Pos];
        for (size_t Pos2=0; Pos2<Groups[Group_Index].Substreams.size(); Pos2++)
        {
            if (Groups[Group_Index].Substreams[Pos2].substream_type==Type_Ac4_Substream)
            {
                n_substreams_in_presentation++;

                // pres_ch_mode && pres_ch_mode_core
                if (Groups[Group_Index].Substreams[Pos2].ch_mode!=(int8u)-1) // channel coded
                {
                    pres_ch_mode=Superset(pres_ch_mode, Groups[Group_Index].Substreams[Pos2].ch_mode);
                    pres_ch_mode_core=Superset(pres_ch_mode_core, Groups[Group_Index].Substreams[Pos2].ch_mode_core);
                }
                else
                {
                    b_obj_or_ajoc=true;
                    if (Groups[Group_Index].Substreams[Pos2].b_static_dmx) // && b_ajoc
                        pres_ch_mode_core=Superset(pres_ch_mode_core, Groups[Group_Index].Substreams[Pos2].ch_mode_core);
                    else
                        b_obj_or_ajoc_adaptive=1;
                }

                // b_pres_4_back_channels_present
                if (Groups[Group_Index].Substreams[Pos2].b_4_back_channels_present)
                    b_pres_4_back_channels_present=true;

                // pres_top_channel_pairs
                switch (Groups[Group_Index].Substreams[Pos2].top_channels_present)
                {
                case 1:
                case 2:
                    pres_top_channel_pairs=pres_top_channel_pairs==0?1:pres_top_channel_pairs; break;
                case 3:
                    pres_top_channel_pairs=2; break;
                }
            }
        }
    }
    if (b_obj_or_ajoc)
        pres_ch_mode=-1;
    if (b_obj_or_ajoc_adaptive || pres_ch_mode_core==pres_ch_mode)
        pres_ch_mode_core=-1;

    // b_pres_has_lfe
    if (pres_ch_mode!=(int8u)-1)
        b_pres_has_lfe=Channel_Mode_Contains_Lfe(pres_ch_mode);
    else if (pres_ch_mode_core==4 || pres_ch_mode_core==6)
        b_pres_has_lfe=true;

    Element_Begin1("ac4_presentation_substream");
    Element_Info1(Ztring::ToZtring(substream_index));
    BS_Begin();
    if (Presentation.b_alternative)
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

            for (int8u Pos2=0; Pos< n_substreams_in_presentation; Pos++)
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
        Get_S1(4, add_data_bytes,                               "add_data_bytes_minus1");
        add_data_bytes++;
        if (add_data_bytes==16)
        {
            int32u add_data_bytes32;
            Get_V4(2, add_data_bytes32,                         "add_data_bytes32");
            add_data_bytes+=(int8u)add_data_bytes32;
        }
        size_t byte_align=BS->Remain()%8;
        if (byte_align)
            Skip_S1(byte_align,                                 "byte_align");
        Skip_BS(add_data_bytes*8,                               "add_data");
    TEST_SB_END();

    Get_S1 (7, L.dialnorm_bits,                                 "dialnorm_bits");
    TEST_SB_SKIP(                                               "b_further_loudness_info");
        further_loudness_info(Presentation.LoudnessInfo, true, true);
    TEST_SB_END();

    int16u drc_metadata_size_value;
    Get_S2(5, drc_metadata_size_value,                           "drc_metadata_size_value");
    TEST_SB_SKIP(                                                "b_more_bits");
        int32u drc_metadata_size_value32;
        Get_V4(3, drc_metadata_size_value32,                     "drc_metadata_size_value");
        drc_metadata_size_value+=(int16u)drc_metadata_size_value32<<5;
    TEST_SB_END();

    size_t Pos_Before=Data_BS_Remain();
    drc_frame(Presentation.DrcInfo, Presentation.b_pres_ndot);
    size_t Pos_After=Data_BS_Remain();
    if (drc_metadata_size_value!=Pos_Before-Pos_After)
    {
        Fill(Stream_Audio, 0, "NOK", "drc_metadata", -1, true, true);//TODO remove
        Element_Info1("Problem");
    }

    if (Presentation.n_substream_groups>1)
    {
        TEST_SB_SKIP(                                           "b_substream_group_gains_present");
            TESTELSE_SB_SKIP(                                   "b_keep");
            TESTELSE_SB_ELSE(                                   "b_keep");
                for (int8u Pos=0; Pos<Presentation.n_substream_groups; Pos++)
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

    if (pres_ch_mode<=4 || BS->Remain()>=4) //TODO: remove this when parsing issue is understood
    {
    custom_dmx_data(Presentation.Dmx, pres_ch_mode, pres_ch_mode_core, b_pres_4_back_channels_present, pres_top_channel_pairs, b_pres_has_lfe);
    loud_corr(pres_ch_mode, pres_ch_mode_core, false/* TODO: b_objects? */);
    }
    else
    {
        Fill(Stream_Audio, 0, "NOK", "presentation_substream", -1, true, true);//TODO remove

    }
    size_t byte_align=BS->Remain()%8;
    if (byte_align)
        Skip_S1(byte_align,                                     "byte_align");
    BS_End();
    Element_End0();
}
//---------------------------------------------------------------------------
void File_Ac4::metadata(size_t Substream_Index)
{
    if (AudioSubstreams[Substream_Index].ch_mode==(int8u)-1)
    {
        Skip_BS(Data_BS_Remain(),                               "metadata");
        Fill(Stream_Audio, 0, "NOK", "metadata ch_mode", -1, true, true);//TODO remove
        return;
    }

    bool b_associated=AudioSubstreams[Substream_Index].ContentInfo.content_classifier!=(int8u)-1 && AudioSubstreams[Substream_Index].ContentInfo.content_classifier>1; //TODO: from presentation_config if content_classifier not present
    bool b_dialog=AudioSubstreams[Substream_Index].ContentInfo.content_classifier==4; //TODO: from presentation_config if content_classifier not present

    Element_Begin1("metadata");
    basic_metadata(AudioSubstreams[Substream_Index].LoudnessInfo, AudioSubstreams[Substream_Index].Preprocessing, AudioSubstreams[Substream_Index].ch_mode, AudioSubstreams[Substream_Index].Sus_Ver);
    extended_metadata(b_associated, b_dialog, AudioSubstreams[Substream_Index].ch_mode, AudioSubstreams[Substream_Index].Sus_Ver);

    // TODO:
    // if (b_alternative && !b_ajoc)
    //     oamd_dyndata_single(n_objs, num_obj_info_blocks, b_iframe, b_alternative, obj_type[n_objs], b_lfe[n_objs]);

    int8u tools_metadata_size8;
    Get_S1(7, tools_metadata_size8,                             "tools_metadata_size");
    int32u tools_metadata_size=tools_metadata_size8;
    TEST_SB_SKIP(                                               "b_more_bits");
        int32u tools_metadata_size32;
        Get_V4 (3, tools_metadata_size32,                       "tools_metadata_size");
        tools_metadata_size+=tools_metadata_size32<<7;
    TEST_SB_END();

    if (false)
    {
        Skip_BS(tools_metadata_size,                            "tools_metadata");
    }
    else
    {
    size_t Pos_Before = Data_BS_Remain();
    if (!AudioSubstreams[Substream_Index].Sus_Ver)
        drc_frame(AudioSubstreams[Substream_Index].DrcInfo, AudioSubstreams[Substream_Index].b_iframe);
    dialog_enhancement(AudioSubstreams[Substream_Index].DeInfo, AudioSubstreams[Substream_Index].ch_mode, true /* TODO: b_iframe */);
    size_t Pos_After=Data_BS_Remain();
    if (tools_metadata_size!=Pos_Before-Pos_After)
    {
        Fill(Stream_Audio, 0, "NOK", "tools_metadata", -1, true, true);//TODO remove
        Element_Info1("Problem");
        if (tools_metadata_size>Pos_Before-Pos_After)
            Skip_BS(tools_metadata_size-(Pos_Before-Pos_After), "?");
    }
    }

    TEST_SB_SKIP("b_emdf_payloads_substream");
        for (;;)
        {
            int32u umd_payload_size;
            int8u umd_payload_id, extSizeBits;
            bool b_smpoffst;
            Get_S1 (5, umd_payload_id,                          "umd_payload_id");
            if (!umd_payload_id)
                break;
            //TODO variable_bits
            TEST_SB_GET(b_smpoffst, "b_smpoffst");
                Skip_V4(11, "smpoffst");
            TEST_SB_END();
            TEST_SB_SKIP("b_duration");
                Skip_V4(11, "duration");
            TEST_SB_END();
            TEST_SB_SKIP("b_groupid");
                Skip_V4(2, "groupid");
            TEST_SB_END();
            TEST_SB_SKIP("b_codecdata");
                Skip_V4(8, "b_codecdata");
            TEST_SB_END();
            TEST_SB_SKIP("b_discard_unknown_payload");
                bool b_payload_frame_aligned;
                if (!b_smpoffst)
                {
                    TEST_SB_GET(b_payload_frame_aligned, "b_payload_frame_aligned");
                        Skip_SB("b_create_duplicate");
                        Skip_SB("b_remove_duplicate");
                    TEST_SB_END();
                }
                if (b_smpoffst || b_payload_frame_aligned)
                {
                    Skip_S1(5, "priority");
                    Skip_S1(2, "proc_allowed");
                }
            TEST_SB_END();
            Get_V4 (8, umd_payload_size,                           "umd_payload_size");
            umd_payload_size++;

            switch (umd_payload_id)
            {
                default:
                    if (umd_payload_size)
                        Skip_BS(umd_payload_size,                            "(Unknown)");
            }
        }
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::basic_metadata(loudness_info& L, preprocessing& P, int8u ch_mode, bool sus_ver)
{
    Element_Begin1("basic_metadata");
    if (!sus_ver)
        Get_S1 (7, L.dialnorm_bits,                             "dialnorm_bits");

    TEST_SB_SKIP(                                               "b_more_basic_metadata");
        if (!sus_ver)
        {
            TEST_SB_SKIP(                                       "b_further_loudness_info");
                further_loudness_info(L, sus_ver, false);
            TEST_SB_END();
        }
        else
        {
            TEST_SB_SKIP(                                       "b_substream_loudness_info");
                Skip_S1(8,                                      "substream_loudness_bits");
                TEST_SB_SKIP(                                   "b_further_substream_loudness_info");
                    further_loudness_info(L, sus_ver, false);
                TEST_SB_END();
            TEST_SB_END();
        }

        if (ch_mode==1) // stereo
        {
            TEST_SB_SKIP(                                       "b_prev_dmx_info");
                Get_S1 (3, P.pre_dmixtyp_2ch,                   "pre_dmixtyp_2ch");
                Get_S1 (2, P.phase90_info_2ch,                  "phase90_info_2ch");
            TEST_SB_END();
        }
        else if (ch_mode!=(int8u)-1 && ch_mode>1)
        {
            if (!sus_ver)
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

                    if (Channel_Mode_Contains_Lfe(ch_mode))
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
                    Get_S1 (3, P.pre_dmixtyp_5ch,               "pre_dmixtyp_5ch");
                TEST_SB_END();

                TEST_SB_SKIP(                                   "b_preupmixtyp_5ch");
                    Skip_S1(4,                                  "pre_upmixtyp_5ch");
                TEST_SB_END();
            }

            if (ch_mode>=5 && ch_mode<=10)
            {
                TEST_SB_SKIP(                                   "b_upmixtyp_7ch");
                    if (ch_mode==5 || ch_mode==6)
                        Skip_S1(2,                              "pre_upmixtyp_3_4");
                    else if (ch_mode==9 || ch_mode==10)
                        Skip_SB(                                "pre_upmixtyp_3_2_2");
                    
                TEST_SB_END();
            }
            Get_S1 (2, P.phase90_info_mc,                       "phase90_info_mc");
            Get_SB (   P.b_surround_attenuation_known,          "b_surround_attenuation_known");
            Get_SB (   P.b_lfe_attenuation_known,               "b_lfe_attenuation_known");
        }

        TEST_SB_SKIP(                                           "b_dc_blocking");
            Skip_SB(                                            "dc_block_on");
        TEST_SB_END();
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::extended_metadata(bool b_associated, bool b_dialog, int8u ch_mode, bool sus_ver)
{
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
        if (Channel_Mode_Contains_C(ch_mode))
        {
            TEST_SB_SKIP(                                       "b_c_active");
                Skip_SB(                                        "b_c_has_dialog");
            TEST_SB_END();
        }
        if (Channel_Mode_Contains_Lr(ch_mode))
        {
            TEST_SB_SKIP(                                       "b_l_active");
                Skip_SB(                                        "b_l_has_dialog");
            TEST_SB_END();

            TEST_SB_SKIP(                                       "b_r_active");
                Skip_SB(                                        "b_r_has_dialog");
            TEST_SB_END();
        }
        if (Channel_Mode_Contains_LsRs(ch_mode))
        {
            Skip_SB(                                            "b_ls_active");
            Skip_SB(                                            "b_rs_active");
        }
        if (Channel_Mode_Contains_LrsRrs(ch_mode))
        {
            Skip_SB(                                            "b_lrs_active");
            Skip_SB(                                            "b_rrs_active");
        }
        if (Channel_Mode_Contains_LwRw(ch_mode))
        {
            Skip_SB(                                            "b_lw_active");
            Skip_SB(                                            "b_rw_active");
        }
        if (Channel_Mode_Contains_VhlVhr(ch_mode))
        {
            Skip_SB(                                            "b_vhl_active");
            Skip_SB(                                            "b_vhr_active");
        }
        if (Channel_Mode_Contains_Lfe(ch_mode))
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
void File_Ac4::dialog_enhancement(de_info& Info, int8u ch_mode, bool b_iframe)
{
    bool b_de_config_flag=b_iframe;

    Element_Begin1("dialog_enhancement");
    TEST_SB_GET (Info.b_de_data_present,                        "b_de_data_present");
        if (!b_iframe)
            Get_SB (b_de_config_flag,                           "b_de_config_flag");

        if (b_de_config_flag)
            dialog_enhancement_config(Info);

        dialog_enhancement_data(Info, b_iframe, 0);
        if (ch_mode==13 || ch_mode==14)
        {
            TEST_SB_SKIP(                                       "b_de_simulcast");
                dialog_enhancement_data(Info, b_iframe, 1);
            TEST_SB_END();
        }
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::dialog_enhancement_config(de_info& Info)
{
    Element_Begin1("de_config");
    Get_S1 (2, Info.Config.de_method,                           "de_method");
    Get_S1 (2, Info.Config.de_max_gain,                         "de_max_gain");
    Get_S1 (3, Info.Config.de_channel_config,                   "de_channel_config");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::dialog_enhancement_data(de_info& Info, bool b_iframe, bool b_de_simulcast)
{
    bool de_keep_pos_flag=false, de_keep_data_flag=false, de_ms_proc_flag=false;
    ac4_huffman abs_table=Info.Config.de_method%2==0?de_hcb_abs_0:de_hcb_abs_1;
    ac4_huffman diff_table=Info.Config.de_method%2==0?de_hcb_diff_0:de_hcb_diff_1;

    int8u de_nr_channels=0, de_nr_bands=8;
    switch (Info.Config.de_channel_config)
    {
        case 0b1:
        case 0b10:
        case 0b100:
            de_nr_channels=1; break;
        case 0b11:
        case 0b101:
        case 0b110:
            de_nr_channels=2; break;
        case 0b111:
            de_nr_channels=3; break;
    }

    Element_Begin1("de_data");
    if (de_nr_channels>0)
    {
        if (de_nr_channels>1 && (Info.Config.de_method==1 || Info.Config.de_method==3) && !b_de_simulcast)
        {
            if (!b_iframe)
                Get_SB(de_keep_pos_flag,                        "de_keep_pos_flag");

            if (!de_keep_pos_flag)
            {
                Skip_S1(5,                                      "de_mix_coef1_idx");
                if (de_nr_channels==3)
                    Skip_S1(5,                                  "de_mix_coef2_idx");
            }
        }

        if (!b_iframe)
            Get_SB(de_keep_data_flag,                           "de_keep_data_flag");

        if (!de_keep_data_flag)
        {

            if (de_nr_channels==2 && (Info.Config.de_method==0 || Info.Config.de_method==2))
                Skip_SB(                                        "de_ms_proc_flag");

            for (int8u ch=0; ch<de_nr_channels-de_ms_proc_flag; ch++)
            {
                if (b_iframe && ch==0)
                {
                    Huffman_Decode(abs_table, "de_par_code");
                    for (int8u band=1; band<de_nr_bands; band++)
                        Huffman_Decode(diff_table, "de_par_code");
                }
                else
                {
                    for (int8u band=0; band<de_nr_bands; band++)
                        Huffman_Decode(diff_table, "de_par_code");
                }
            }

            if (Info.Config.de_method>=2)
                Skip_S1(5,                                      "de_signal_contribution");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::custom_dmx_data(dmx& D, int8u pres_ch_mode, int8u pres_ch_mode_core, bool b_pres_4_back_channels_present, int8u pres_top_channel_pairs, bool b_pres_has_lfe)
{
    int8u bs_ch_config=(int8u)-1, n_cdmx_configs;
    if (pres_top_channel_pairs>0 && pres_ch_mode>=11 && pres_ch_mode<=14)
    {
        if (pres_top_channel_pairs==2)
        {
            if (pres_ch_mode >=13 && b_pres_4_back_channels_present)
                bs_ch_config=0;
            else if (pres_ch_mode<=12 && b_pres_4_back_channels_present)
                bs_ch_config=1;
            else if (pres_ch_mode<=12)
                    bs_ch_config=2;
        }
        else if (pres_top_channel_pairs==1)
        {
            if (pres_ch_mode>=13 && b_pres_4_back_channels_present)
                bs_ch_config=3;
            if (pres_ch_mode<=12 && b_pres_4_back_channels_present)
                bs_ch_config=4;
            else if (pres_ch_mode<=12)
                bs_ch_config = 5;
        }
    }

    Element_Begin1("custom_dmx_data");
    if (bs_ch_config!=(int8u)-1)
    {
        TEST_SB_SKIP(                                           "b_cdmx_data_present");
            Get_S1(2, n_cdmx_configs,                           "n_cdmx_configs_minus1");
            n_cdmx_configs++;

            for (int8u Pos=0; Pos<n_cdmx_configs; Pos++)
            {
                int8u out_ch_config;
                if (bs_ch_config==2 || bs_ch_config == 5)
                    Get_S1(1, out_ch_config,                    "out_ch_config[dc]");
                else
                    Get_S1(3, out_ch_config,                    "out_ch_config[dc]");

                cdmx_parameters(bs_ch_config, out_ch_config);
            }
        TEST_SB_END();
    }

    if ((pres_ch_mode!=(int8u)-1 && pres_ch_mode>=3) || (pres_ch_mode_core!=(int8u)-1 && pres_ch_mode_core>=3))
    {
        TEST_SB_SKIP(                                           "b_stereo_dmx_coeff");
            Get_S1 (3, D.loro_centre_mixgain,                   "loro_centre_mixgain");
            Get_S1 (3, D.loro_surround_mixgain,                 "loro_surround_mixgain");
            TEST_SB_SKIP(                                       "b_ltrt_mixinfo");
                Get_S1 (3, D.ltrt_centre_mixgain,               "ltrt_centre_mixgain");
                Get_S1 (3, D.ltrt_surround_mixgain,             "ltrt_surround_mixgain");
            TEST_SB_END();
            if (b_pres_has_lfe)
            {
                TEST_SB_SKIP(                                   "b_lfe_mixinfo");
                    Get_S1 (5, D.lfe_mixgain,                   "lfe_mixgain");
                TEST_SB_END();
            }
            Get_S1 (2, D.preferred_dmx_method,                  "preferred_dmx_method");
        TEST_SB_END();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::cdmx_parameters(int8u bs_ch_config, int8u out_ch_config)
{
    Element_Begin1("cdmx_parameters");
    if (bs_ch_config==0 || bs_ch_config==3)
        tool_scr_to_c_l();

    if (bs_ch_config<2)
    {
        switch (out_ch_config)
        {
        case 0:
            tool_t4_to_f_s();
            tool_b4_to_b2();
        break;
        case 1:
            tool_t4_to_t2();
            tool_b4_to_b2();
        break;
        case 2:
            tool_b4_to_b2();
        break;
        case 3:
            tool_t4_to_f_s_b();
        break;
        case 4:
            tool_t4_to_t2();
        break;
        }
    }
    else if (bs_ch_config==2)
    {
        switch (out_ch_config)
        {
        case 0:
            tool_t4_to_f_s();
        break;
        case 1:
            tool_t4_to_t2();
        break;
        }
    }
    else if (bs_ch_config==3 || bs_ch_config==4)
    {
        switch (out_ch_config)
        {
        case 0:
            tool_t2_to_f_s();
            tool_b4_to_b2();
        break;
        case 1:
            tool_b4_to_b2();
        break;
        case 2:
            tool_b4_to_b2();
        break;
        case 3:
            tool_t2_to_f_s_b();
        break;
        }
    }
    else if (bs_ch_config==5 && out_ch_config==0)
    {
        tool_t2_to_f_s();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_scr_to_c_l()
{
    Element_Begin1("tool_scr_to_c_l");
        TESTELSE_SB_SKIP(                                       "b_put_screen_to_c");
            Skip_S1(3,                                          "gain_f1_code");
        TESTELSE_SB_ELSE(                                       "b_put_screen_to_c");
            Skip_S1(3,                                          "gain_f2_code");
        TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_b4_to_b2()
{
    Element_Begin1("tool_b4_to_b2");
    Skip_S1(3,                                                  "gain_b_code");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_t4_to_t2()
{
    Element_Begin1("tool_t4_to_t2");
    Skip_S1(3,                                                  "gain_t1_code");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_t4_to_f_s_b()
{
    Element_Begin1("tool_t4_to_f_s_b");
    TESTELSE_SB_SKIP(                                           "b_top_front_to_front");
        Skip_S1(3,                                              "gain_t2a_code");
    TESTELSE_SB_ELSE(                                           "b_top_front_to_front");
        TESTELSE_SB_SKIP(                                       "b_top_front_to_side");
        Skip_S1(3,                                              "gain_t2b_code");
        TESTELSE_SB_ELSE(                                       "b_top_front_to_side");
        Skip_S1(3,                                              "gain_t2c_code");
        TESTELSE_SB_END();
    TESTELSE_SB_END();



    TESTELSE_SB_SKIP(                                           "b_top_back_to_front");
        Skip_S1(3,                                              "gain_t2d_code");
    TESTELSE_SB_ELSE(                                           "b_top_back_to_front");
        TESTELSE_SB_SKIP(                                       "b_top_back_to_side");
        Skip_S1(3,                                              "gain_t2e_code");
        TESTELSE_SB_ELSE(                                       "b_top_back_to_side");
        Skip_S1(3,                                              "gain_t2f_code");
        TESTELSE_SB_END();
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_t4_to_f_s()
{
    Element_Begin1("tool_t4_to_f_s");
    TESTELSE_SB_SKIP(                                           "b_top_front_to_front");
        Skip_S1(3,                                              "gain_t2a_code");
    TESTELSE_SB_ELSE(                                           "b_top_front_to_front");
        Skip_S1(3,                                              "gain_t2b_code");
    TESTELSE_SB_END();

    TESTELSE_SB_SKIP(                                           "b_top_back_to_front");
        Skip_S1(3,                                              "gain_t2d_code");
    TESTELSE_SB_ELSE(                                           "b_top_back_to_front");
        Skip_S1(3,                                              "gain_t2e_code");
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_t2_to_f_s_b()
{
    Element_Begin1("tool_t2_to_f_s_b");
    TESTELSE_SB_SKIP(                                           "b_top_to_front");
        Skip_S1(3,                                              "gain_t2a_code");
    TESTELSE_SB_ELSE(                                           "b_top_to_front");
        TESTELSE_SB_SKIP(                                       "b_top_to_side");
            Skip_S1(3,                                          "gain_t2b_code");
        TESTELSE_SB_ELSE(                                       "b_top_to_side");
            Skip_S1(3,                                          "gain_t2c_code");
        TESTELSE_SB_END();
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::tool_t2_to_f_s()
{
    Element_Begin1("tool_t2_to_f_s");
    TESTELSE_SB_SKIP(                                           "b_top_to_front");
        Skip_S1(3,                                              "gain_t2a_code");
    TESTELSE_SB_ELSE(                                           "b_top_to_front");
        Skip_S1(3,                                              "gain_t2b_code");
    TESTELSE_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::loud_corr(int8u pres_ch_mode, int8u pres_ch_mode_core, bool b_objects)
{
    bool b_obj_loud_corr=false, b_corr_for_immersive_out=false;

    Element_Begin1("loud_corr");
    if (b_objects)
        Get_SB(b_obj_loud_corr,                                 "b_obj_loud_corr");

    if ((pres_ch_mode!=(int8u)-1 && pres_ch_mode>4) || b_obj_loud_corr)
        Get_SB(b_corr_for_immersive_out,                        "b_corr_for_immersive_out");

    if ((pres_ch_mode!=(int8u)-1 && pres_ch_mode>1) || b_obj_loud_corr)
    {
        TEST_SB_SKIP(                                           "b_loro_loud_comp");
            Skip_S1(5,                                          "loro_dmx_loud_corr");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_ltrt_loud_comp");
            Skip_S1(5,                                          "ltrt_dmx_loud_corr");
        TEST_SB_END();
    }

    if ((pres_ch_mode!=(int8u)-1 && pres_ch_mode>4) || b_obj_loud_corr)
    {
        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_5_X");
        TEST_SB_END();

        if (b_corr_for_immersive_out)
        {
            TEST_SB_SKIP(                                       "b_loud_comp");
                Skip_S1(5,                                      "loud_corr_5_X_2");
            TEST_SB_END();

            TEST_SB_SKIP(                                       "b_loud_comp");
                Skip_S1(5,                                      "loud_corr_7_X");
            TEST_SB_END();
        }
    }

    if (((pres_ch_mode!=(int8u)-1 && pres_ch_mode>10) || b_obj_loud_corr) && b_corr_for_immersive_out)
    {
        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_7_X_4");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_7_X_2");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_5_X_4");
        TEST_SB_END();
    }

    if (pres_ch_mode!=(int8u)-1 && pres_ch_mode>4)
    {
        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_5_X_2");
        TEST_SB_END();
    }

    if (pres_ch_mode!=(int8u)-1 && pres_ch_mode>2)
    {
        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_5_X");
        TEST_SB_END();

        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_core_loro");
            Skip_S1(5,                                          "loud_corr_core_ltrt");
        TEST_SB_END();
    }

    if (b_obj_loud_corr)
    {
        TEST_SB_SKIP(                                           "b_loud_comp");
            Skip_S1(5,                                          "loud_corr_9_X_4");
        TEST_SB_END();
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_frame(drc_info& DrcInfo, bool b_iframe)
{
    Element_Begin1("drc_frame");
    TEST_SB_SKIP(                                           "b_drc_present");
        if (b_iframe)
            drc_config(DrcInfo);

        drc_data(DrcInfo);
    TEST_SB_END();
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_config(drc_info& DrcInfo)
{
    int8u drc_decoder_nr_modes;
    Element_Begin1("drc_config");
        Get_S1(3, drc_decoder_nr_modes,                         "drc_decoder_nr_modes");
        DrcInfo.Decoders.clear();
        for (int8u Pos=0; Pos<=drc_decoder_nr_modes; Pos++)
        {
            DrcInfo.Decoders.resize(DrcInfo.Decoders.size()+1);
            drc_decoder_mode_config(DrcInfo.Decoders.back());
        }

        //Manage drc_repeat_id
        for (int8u i=0; i<=drc_decoder_nr_modes; i++)
        {
            if (DrcInfo.Decoders[i].drc_repeat_id!=(int8u)-1)
            {
                for (int8u j=0; j<=drc_decoder_nr_modes; j++)
                    if (j!=i && DrcInfo.Decoders[j].drc_decoder_mode_id==DrcInfo.Decoders[i].drc_repeat_id)
                    {
                        int8u drc_decoder_mode_id=DrcInfo.Decoders[i].drc_decoder_mode_id;
                        DrcInfo.Decoders[i]=DrcInfo.Decoders[j];
                        DrcInfo.Decoders[i].drc_decoder_mode_id=drc_decoder_mode_id;
                        DrcInfo.Decoders[i].drc_compression_curve_flag=true;
                        break;
                    }
            }
        }

        Get_S1 (3, DrcInfo.drc_eac3_profile,                    "drc_eac3_profile");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_data(drc_info& DrcInfo)
{
    bool curve_present=false;
    int16u drc_gainset_size;
    int8u drc_version;
    size_t Remain_Before, used_bits=0;

    Element_Begin1("drc_data");
    for (int8u Pos=0; Pos<DrcInfo.Decoders.size(); Pos++)
    {
        if (!DrcInfo.Decoders[Pos].drc_compression_curve_flag)
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
                drc_gains(DrcInfo.Decoders[Pos]);
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
void File_Ac4::drc_gains(drc_decoder_config& D)
{
    Element_Begin1("drc_gains");
    Skip_S1(7,                                                  "drc_gain_val");
    if (D.drc_gains_config>0)
    {
        // TODO:
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ac4::drc_decoder_mode_config(drc_decoder_config& D)
{
    D.drc_compression_curve_flag=false;

    Element_Begin1("drc_decoder_mode_config");
    Get_S1(3, D.drc_decoder_mode_id,                            "drc_decoder_mode_id[pcount]");
    if (D.drc_decoder_mode_id>3)
    {
        Skip_S1(5,                                              "drc_output_level_from");
        Skip_S1(5,                                              "drc_output_level_to");
    }

    TESTELSE_SB_SKIP(                                           "drc_repeat_profile_flag");
        Get_S1 (3, D.drc_repeat_id,                             "drc_repeat_id");
        D.drc_compression_curve_flag=true;
    TESTELSE_SB_ELSE(                                           "drc_repeat_profile_flag");
        TESTELSE_SB_GET (D.drc_default_profile_flag,            "drc_default_profile_flag");
            D.drc_compression_curve_flag=true;
        TESTELSE_SB_ELSE(                                       "drc_default_profile_flag");
            TESTELSE_SB_GET(D.drc_compression_curve_flag,       "drc_compression_curve_flag[drc_decoder_mode_id[pcount]]");
                drc_compression_curve();
            TESTELSE_SB_ELSE(                                   "drc_compression_curve_flag[drc_decoder_mode_id[pcount]]");
                Get_S1 (2, D.drc_gains_config,                  "drc_gains_config[drc_decoder_mode_id[pcount]]");
            TESTELSE_SB_END();
        TESTELSE_SB_END();
    TESTELSE_SB_END();
    Element_End0();
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

    Get_S1(5, drc_gain_max_cut,                                 "drc_gain_max_cut");
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
void File_Ac4::further_loudness_info(loudness_info& L, bool sus_ver, bool b_presentation_ldn)
{
    bool b_loudcorr_type;
    Element_Begin1("further_loudness_info");
    if (b_presentation_ldn || !sus_ver)
    {
        int8u loudness_version;
        Get_S1 (2, loudness_version,                            "loudness_version");
        if (loudness_version==3)
            Skip_S1(4,                                          "extended_loudness_version");

        Get_S1(4, L.loud_prac_type,                             "loud_prac_type");
        if (L.loud_prac_type)
        {
            TEST_SB_SKIP(                                       "b_loudcorr_dialgate");
                Get_S1 (3, L.loud_dialgate_prac_type,           "dialgate_prac_type");
            TEST_SB_END();
            Get_SB (L.b_loudcorr_type,                          "b_loudcorr_type");
        }
    }
    else
    {
        Skip_SB(                                                "b_loudcorr_dialgate");
    }

    TEST_SB_SKIP(                                               "b_loudrelgat");
        Get_S2 (11, L.loudrelgat,                               "loudrelgat");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_loudspchgat");
        Get_S2 (11, L.loudspchgat,                              "loudspchgat");
        Get_S1 (3, L.loudspchgat_dialgate_prac_type,            "dialgate_prac_type");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_loudstrm3s");
        Skip_S2(11,                                             "loudstrm3s");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_max_loudstrm3s");
        Skip_S2(11,                                             "max_loudstrm3s");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_truepk");
        Get_S2 (11, L.truepk,                                   "truepk");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_max_truepk");
        Skip_S2(11,                                             "max_truepk");
    TEST_SB_END();

    if (b_presentation_ldn || !sus_ver)
    {
        TEST_SB_SKIP(                                           "b_prgmbndy");
            bool prgmbndy_bit=false;
            while(!prgmbndy_bit) // TODO: skip all bits in one time
                Get_SB(prgmbndy_bit,                            "prgmbndy_bit");

            Skip_SB(                                            "b_end_or_start");
            TEST_SB_SKIP(                                       "b_prgmbndy_offset");
                Skip_S2(11,                                     "prgmbndy_offset");
            TEST_SB_END();
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_lra");
        Get_S2 (10, L.lra,                                      "lra");
        Get_S1 ( 3, L.lra_prac_type,                            "lra_prac_type");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_loudmntry");
        Skip_S2(11,                                             "loudmntry");
    TEST_SB_END();

    TEST_SB_SKIP(                                               "b_max_loudmntry");
        Get_S2 (11, L.max_loudmntry,                            "max_loudmntry");
    TEST_SB_END();

    if (sus_ver)
    {
        TEST_SB_SKIP(                                           "b_rtllcomp");
            Skip_S1(8,                                          "rtllcomp");
        TEST_SB_END();
    }

    TEST_SB_SKIP(                                               "b_extension");
        int8u e_bits_size; // TODO: check if size is sufficient
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
void File_Ac4::Get_V4(int8u Bits, int32u& Info, const char* Name)
{
    Info=0;
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int8u Count=0;
            for (;;)
            {
                Info+=BS->Get4(Bits);
                Count+=1+Bits;
                if (!BS->GetB())
                    break;
                Info<<=Bits;
                Info+=(1<<Bits);
            }

            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            for (;;)
            {
                Info+=BS->Get4(Bits);
                if (!BS->GetB())
                    break;
                Info<<=Bits;
                Info+=(1<<Bits);
            }
        }
}

//---------------------------------------------------------------------------
void File_Ac4::Skip_V4(int8u  Bits, const char* Name)
{
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            int32u Info=0;
            int8u Count=0;
            for (;;)
            {
                Info+=BS->Get4(Bits);
                Count+=Bits;
                if (!BS->GetB())
                    break;
                Info<<=Bits;
                Info+=(1<<Bits);
            }

            Param(Name, Info, Count);
            Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
        }
        else
    #endif //MEDIAINFO_TRACE
        {
            for (;;)
            {
                BS->Skip(Bits);
                if (!BS->GetB())
                    break;
            }
        }
}

//---------------------------------------------------------------------------
void File_Ac4::Get_V4(int8u Bits1, int8u Bits2, int8u Flag_Value, int32u &Info, const char* Name)
{
    Info = 0;
    int8u Count=Bits1;

    Peek_S4(Count, Info);
    if (Info==Flag_Value)
    {
        Count=Bits2;
        Peek_S4(Count, Info);
    }
    BS->Skip(Count);

    #if MEDIAINFO_TRACE
    if (Trace_Activated)
    {
        Param(Name, Info, Count);
        Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
    }
    #endif
}

//---------------------------------------------------------------------------
void File_Ac4::Skip_V4(int8u Bits1, int8u Bits2, int8u Flag_Value, const char* Name)
{
    int32u Info = 0;
    int8u Count=Bits1;

    Peek_S4(Count, Info);
    if (Info==Flag_Value)
    {
        Count=Bits2;
        Peek_S4(Count, Info);
    }
    BS->Skip(Count);

    #if MEDIAINFO_TRACE
    if (Trace_Activated)
    {
        Param(Name, Info, Count);
        Param_Info(__T("(")+Ztring::ToZtring(Count)+__T(" bits)"));
    }
    #endif
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
void File_Ac4::Get_V4(const variable_size* Bits, int8u &Info, const char* Name)
{
    int8u TotalSize=Bits[0].AddedSize;
    Bits++;
    int8u BitSize=0;
    int16u Temp;
    for (int8u i=0; i<TotalSize; i++)
    {
        int8u AddedSize=Bits[i].AddedSize;
        if (AddedSize)
        {
            BitSize+=AddedSize;
            Peek_S2(BitSize, Temp);
        }
        if (Temp==Bits[i].Value)
        {
            Skip_S2(BitSize, Name); Param_Info1(i);
            Info=i;
            return;
        }
    }

    //Problem
    Skip_S2(BitSize, Name);
    Trusted_IsNot("Variable size");
    Info=(int8u)-1;
}

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
int8u File_Ac4::Superset(int8u Ch_Mode1, int8u Ch_Mode2)
{
    if (Ch_Mode1>15 && Ch_Mode2>15)
        return (int8u)-1;
    else if (Ch_Mode1>15)
        return Ch_Mode2;
    else if (Ch_Mode2>15)
        return Ch_Mode1;
        else if (Ch_Mode1==15 || Ch_Mode2==15)
        return 15;

    static int8u Modes[16][3]=
    {
        {1,0,0},
        {2,0,0},
        {3,0,0},
        {5,0,0},
        {5,1,0},
        {7,0,0},
        {7,1,0},
        {7,0,1},
        {7,1,1},
        {7,0,2},
        {7,1,2},
        {7,0,4},
        {7,1,4},
        {9,0,4},
        {9,1,4},
        {22,2,0}
    };

    for (int8u Pos=0; Pos<15; Pos++)
    {
        if (Modes[Ch_Mode1][0]<=Modes[Pos][0] && Modes[Ch_Mode1][1]<=Modes[Pos][1] && Modes[Ch_Mode1][2]<=Modes[Pos][2] &&
            Modes[Ch_Mode2][0]<=Modes[Pos][0] && Modes[Ch_Mode2][1]<=Modes[Pos][1] && Modes[Ch_Mode2][2]<=Modes[Pos][2])
            return Pos;
    }

    return (int8u)-1;
}

//---------------------------------------------------------------------------
int16u File_Ac4::Huffman_Decode(const ac4_huffman& Table, const char* Name)
{
    int8u bit;
    int16s index = 0;

    Element_Begin1(Name);
    while (index>=0)
    {
        Get_S1(1, bit,                                          "bit");
        index=Table[index][bit];
    }
    Element_End0();

    return index+64;
}

} //NameSpace

#endif //MEDIAINFO_AC4_YES
