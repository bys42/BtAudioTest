/** @file
    L2Cap defines.

 **/

#ifndef _L2CAP_DEF_H_
#define _L2CAP_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "SbcPlayerConfig.h" // take this out if real MAX_ACL_BUFFER is got

#define ACL_HANDLE(BcFlag, PbFlag, Handle) ((BcFlag << 14) | (PbFlag << 12) | Handle)

#define BOUNDARY_NONAUTO_FLUSHABLE_HOST_TO_CONTROL 0x00
#define BOUNDARY_CONTINUING_FRAGMENT               0x01
#define BOUNDARY_AUTO_FLUSHABLE                    0x02

#define BROADCAST_NO 0x00

#pragma pack(1)
typedef struct {
    union {
        UINT16 Handle;
        struct {
            UINT16 RawHandle : 12;
            UINT16 PB : 2;
            UINT16 BC : 2;
        };
    };
    UINT16 DataLength;
} HCI_ACL_DATA_HEADER;

typedef struct {
    UINT16 PayloadLength;
    UINT16 Cid;
} L2CAP_B_FRAME_HEADER;

#define SIGNAL_CHANNEL_ID 0x0001
typedef struct {
    UINT8 Code;
    UINT8 Identifier;
    UINT16 Length;
} L2CAP_SIGNAL_HEADER;

#define L2CAP_CONNECTION_REQUEST_CODE 0x02
typedef struct {
    UINT16 Psm;
    UINT16 SCid;
} L2CAP_CONNECTION_REQUEST;

#define L2CAP_CONNECTION_RESPONSE_CODE 0x03
typedef struct {
    UINT16 DCid;
    UINT16 SCid;
    UINT16 Result;
    UINT16 Status;
} L2CAP_CONNECTION_RESPONSE;

#define L2CAP_CONFIGURATION_REQUEST_CODE 0x04
typedef struct {
    UINT16 DCid;
    UINT16 Flag;
} L2CAP_CONFIGURATION_REQUEST;

#define L2CAP_CONFIGURATION_RESPONSE_CODE 0x05
typedef struct {
    UINT16 SCid;
    UINT16 Flag;
    UINT16 Result;
} L2CAP_CONFIGURATION_RESPONSE;

#define L2CAP_DISCONNECTION_REQUEST_CODE 0x06
typedef struct {
    UINT16 DCid;
    UINT16 SCid;
} L2CAP_DISCONNECTION_REQUEST;

#define L2CAP_DISCONNECTION_RESPONSE_CODE 0x07
typedef struct {
    UINT16 DCid;
    UINT16 SCid;
} L2CAP_DISCONNECTION_RESPONSE;

#define L2CAP_INFORMATION_REQUEST_CODE 0x0A
typedef struct {
    UINT16 Type;
} L2CAP_INFORMATION_REQUEST;

#define L2CAP_INFORMATION_RESPONSE_CODE 0x0B
typedef struct {
    UINT16 Type;
    UINT16 Result;
} L2CAP_INFORMATION_RESPONSE;

typedef struct {
    HCI_ACL_DATA_HEADER Acl;
    L2CAP_B_FRAME_HEADER BFrame;
    L2CAP_SIGNAL_HEADER Signal;
    union {
        L2CAP_CONNECTION_REQUEST ConnectReq;
        L2CAP_CONNECTION_RESPONSE ConnectRes;
        L2CAP_CONFIGURATION_REQUEST ConfigReq;
        L2CAP_CONFIGURATION_RESPONSE ConfigRes;
        L2CAP_DISCONNECTION_REQUEST DisconReq;
        L2CAP_DISCONNECTION_RESPONSE DisconRes;
        L2CAP_INFORMATION_REQUEST InfoReq;
        L2CAP_INFORMATION_RESPONSE InfoRes;
        UINT8 SignalData[MAX_ACL_BUFFER - sizeof(HCI_ACL_DATA_HEADER) - sizeof(L2CAP_B_FRAME_HEADER) - sizeof(L2CAP_SIGNAL_HEADER)];
    };
} L2CAP_SIGNAL_CHANNEL;
#pragma pack()

/****** DO NOT WRITE BELOW THIS LINE *******/
#ifdef __cplusplus
}
#endif

#endif
