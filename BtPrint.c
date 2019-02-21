/** @file BtPrint.c
    Print BT Data source.

 **/

#include <Library/UefiLib.h>
#include "AvdtpDef.h"
#include "BtBaseDef.h"

/**
   PrintData.

   @param  DataStart
   @param  DataSize
 **/
VOID
PrintData (
    UINT8 *DataStart,
    UINTN DataSize
    )
{
    for (DataSize; DataSize != 0; DataSize--) {
        Print(L" %02x", *DataStart);
        DataStart++;
    }
    return;
}

/**
   PrintHciCmdStatus.

   @param  HciCmd
   @param  Status
 **/
VOID
PrintHciCmdStatus(
    VOID *Buffer,
    EFI_STATUS Status
    )
{
    HCI_COMMAND_PACKET *HciCmd;
    if (NO_BTTEST_DBG) {
        return;
    }
    HciCmd = (HCI_COMMAND_PACKET*)Buffer;
    Print(L"SendHciCommand:%04x, Status: %r\n", HciCmd->OpCode, Status);
    return;
}

/**
   PrintHciEvent.

   @param  HciEvent
 **/
VOID
PrintHciEvent(VOID *Buffer)
{
    HCI_EVENT_PACKET *HciEvent;
    if (NO_BTTEST_DBG) {
        return;
    }
    HciEvent = (HCI_EVENT_PACKET*)Buffer;
    Print(L"EventCode: %02x, ParaLength: %02d\n", HciEvent->EventCode, HciEvent->ParameterTotalLength);
    Print(L"Parameter:");
    PrintData ((UINT8*)(&HciEvent->Parameter[0]), HciEvent->ParameterTotalLength);
    Print(L"\n");
    return;
}

/**
   PrintAclData.

   @param  Buffer
   @param  Direction
 **/
