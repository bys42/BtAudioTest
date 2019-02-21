/** @file
    AVDTP defines.

 **/

#ifndef _AVDTP_DEF_H_
#define _AVDTP_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "L2CapDef.h"
#include "SbcPlayerConfig.h" // take this out if real MAX_ACL_BUFFER is got

#define AVDTP_PSM                 0x0019

#define AVDTP_PKT_TYPE_SINGLE       0x00
#define AVDTP_PKT_TYPE_START        0x01
#define AVDTP_PKT_TYPE_CONTINUE     0x02
#define AVDTP_PKT_TYPE_END          0x03

#define AVDTP_MSG_TYPE_COMMAND      0x00
#define AVDTP_MSG_TYPE_GEN_REJECT   0x01
#define AVDTP_MSG_TYPE_RES_ACCEPT   0x02
#define AVDTP_MSG_TYPE_RES_REJECT   0x03

#define AVDTP_RESERVE   0x00
#define AVDTP_DISCOVER  0x01
#define AVDTP_GET_CAPABILITIES  0x02
#define AVDTP_SET_CONFIGURATION 0x03
#define AVDTP_GET_CONFIGURATION 0x04
#define AVDTP_RECONFIGURE   0x05
#define AVDTP_OPEN  0x06
#define AVDTP_START 0x07
#define AVDTP_CLOSE 0x08
#define AVDTP_SUSPEND   0x09
#define AVDTP_ABORT 0x0A
#define AVDTP_SECURITY_CONTROL  0x0B
#define AVDTP_GET_ALL_CAPABILITIES  0x0C
#define AVDTP_DELAYREPORT   0x0D

#define AVDTP_DISCOVER_LENGTH 2
#define AVDTP_GET_CAPABILITIES_LENGTH 3
#define AVDTP_SET_CONFIGURATION_LENGTH 14
#define AVDTP_RECONFIGURE_LENGTH 11
#define AVDTP_OPEN_LENGTH 3
#define AVDTP_START_LENGTH 3
#define AVDTP_SUSPEND_LENGTH 3
#define AVDTP_CLOSE_LENGTH 3
#define AVDTP_STREAM_CONTROL_LENGTH 3

#pragma pack(1)
typedef struct {
    UINT8 ChannelMode : 4;
    UINT8 SamplingFrequency : 4;
    UINT8 AllocationMethod : 2;
    UINT8 Subbands : 2;
    UINT8 BlockLength : 4;
    UINT8 BitpoolMin;
    UINT8 BitpoolMax;
} AVDTP_SBC_CONFIG;

typedef struct {
    UINT8 AcpSeid;
    UINT8 MediaType;
} ACP_SEID_INFO;

typedef union {
    struct {
        UINT8 AcpSeid;
        UINT8 IntSeid;
        UINT8 MediaTrans;
        UINT8 MediaTransInfoLength;
        UINT8 MediaCodec;
        UINT8 MediaCodecInfoLength;
        UINT16 CodecId;
        AVDTP_SBC_CONFIG SbcConfig;
    } SetConfig;
    struct {
        UINT8 AcpSeid;
        UINT8 MediaCodec;
        UINT8 MediaCodecInfoLength;
        UINT16 CodecId;
        AVDTP_SBC_CONFIG SbcConfig;
    } Reconfig;
    ACP_SEID_INFO DiscoverResp[1];
    UINT8 AcpSeid;
    UINT8 RawData[0];
} AVDTP_SIGNAL_DATA;

#define AVDTP_TRANS_MASK (BIT4 - 1)

typedef struct {
    HCI_ACL_DATA_HEADER Acl;
    L2CAP_B_FRAME_HEADER BFrame;
    UINT8 MessageType : 2;
    UINT8 PacketType : 2;
    UINT8 Transaction : 4;
    union {
        struct {
            UINT8 SignalId;
            AVDTP_SIGNAL_DATA SignalData;
        } Single;
        struct {
            UINT8 PacketCount;
            UINT8 SignalId;
            UINT8 Data[0];
        } Start;
        struct {
            UINT8 Data[0];
        } ContinueOrEnd;
        UINT8 RawData[MAX_ACL_BUFFER - sizeof(HCI_ACL_DATA_HEADER) - sizeof(L2CAP_B_FRAME_HEADER) - sizeof(UINT8)];
    };
} AVDTP_SIGNAL_PACKET; // Single Signal Only

#define SBC_PACKET_HEADER_SIZE (21)
typedef struct {
    HCI_ACL_DATA_HEADER Acl;
    L2CAP_B_FRAME_HEADER BFrame;
    UINT16 Type;
    UINT16 Seqns;
    UINT32 TimeStamp;
    UINT32 Ssrc;
    UINT8 SbcHeader;
} SBC_PACKET_HEADER;

#define SBC_SYNCWORD (0x9C)
#define SAMPLING_FREQUENCY_44100 (0x02)
#define MONO (0x00)
#define DUAL_CHANNEL (0x01)
#define STEREO (0x02)
#define JOINT_STEREO (0x03)

typedef union {
    struct {
        UINT8 Subbands : 1;
        UINT8 AllocationMethod : 1;
        UINT8 ChannelMode : 2;
        UINT8 Blocks : 2;
        UINT8 SamplingFrequency : 2;
        UINT8 Bitpool;
    };
    UINT16 Raw;
} SBC_SETTING;

typedef struct {
    UINT8 SyncWord;
    SBC_SETTING Setting;
    UINT8 Crc;
} SBC_FRAME_HEADER;
#pragma pack()

/****** DO NOT WRITE BELOW THIS LINE *******/
#ifdef __cplusplus
}
#endif

#endif
