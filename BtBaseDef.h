/** @file
    BT base defines, including BT USB, HCI CMD, HCI EVT.

 **/

#ifndef _BT_BASE_DEF_H_
#define _BT_BASE_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "SbcPlayerConfig.h" // take this out if real MAX_HCI_CMD_EVT_BUFFER is got

#define SEND_ACL (0)
#define READ_ACL (1)

#define USB_CLASS_WIRELESS_CONTROLLER  0xE0
#define USB_SUBCLASS_RF_CONTROLLER     0x01
#define USB_PROTOCOL_BT_PROG_INTERFACE 0x01
#define BT_USB_ENDPOINT_NUMBER         3
#define DIRECTION_IN 1
#define DIRECTION_OUT 0

typedef enum _USB_ENDPOINT_TYPE {
    UsbEndpointControl = 0x00,
    UsbEndpointIsochronous = 0x01,
    UsbEndpointBulk = 0x02,
    UsbEndpointInterrupt = 0x03,
    UsbEndpointTypeMax
} USB_ENDPOINT_TYPE;

typedef enum _USB_ENDPOINT_DIRECTION {
    UsbOut = 0x00,
    UsbIn = 0x01,
    UsbDirectionMax
} USB_ENDPOINT_DIRECTION;

#define BT_OPCODE(Ogf, Ocf) ((Ogf << 10) | Ocf)

#define ACL_PACKET_TYPE_2DH1 BIT1
#define ACL_PACKET_TYPE_3DH1 BIT2
#define ACL_PACKET_TYPE_DM1  BIT3
#define ACL_PACKET_TYPE_DH1  BIT4
#define ACL_PACKET_TYPE_2DH3 BIT8
#define ACL_PACKET_TYPE_3DH3 BIT9
#define ACL_PACKET_TYPE_DM3  BIT10
#define ACL_PACKET_TYPE_DH3  BIT11
#define ACL_PACKET_TYPE_2DH5 BIT12
#define ACL_PACKET_TYPE_3DH5 BIT13
#define ACL_PACKET_TYPE_DM5  BIT14
#define ACL_PACKET_TYPE_DH5  BIT15

#define ACL_PACKET_TYPE_DEFAULT  (ACL_PACKET_TYPE_DH5 | ACL_PACKET_TYPE_DM5 | ACL_PACKET_TYPE_DH3 | ACL_PACKET_TYPE_DM3 | ACL_PACKET_TYPE_DH1 | ACL_PACKET_TYPE_DM1)

#define INQUIRY_COMPLETE 0x01
#define INQUIRY_RESULT   0x02
#define CONNECTION_COMPLETE  0x03
#define DISCONNECTION_COMPLETE  0x05
#define AUTHENTICATION_COMPLETE   0x06
#define HCI_REMOTE_NAME_REQUEST_COMPLETE 0x07
#define ENCTYPTION_CHANGE 0x08
#define READ_REMOTE_SUPPORTED_FEATURES_COMPLETE 0x0B
#define COMMAND_COMPLETE 0x0E
#define COMMAND_STATUS   0x0F
#define NUMBER_OF_COMPLETED_PACKETS 0x13
#define LINK_KEY_REQUEST 0x17
#define LINK_KEY_NOTIFICATION 0x18
#define READ_CLOCK_OFFSET_COMPLETE 0x1C
#define READ_REMOTE_EXTENDED_SUPPORTED_FEATURES_COMPLETE 0x23
#define INQUIRY_RESULT_WITH_RSSI  0x22
#define EXTENDED_INQUIRY_RESULT   0x2F
#define IO_CAPABILITY_REQUEST 0x31
#define IO_CAPABILITY_RESPONSE 0x32
#define USER_CONFIRMATION_REQUEST 0x33

#define LINK_CONTROL 0x01
#define HCI_INQUIRY                     0x0001
#define HCI_INQUIRY_CANCEL              0x0002
#define HCI_CREATE_CONNECTION           0x0005
#define HCI_DISCONNECT                  0x0006
#define LINK_KEY_REQUEST_NEGATIVE_REPLY 0x000C
#define AUTHENTICATION_REQUESTED        0x0011
#define SET_CONNECTION_ENCRYPTION       0x0013
#define HCI_REMOTE_NAME_REQUEST         0x0019
#define IO_CAPABILITY_REQUEST_REPLY     0x002B
#define USER_CONFIRMATION_REQUEST_REPLY 0x002C

#define CONTROLLER_AND_BASEBAND 0x03
#define HCI_SET_EVENT_MASK            0x0001
#define HCI_RESET                     0x0003
#define HCI_WRITE_INQUIRY_MODE        0x0045
#define HCI_WRITE_SIMPLE_PAIRING_MODE 0x0056