VOID
PrintAclData (
    IN VOID *Buffer,
    IN UINT8 Direction
    )
{
    HCI_ACL_DATA_HEADER  *AclHeader;
    L2CAP_B_FRAME_HEADER *BFrame;
    if (NO_BTTEST_DBG) {
        return;
    }

    if (Direction == SEND_ACL) {
        Print(L"\nSendACLData:\n");
    } else if (Direction == READ_ACL) {
        Print(L"\nReadACLData:\n");
    } else {
        Print(L"\nInvalid Direction\n");
        return;
    }

    AclHeader = (HCI_ACL_DATA_HEADER*)Buffer;
    Print(L"Handle %04x, DataLength %02d, ", AclHeader->Handle, AclHeader->DataLength);

    if ((AclHeader->PB) == 1) {
        Print(L"Raw Data: ");
        PrintData ((UINT8*)(AclHeader + 1), AclHeader->DataLength);
        Print(L"\n");
        return;
    }

    BFrame = (L2CAP_B_FRAME_HEADER*)(AclHeader + 1);
    Print(L"PayloadLength %02d, Cid %04x - ", BFrame->PayloadLength, BFrame->Cid);

    switch (BFrame->Cid) {
    case SIGNAL_CHANNEL_ID:
        L2CAP_SIGNAL_HEADER *Signal = (L2CAP_SIGNAL_HEADER*)(BFrame + 1);
        Print(L"SIGNAL_CHANNEL_ID\n");
        Print(L"Code %02x, Id %02x, Length %02d - ", Signal->Code, Signal->Identifier, Signal->Length);

        switch (Signal->Code) {
        case L2CAP_CONNECTION_RESPONSE_CODE:
            L2CAP_CONNECTION_RESPONSE *ConnectRes = (L2CAP_CONNECTION_RESPONSE*)(Signal + 1);
            Print(L"L2CAP_CONNECTION_RESPONSE\n");
            Print(L"DCid %04x, SCid %04x, Result %04x, Status %04x\n", ConnectRes->DCid, ConnectRes->SCid, ConnectRes->Result, ConnectRes->Status);
            break;

        case L2CAP_CONFIGURATION_REQUEST_CODE:
            L2CAP_CONFIGURATION_REQUEST *ConfigReq = (L2CAP_CONFIGURATION_REQUEST*)(Signal + 1);
            Print(L"L2CAP_CONFIGURATION_REQUEST\n");
            Print(L"DCid %04x, Flag %04x\n", ConfigReq->DCid, ConfigReq->Flag);
            if (Signal->Length > sizeof(L2CAP_CONFIGURATION_REQUEST)) {
                Print(L"Config:");
                PrintData ((UINT8*)(ConfigReq + 1), Signal->Length - sizeof(L2CAP_CONFIGURATION_REQUEST));
                Print(L"\n");
            }
            break;

        case L2CAP_CONFIGURATION_RESPONSE_CODE:
            L2CAP_CONFIGURATION_RESPONSE *ConfigRes = (L2CAP_CONFIGURATION_RESPONSE*)(Signal + 1);
            Print(L"L2CAP_CONFIGURATION_RESPONSE\n");
            Print(L"SCid %04x, Flag %04x, Result %04x\n", ConfigRes->SCid, ConfigRes->Flag, ConfigRes->Result);
            if (Signal->Length > sizeof(L2CAP_CONFIGURATION_RESPONSE)) {
                Print(L"Config:");
                PrintData ((UINT8*)(ConfigRes + 1), Signal->Length - sizeof(L2CAP_CONFIGURATION_RESPONSE));
                Print(L"\n");
            }
            break;

        default:
            Print(L"SIGNAL_CHANNEL_DATA\n");
            Print(L"Data:");
            PrintData ((UINT8*)(Signal + 1), Signal->Length);
            Print(L"\n");
            break;
        }
        break;

    case DEFAULT_AVDTP_SIGNAL_CHANNEL_ID:
        AVDTP_SIGNAL_PACKET *Avdtp = (AVDTP_SIGNAL_PACKET*)Buffer;
        UINT8               SignalId = 0;
        UINT8               PacketCount = 0;
        UINT8               *DataRemain;
        UINT16              DataSize;

        Print(L"DEFAULT_AVDTP_SIGNAL_CHANNEL_ID\n");
        Print(L"MessageType %02x ", Avdtp->MessageType);
        switch (Avdtp->MessageType) {
        case AVDTP_MSG_TYPE_COMMAND:
            Print(L"(COMMAND)");
            break;
        case AVDTP_MSG_TYPE_GEN_REJECT:
            Print(L"(GEN_REJECT)");
            break;
        case AVDTP_MSG_TYPE_RES_ACCEPT:
            Print(L"(RES_ACCEPT)");
            break;
        case AVDTP_MSG_TYPE_RES_REJECT:
            Print(L"(RES_REJECT)");
            break;
        default:
            break;
        }
        Print(L", PacketType %02x ", Avdtp->PacketType);
        switch (Avdtp->PacketType) {
        case AVDTP_PKT_TYPE_SINGLE:
            Print(L"(AVDTP_PKT_TYPE_SINGLE)");
            SignalId = Avdtp->Single.SignalId;
            DataRemain = &Avdtp->Single.SignalData.RawData[0];
            DataSize = BFrame->PayloadLength - 2;
            break;
        case AVDTP_PKT_TYPE_START:
            SignalId = Avdtp->Start.SignalId;
            PacketCount = Avdtp->Start.PacketCount;
            DataRemain = &Avdtp->Start.Data[0];
            DataSize = BFrame->PayloadLength - 3;
            Print(L"(AVDTP_PKT_TYPE_START)");
            break;
        case AVDTP_PKT_TYPE_CONTINUE:
            DataRemain = &Avdtp->ContinueOrEnd.Data[0];
            DataSize = BFrame->PayloadLength - 1;
            Print(L"(AVDTP_PKT_TYPE_CONTINUE)");
            break;
        case AVDTP_PKT_TYPE_END:
            DataRemain = &Avdtp->ContinueOrEnd.Data[0];
            DataSize = BFrame->PayloadLength - 1;
            Print(L"(AVDTP_PKT_TYPE_END)");
            break;
        default:
            break;
        }
        Print(L", Transaction %02x ", Avdtp->Transaction);
        if (SignalId != 0) {
            Print(L", SignalId %02x ", SignalId);
            switch (SignalId) {
            case AVDTP_DISCOVER:
                Print(L"(AVDTP_DISCOVER)");
                break;
            case AVDTP_GET_CAPABILITIES:
                Print(L"(AVDTP_GET_CAPABILITIES)");
                break;
            case AVDTP_SET_CONFIGURATION:
                Print(L"(AVDTP_SET_CONFIGURATION)");
                break;
            case AVDTP_GET_CONFIGURATION:
                Print(L"(AVDTP_GET_CONFIGURATION)");
                break;
            case AVDTP_RECONFIGURE:
                Print(L"(AVDTP_RECONFIGURE)");
                break;
            case AVDTP_OPEN:
                Print(L"(AVDTP_OPEN)");
                break;
            case AVDTP_START:
                Print(L"(AVDTP_START)");
                break;
            case AVDTP_CLOSE:
                Print(L"(AVDTP_CLOSE)");
                break;
            case AVDTP_SUSPEND:
                Print(L"(AVDTP_SUSPEND)");
                break;
            case AVDTP_ABORT:
                Print(L"(AVDTP_ABORT)");
                break;
            case AVDTP_SECURITY_CONTROL:
                Print(L"(AVDTP_SECURITY_CONTROL)");
                break;
            case AVDTP_GET_ALL_CAPABILITIES:
                Print(L"(AVDTP_GET_ALL_CAPABILITIES)");
                break;
            case AVDTP_DELAYREPORT:
                Print(L"(AVDTP_DELAYREPORT)");
                break;
            default:
                Print(L"(AVDTP_RESERVE)");
                break;
            }
        }
        if (PacketCount != 0) {
            Print(L", PacketCount: %02d", PacketCount);
        }
        Print(L"\nData:");
        PrintData (DataRemain, DataSize);
        Print(L"\n");
        break;
    case DEFAULT_AVDTP_STREAM_CHANNEL_ID:
        Print(L"DEFAULT_AVDTP_STREAM_CHANNEL_ID\n");
        break;
    default:
        Print(L"UNCATE_CHANNEL_ID\n");
        break;
    }
    Print(L"Raw Data: ");
    PrintData ((UINT8*)(BFrame + 1), BFrame->PayloadLength);
    Print(L"\n");
    return;
}

