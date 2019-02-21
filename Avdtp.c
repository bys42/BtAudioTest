/** @file
    AVDTP source.

 **/

#include "AvdtpDef.h"
#include "SbcPlayer.h"

//
// Global Vars
//
UINT16      gAvdtpSigCid = 0;
UINT16      gAvdtpStrCid = 0;
UINT8       gAvdtpTrans = 0;
UINT8       gAcpSeid = 0;
SBC_SETTING gSbcSetting = {.Raw = DEFAULT_SBC_SETTING};

/**
   SendAvdtpCommnad

   @param  SignalId       AVDTP Signal Id
   @param  PayloadLength  AvdtpSignal Pack PayloadLength
   @param  AvdtpSignal    Pointer to AvdtpSignal Pack

   @retval EFI_STATUS  SendAvdtpCommnad result
 **/
EFI_STATUS
SendAvdtpCommnad(
    IN UINT8 SignalId,
    IN UINT16 PayloadLength,
    IN OUT AVDTP_SIGNAL_PACKET *AvdtpSignal
    )
{
    EFI_STATUS Status;
    UINTN      BufferSize;
    UINT8      TryCount;

    AvdtpSignal->MessageType = AVDTP_MSG_TYPE_COMMAND;
    AvdtpSignal->PacketType = AVDTP_PKT_TYPE_SINGLE;
    AvdtpSignal->Transaction = gAvdtpTrans;
    AvdtpSignal->Single.SignalId = SignalId;

    AvdtpSignal->BFrame.PayloadLength = PayloadLength;
    AvdtpSignal->BFrame.Cid = gAvdtpSigCid;

    Status = SendNonAutoFlushAcl(AvdtpSignal);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    TryCount = READ_TRIAL_COUNT;
    while (TryCount--) {
        BufferSize = sizeof(AVDTP_SIGNAL_PACKET);
        Status = ReadACLData(AvdtpSignal, &BufferSize);
        if (Status == EFI_SUCCESS) {
            if (AvdtpSignal->BFrame.Cid != DEFAULT_AVDTP_SIGNAL_CHANNEL_ID) {
                continue;
            }
            if (AvdtpSignal->Transaction != gAvdtpTrans ||
                AvdtpSignal->MessageType != AVDTP_MSG_TYPE_RES_ACCEPT) {
                return EFI_DEVICE_ERROR;
            }
            gAvdtpTrans++;
            gAvdtpTrans &= AVDTP_TRANS_MASK;
            return EFI_SUCCESS;
        }
        if (Status != EFI_TIMEOUT) {
            return Status;
        }
    }
    return EFI_TIMEOUT;
}

/**
   StreamChannelControl

   @param  Operation   StreamChannelControl Operation

   @retval EFI_STATUS  StreamChannelControl result
 **/
EFI_STATUS
StreamChannelControl (UINT8 Operation)
{
    EFI_STATUS          Status;
    AVDTP_SIGNAL_PACKET AvdtpSignal;
    AVDTP_SIGNAL_DATA   *SignalData = &(AvdtpSignal.Single.SignalData);

    if (gAcpSeid == 0) {
        return EFI_NOT_READY;
    }

    if (Operation == AVDTP_OPEN && gAvdtpStrCid != 0) {
        return EFI_ALREADY_STARTED;
    } else if (Operation != AVDTP_OPEN && gAvdtpStrCid == 0) {
        return EFI_NOT_STARTED;
    }

    SignalData->AcpSeid = gAcpSeid;
    Status = SendAvdtpCommnad (Operation, AVDTP_STREAM_CONTROL_LENGTH, &AvdtpSignal);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    if (Operation == AVDTP_OPEN) {
        Status = ConnectChannel (AVDTP_PSM, DEFAULT_AVDTP_STREAM_CHANNEL_ID, &gAvdtpStrCid);
    } else if (Operation == AVDTP_CLOSE) {
        Status = DisconnectChannel (gAvdtpStrCid, DEFAULT_AVDTP_STREAM_CHANNEL_ID);
        gAvdtpStrCid = 0;
    }

    return Status;
}

/**
   StreamChannelConfig

   @param  SbcSetting  SBC Setting
   @param  IsInitial   Indicates it's a Initial Config

   @retval EFI_STATUS  StreamChannelConfig result
 **/