#pragma pack(1)
typedef struct {
    UINT8 Status;
    UINT16 Handle;
    UINT64 BdAddr : 48;
    UINT64 LinkType : 8;
    UINT64 EncrypEnabled : 8;
} CONNECTION_COMPLETE_DATA;

typedef struct {
    union {
        UINT64 BdAddr : 48;
        struct {
            UINT8 BdAddrPtr[6];
            UINT8 PageScanRepMode;
            UINT8 Reserved;
        };
    };
    UINT16 ClockOffset;
} REMOTE_DEVICE_LIST;

typedef struct {
    union {
        UINT64 BdAddr : 48;
        struct {
            UINT8 BdAddrPtr[6];
            UINT8 PageScanRepMode;
            UINT8 Reserved;
        };
    };
    UINT8 Reserved2 : 8;
    UINT8 DeviceClass[3];
    UINT16 ClockOffset;
} INQUIRY_RESULT_DATA;

typedef struct {
    UINT8 Status;
    union {
        UINT64 BdAddr : 48;
        struct {
            UINT8 BdAddrPtr[6];
            CHAR8 Name[248];
        };
    };
} REMOTE_NAME_REQUEST_COMPLETE_DATA;

typedef struct {
    UINT8 EventCode;
    UINT8 ParameterTotalLength;
    union {
        UINT8 Status;

        struct {
            UINT8 RespNum;
            INQUIRY_RESULT_DATA Result[1];
        } Inquiry;
        CONNECTION_COMPLETE_DATA ConnectionComplete;
        REMOTE_NAME_REQUEST_COMPLETE_DATA RemoteName;
        UINT8 Parameter[MAX_HCI_CMD_EVT_BUFFER - sizeof(UINT8) - sizeof(UINT8)];
    };
} HCI_EVENT_PACKET;

typedef struct {
    UINT32 Lap : 24;
    UINT32 InquiryLength : 8;
    UINT8 NumResponses;
} HCI_INQUIRY_DATA;

typedef struct {
    union {
        UINT64 BdAddr : 48;
        struct {
            UINT8 BdAddrPtr[6];
            UINT8 PageScanRepMode;
            UINT8 Reserved;
        };
    };
    UINT16 ClockOffset;
} HCI_REMOTE_NAME_REQUEST_DATA;

typedef struct {
    UINT64 BdAddr : 48;
    UINT64 PacketType : 16;
    UINT8 PageScanRepMode;
    UINT8 Reserved;
    UINT16 ClockOffset;
    UINT8 RoleSwith;
} HCI_CREATE_CONNECTION_DATA;

typedef struct {
    UINT16 Handle;
} AUTHENTICATION_REQUESTED_DATA;

typedef struct {
    UINT8 BdAddrPtr[6];
} LINK_KEY_REQUEST_NEGATIVE_REPLY_DATA;

typedef struct {
    union {
        UINT64 BdAddr : 48;
        struct {
            UINT8 BdAddrPtr[6];
            UINT8 IoCapability;
            UINT8 OobData;
        };

    };
    UINT8 AuthRequirement;
} IO_CAPABILITY_REQUEST_REPLY_DATA;

typedef struct {
    UINT8 BdAddrPtr[6];
} USER_CONFIRMATION_REQUEST_REPLY_DATA;

typedef struct {
    UINT16 Handle;
    UINT8 EncrypEnabled;
} SET_CONNECTION_ENCRYPTION_DATA;

typedef struct {
    UINT16 Handle;
    UINT8 Reason;
} HCI_DISCONNECT_DATA;

#define HCI_COMMAND_HEADER_SIZE 3
typedef struct {
    UINT16 OpCode;
    UINT8 ParameterTotalLength;
    union {
        HCI_INQUIRY_DATA Inquiry;
        HCI_REMOTE_NAME_REQUEST_DATA RemoteNameReq;
        HCI_CREATE_CONNECTION_DATA CreateConnection;
        AUTHENTICATION_REQUESTED_DATA AuthRequested;
        LINK_KEY_REQUEST_NEGATIVE_REPLY_DATA LinkKeyNegative;
        IO_CAPABILITY_REQUEST_REPLY_DATA IoCapReqReply;
        USER_CONFIRMATION_REQUEST_REPLY_DATA UserConfirmReqReply;
        SET_CONNECTION_ENCRYPTION_DATA SetConnectionEncpyt;
        HCI_DISCONNECT_DATA Disconnect;
        UINT8 Parameter[MAX_HCI_CMD_EVT_BUFFER];
    };
} HCI_COMMAND_PACKET;

#pragma pack()

/****** DO NOT WRITE BELOW THIS LINE *******/
#ifdef __cplusplus
}
#endif

#endif