/**
   PrintSbcFrameHeader.

   @param  FrameHeader

 **/
VOID
PrintSbcFrameHeader (VOID *Buffer)
{
    SBC_FRAME_HEADER *FrameHeader;
    if (NO_BTTEST_DBG) {
        return;
    }

    FrameHeader = (SBC_FRAME_HEADER*) Buffer;

    Print(L"SyncWord: %x\n", FrameHeader->SyncWord);
    Print(L"Subbands: %d (%x in raw)\n", (FrameHeader->Setting.Subbands + 1) * 4, FrameHeader->Setting.Subbands);
    Print(L"Blocks: %d (%x in raw)\n", (FrameHeader->Setting.Blocks + 1) * 4, FrameHeader->Setting.Blocks);
    Print(L"AllocationMethod: ");
    if (FrameHeader->Setting.AllocationMethod == 0) {
        Print (L"Loudness");
    } else {
        Print (L"SNR");
    }
    Print (L" (%x in raw)\n", FrameHeader->Setting.AllocationMethod);

    Print(L"ChannelMode: ");
    if (FrameHeader->Setting.ChannelMode == MONO) {
        Print (L"Mono");
    } else if (FrameHeader->Setting.ChannelMode == DUAL_CHANNEL) {
        Print (L"Dual Channel");
    } else if (FrameHeader->Setting.ChannelMode == STEREO) {
        Print (L"Stereo");
    } else {
        Print (L"Joint Stereo");
    }
    Print (L" (%x in raw)\n", FrameHeader->Setting.ChannelMode);

    Print(L"SamplingFrequency: ");
    if (FrameHeader->Setting.SamplingFrequency == 0) {
        Print (L"16kHZ");
    } else if (FrameHeader->Setting.SamplingFrequency == 1) {
        Print (L"32kHZ");
    } else if (FrameHeader->Setting.SamplingFrequency == 2) {
        Print (L"44.1kHZ");
    } else {
        Print (L"48kHZ");
    }
    Print (L" (%x in raw)\n", FrameHeader->Setting.SamplingFrequency);

    Print(L"Bitpool: %d\n", FrameHeader->Setting.Bitpool);
    return;
}