EFI_STATUS
StreamChannelConfig (
    SBC_SETTING *SbcSetting,
    BOOLEAN IsInitial
    )
{
    AVDTP_SIGNAL_PACKET AvdtpSignal;
    AVDTP_SIGNAL_DATA   *SignalData = &(AvdtpSignal.Single.SignalData);
    AVDTP_SBC_CONFIG    *SbcConfig;
    UINT8               SignalId;
    UINT16              PayloadLength;

    if (gAcpSeid == 0) {
        return EFI_NOT_READY;
    }

    if (SbcSetting == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    if (IsInitial) {
        SignalId = AVDTP_SET_CONFIGURATION;
        PayloadLength = AVDTP_SET_CONFIGURATION_LENGTH;
        SignalData->SetConfig.AcpSeid = gAcpSeid;
        SignalData->SetConfig.IntSeid = gAcpSeid;       // fake
        SignalData->SetConfig.MediaTrans = 0x01;
        SignalData->SetConfig.MediaTransInfoLength = 0x00;
        SignalData->SetConfig.MediaCodec = 0x07;
        SignalData->SetConfig.MediaCodecInfoLength = 0x06;
        SignalData->SetConfig.CodecId = 0x0000;       // SBC Id
        SbcConfig = &SignalData->SetConfig.SbcConfig;
    } else {
        if (gSbcSetting.Raw == SbcSetting->Raw) {
            return EFI_SUCCESS;
        }
        SignalId = AVDTP_RECONFIGURE;
        PayloadLength = AVDTP_RECONFIGURE_LENGTH;
        SignalData->Reconfig.AcpSeid = gAcpSeid;
        SignalData->Reconfig.MediaCodec = 0x07;
        SignalData->Reconfig.MediaCodecInfoLength = 0x06;
        SignalData->Reconfig.CodecId = 0x0000; // SBC Id
        SbcConfig = &SignalData->Reconfig.SbcConfig;
    }

    gSbcSetting.Raw = SbcSetting->Raw;
    SbcConfig->ChannelMode = 1 << (3 - gSbcSetting.ChannelMode);
    SbcConfig->SamplingFrequency = 1 << (3 - gSbcSetting.SamplingFrequency);
    SbcConfig->AllocationMethod = 1 + gSbcSetting.AllocationMethod;
    SbcConfig->Subbands = 1 << (1 - gSbcSetting.Subbands);
    SbcConfig->BlockLength = 1 << (3 - gSbcSetting.Blocks);
    SbcConfig->BitpoolMin = DEFAULT_BITPOOL_MIN;
    SbcConfig->BitpoolMax = DEFAULT_BITPOOL_MAX;

    return SendAvdtpCommnad (SignalId, PayloadLength, &AvdtpSignal);
}

/**
   SendStreamFile

   @param  Repeat  Indicates it play repeatly.
 **/
EFI_STATUS
SendStreamFile (BOOLEAN Repeat)
{
    EFI_STATUS        Status;
    UINT64            FileSize;
    UINT64            ReadSize;
    SBC_FRAME_HEADER  FrameHeader;
    UINT8             Blocks;
    UINT8             Subbands;
    UINT16            FrameSize;
    UINT8             FrameCount;
    UINT32            TimeLength;
    UINT16            SbcPackSize;
    UINTN             SkipSize;
    SBC_PACKET_HEADER *StreamPack;
    UINT32            TimeStamp;
    UINT16            Seqns;
    UINT64            Position;
    EFI_INPUT_KEY     Key;

    OpenSelectedFile (&FileSize);

    ReadSize = sizeof(SBC_FRAME_HEADER);
    ReadSelectedFile (0, &ReadSize, &FrameHeader);

    Blocks = (FrameHeader.Setting.Blocks + 1) * 4;
    Subbands = (FrameHeader.Setting.Subbands + 1) * 4;
    if (FrameHeader.Setting.ChannelMode == STEREO) {
        FrameSize = 4 + Subbands + (Blocks * FrameHeader.Setting.Bitpool + 7) / 8; // +7: Ceiling Function
    } else if (FrameHeader.Setting.ChannelMode == JOINT_STEREO) {
        FrameSize = 4 + Subbands + (Subbands + Blocks * FrameHeader.Setting.Bitpool + 7) / 8; // +7: Ceiling Function
    }

    FrameCount = (MAX_ACL_BUFFER * ACL_COUNT_PER_SBC_PACKET - SBC_PACKET_HEADER_SIZE) / FrameSize;
    if (FrameCount > 0x0F) {
        FrameCount = 0x0F;
    }
    TimeLength = Blocks * Subbands * FrameCount;
    SbcPackSize = FrameSize * FrameCount;
    SkipSize = SbcPackSize * SBCPACK_SKIP_COUNT;

    Status = gBS->AllocatePool(EfiBootServicesData, sizeof(SBC_PACKET_HEADER) + SbcPackSize, &StreamPack);
    if (EFI_ERROR(Status)) {
        goto SendStreamFileExit;
    }
    StreamPack->BFrame.Cid = gAvdtpStrCid;
    StreamPack->BFrame.PayloadLength = 13 + SbcPackSize;  // MediaPayloadHeader+FrameHeader = 13
    StreamPack->Type = 0x0180;
    StreamPack->SbcHeader = FrameCount;
    StreamPack->Ssrc = SwapBytes32(0x01);
    TimeStamp = 0;
    Seqns = 0;

    Status = StreamChannelConfig(&(FrameHeader.Setting), FALSE);
    if (EFI_ERROR (Status)) {
        goto SendStreamFileExit;
    }

    Status = StreamChannelControl(AVDTP_START);
    if (EFI_ERROR (Status)) {
        goto SendStreamFileExit;
    }

    Position = 0;
    ReadSize = SbcPackSize;
    while (TRUE) {
        ReadSelectedFile(Position, &ReadSize, (StreamPack + 1));
        if (ReadSize != SbcPackSize) {
            break;
        }
        StreamPack->Seqns = SwapBytes16(Seqns);
        StreamPack->TimeStamp = SwapBytes32(TimeStamp);
        Status = SendStreamAcl (StreamPack);
        if (EFI_ERROR(Status)) {
            break;
        }

        Seqns++;
        TimeStamp += TimeLength;
        Position += ReadSize;
        if (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key) == EFI_SUCCESS) {
            if (Key.ScanCode == SCAN_ESC) {
                Status = EFI_ABORTED;
                break;
            } else if (Key.ScanCode == SCAN_LEFT) {
                if (Position > SkipSize) {
                    Position -= SkipSize;
                } else {
                    Position = 0;
                }
                continue;
            } else if (Key.ScanCode == SCAN_RIGHT) {
                Position += SkipSize;
                continue;
            } else if (Key.ScanCode == SCAN_PAGE_DOWN) {
                Position = FileSize;
            } else if (Key.ScanCode == SCAN_HOME) {
                Position = 0;
            }
        }
        if (Position >= FileSize) {
            if (Repeat == FALSE) { // skip remain file < SbcPack
                break;
            }
            Position = 0;
        }
    }
    DrainRecievedData();
    StreamChannelControl(AVDTP_SUSPEND);

SendStreamFileExit:
    CloseSelectedFile();
    if (StreamPack != NULL) {
        gBS->FreePool(StreamPack);
    }

    return Status;
}

/**
   AvdtpStart

   @retval EFI_STATUS  StartAvdtp result
 **/
EFI_STATUS
AvdtpStart(VOID)
{
    EFI_STATUS          Status;
    AVDTP_SIGNAL_PACKET AvdtpSignal;
    AVDTP_SIGNAL_DATA   *SignalData = &(AvdtpSignal.Single.SignalData);
    SBC_SETTING         SbcSetting;

    Status = ConnectChannel (AVDTP_PSM, DEFAULT_AVDTP_SIGNAL_CHANNEL_ID, &gAvdtpSigCid);
    if (EFI_ERROR (Status)) {
        return Status;
    }

    Status = SendAvdtpCommnad (AVDTP_DISCOVER, AVDTP_DISCOVER_LENGTH, &AvdtpSignal);
    if (EFI_ERROR (Status)) {
        return Status;
    }
    gAcpSeid = SignalData->AcpSeid;

    //
    // Already have SignalData->AcpSeid = gAcpSeid
    //
    Status = SendAvdtpCommnad (AVDTP_GET_CAPABILITIES, AVDTP_GET_CAPABILITIES_LENGTH, &AvdtpSignal);
    if (EFI_ERROR (Status)) {
        return Status;
    }

    //
    // Skip CAPABILITIES Check here, should return Unsupport if check fail.
    //

    SbcSetting.Raw = DEFAULT_SBC_SETTING;
    Status = StreamChannelConfig(&SbcSetting, TRUE);
    if (EFI_ERROR (Status)) {
        return Status;
    }

    return StreamChannelControl(AVDTP_OPEN);
}

/**
   AvdtpStop

   @retval EFI_STATUS  AvdtpStop result
 **/
EFI_STATUS
AvdtpStop (VOID)
{
    StreamChannelControl(AVDTP_CLOSE);
    if (gAvdtpSigCid != 0) {
        DisconnectChannel(gAvdtpSigCid, DEFAULT_AVDTP_SIGNAL_CHANNEL_ID);
        gAvdtpSigCid = 0;
    }
    return EFI_SUCCESS;
}
