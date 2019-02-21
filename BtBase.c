/** @file
    BT Base functions source.

 **/

#include <Protocol/UsbIo.h>
#include "BtBaseDef.h"
#include "L2CapDef.h"
#include "SbcPlayer.h"

//
// Global Vars
//
EFI_USB_IO_PROTOCOL *gBtUsbIo = NULL;
UINT8               gBulkEndPointAddr[UsbDirectionMax];
UINT8               gIntEndPointAddr;
REMOTE_DEVICE_LIST  gDeviceList[INQUIRT_RESULT_MAX];
CHAR16              *gDeviceNameList[INQUIRT_RESULT_MAX];
UINT8               gDeviceCount = 0;
UINT16              gConnectionHandle;
UINT8               gIdentifier = 0x80;

/**
   Escape

   @param  TRUE   Esc is pressed.
   @param  FALSE  Esc is not pressed.
 **/
BOOLEAN
Escape (VOID)
{
    EFI_INPUT_KEY Key;

    if (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key) != EFI_SUCCESS) {
        return FALSE;
    }
    if (Key.ScanCode == SCAN_ESC) {
        return TRUE;
    }
    return FALSE;
}

/**
   CleanRemoteBtListBuffer

   @retval None
 **/
VOID
CleanRemoteBtListBuffer (VOID)
{
    UINT8 Index;
    for (Index = 0; Index < gDeviceCount; Index++) {
        if (gDeviceNameList[Index] != NULL) {
            gBS->FreePool (gDeviceNameList[Index]);
            gDeviceNameList[Index] = NULL;
        }
    }
    gDeviceCount = 0;

    return;
}

/**
   GetgBtUsbIo

   @param  None
 **/
VOID
GetgBtUsbIo (VOID)
{
    EFI_STATUS                   Status;
    UINT8                        Index;
    UINTN                        UsbIoHandleCount;
    EFI_HANDLE                   *UsbIoHandles;
    EFI_USB_IO_PROTOCOL          *UsbIo;
    EFI_USB_DEVICE_DESCRIPTOR    UsbDevDesc;
    EFI_USB_INTERFACE_DESCRIPTOR UsbItfDesc;
    EFI_USB_ENDPOINT_DESCRIPTOR  EndpointDescriptor;
    USB_ENDPOINT_TYPE            EndPointType;
    USB_ENDPOINT_DIRECTION       Direction;

    Status = gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiUsbIoProtocolGuid,
        NULL,
        &UsbIoHandleCount,
        &UsbIoHandles
        );

    if (EFI_ERROR (Status)) {
        return;
    }

    for (Index = 0; Index < UsbIoHandleCount; Index++) {
        Status = gBS->HandleProtocol (UsbIoHandles[Index], &gEfiUsbIoProtocolGuid, (VOID**) &UsbIo);
        if (EFI_ERROR (Status)) {
            continue;
        }

        Status = UsbIo->UsbGetDeviceDescriptor(UsbIo, &UsbDevDesc);
        if (EFI_ERROR (Status)) {
            continue;
        }

        Status = UsbIo->UsbGetInterfaceDescriptor(UsbIo, &UsbItfDesc);
        if (EFI_ERROR (Status)) {
            continue;
        }

        if ((UsbDevDesc.DeviceClass == USB_CLASS_WIRELESS_CONTROLLER) &&
            (UsbDevDesc.DeviceSubClass == USB_SUBCLASS_RF_CONTROLLER) &&
            (UsbDevDesc.DeviceProtocol == USB_PROTOCOL_BT_PROG_INTERFACE) &&
            (UsbItfDesc.NumEndpoints == BT_USB_ENDPOINT_NUMBER)) {
            gBtUsbIo = UsbIo;
            break;
        }
    }

    if (gBtUsbIo != NULL) {
        for(Index = 0; Index < BT_USB_ENDPOINT_NUMBER; Index++) {
            Status = gBtUsbIo->UsbGetEndpointDescriptor (gBtUsbIo, Index, &EndpointDescriptor);
            if (EFI_ERROR (Status)) {
                continue;
            }
            EndPointType = EndpointDescriptor.Attributes & (BIT0 | BIT1);
            switch (EndPointType) {
            case UsbEndpointBulk:
                Direction = ((EndpointDescriptor.EndpointAddress & BIT7) == BIT7) ? UsbIn : UsbOut;
                gBulkEndPointAddr[Direction] = EndpointDescriptor.EndpointAddress;
                break;
            case UsbEndpointInterrupt:
                gIntEndPointAddr = EndpointDescriptor.EndpointAddress;
                break;
            default:
                continue;
            }
        }
    }

    if (UsbIoHandles != NULL) {
        gBS->FreePool(UsbIoHandles);
    }
    return;
}

/**
   ReceiveHciEvent

   @param  HciEvent   Pointer to HciEvent
   @param  CountMax   Max count for read try

   @retval EFI_STATUS  ReceiveHciEvent result
 **/
EFI_STATUS
ReceiveHciEvent (HCI_EVENT_PACKET *HciEvent)
{
    EFI_STATUS Status;
    UINTN      DataLength;
    UINT32     UsbStatus;

    DataLength = sizeof(HCI_EVENT_PACKET);
    Status = gBtUsbIo->UsbSyncInterruptTransfer (gBtUsbIo, gIntEndPointAddr, HciEvent, &DataLength, USB_TRANS_TIMEOUT_HCI_EVENT, &UsbStatus);
    if (Status == EFI_SUCCESS) {
        PrintHciEvent(HciEvent);
    }
    return Status;
}

/**
   WaitEventCode

   @param  HciEvent    Pointer to HciEvent
   @param  EventCode   EventCode to wait for

   @retval EFI_STATUS  WaitEventCode result
 **/
EFI_STATUS
WaitEventCode (
    HCI_EVENT_PACKET *HciEvent,
    UINT8 EventCode
    )
{
    EFI_STATUS Status;
    UINT8      ReadCountDown = READ_TRIAL_COUNT;

    while (ReadCountDown--) {
        Status = ReceiveHciEvent (HciEvent);
        if (Status == EFI_SUCCESS) {
            if (HciEvent->EventCode == EventCode) {
                return EFI_SUCCESS;
            }
        } else if (Status != EFI_TIMEOUT) {
            return Status;
        }
    }
    return EFI_TIMEOUT;
}

/**
   SendHciCommand

   @param  HciCmd   Pointer to HciCmd

   @retval EFI_STATUS  SendHciCommand result
 **/
EFI_STATUS
SendHciCommand (HCI_COMMAND_PACKET  *HciCmd)
{
    EFI_STATUS             Status;
    EFI_USB_DEVICE_REQUEST Request;
    UINT32                 UsbStatus;

    Request.RequestType = 0x20;
    Request.Request = 0x00;
    Request.Value = 0x00;
    Request.Index = 0x00;
    Request.Length = HCI_COMMAND_HEADER_SIZE + HciCmd->ParameterTotalLength;

    Status = gBtUsbIo->UsbControlTransfer (gBtUsbIo,
                                           &Request,
                                           EfiUsbDataOut,
                                           USB_TRANS_TIMEOUT_HCI_COMMAND,
                                           HciCmd,
                                           Request.Length,
                                           &UsbStatus);
    PrintHciCmdStatus(HciCmd, Status);

    return Status;
}

/**
   SendHciCommandWaitEvent

   @param  HciCmd      Pointer to HciCmd
   @param  HciEvent    Pointer to HciEvent
   @param  EventCode   EventCode to wait for

   @retval EFI_STATUS  WaitEventCode result
 **/
EFI_STATUS
SendHciCommandWaitEvent (
    HCI_COMMAND_PACKET *HciCmd,
    HCI_EVENT_PACKET *HciEvent,
    UINT8 EventCode
    )
{
    EFI_STATUS Status;

    Status = SendHciCommand(HciCmd);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    return WaitEventCode(HciEvent, EventCode);
}

/**
   ReadACLData

   @param  Buffer      Pointer to ACLData Buffer
   @param  BufferSize  Buffer Size

   @retval EFI_STATUS  ReadACLData result
 **/
EFI_STATUS
ReadACLData (
    IN OUT VOID  *Buffer,
    IN OUT UINTN *BufferSize
    )
{
    EFI_STATUS Status;
    UINT32     UsbStatus;

    Status = gBtUsbIo->UsbBulkTransfer (gBtUsbIo,
                                        gBulkEndPointAddr[DIRECTION_IN],
                                        Buffer,
                                        BufferSize,
                                        USB_TRANS_TIMEOUT_ACL_READ,
                                        &UsbStatus);
    if (Status == EFI_SUCCESS) {
        PrintAclData(Buffer, READ_ACL);
    }

    return Status;
}

/**
   DrainRecievedData

   @param  None
 **/
VOID
DrainRecievedData (VOID)
{
    UINT8            Buffer[0x100];
    UINTN            BufferSize;
    HCI_EVENT_PACKET HciEvent;

    do {
        BufferSize = 0x100;
    } while (ReadACLData(Buffer, &BufferSize) == EFI_SUCCESS);
    while (ReceiveHciEvent(&HciEvent) == EFI_SUCCESS);

    return;
}

/**
   SendACLData

   @param  Buffer      Pointer to ACLData

   @retval EFI_STATUS  SendACLData result
 **/
EFI_STATUS
SendACLData (VOID *Buffer)
{
    EFI_STATUS       Status;
    UINTN            BufferSize;
    UINT32           UsbStatus;
    HCI_EVENT_PACKET HciEvent;

    BufferSize = ((HCI_ACL_DATA_HEADER*)Buffer)->DataLength + sizeof(HCI_ACL_DATA_HEADER);

    PrintAclData(Buffer, SEND_ACL);

    Status = gBtUsbIo->UsbBulkTransfer (gBtUsbIo,
                                        gBulkEndPointAddr[DIRECTION_OUT],
                                        Buffer,
                                        &BufferSize,
                                        USB_TRANS_TIMEOUT_ACL_SEND,
                                        &UsbStatus);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    return WaitEventCode(&HciEvent, NUMBER_OF_COMPLETED_PACKETS);
}

/**
   SendNonAutoFlushAcl

   @param  StreamPack  Pointer to StreamPack

   @retval EFI_STATUS  SendNonAutoFlushAcl result
 **/
EFI_STATUS
SendNonAutoFlushAcl (VOID *Buffer)
{
    HCI_ACL_DATA_HEADER *Acl = (HCI_ACL_DATA_HEADER*)Buffer;

    Acl->Handle = ACL_HANDLE (BROADCAST_NO, BOUNDARY_NONAUTO_FLUSHABLE_HOST_TO_CONTROL, gConnectionHandle);
    Acl->DataLength = ((L2CAP_B_FRAME_HEADER*)(Acl + 1))->PayloadLength + sizeof(L2CAP_B_FRAME_HEADER);

    return SendACLData (Buffer);
}


/**
   SendStreamAcl

   @param  StreamPack  Pointer to StreamPack

   @retval EFI_STATUS  SendStreamAcl result
 **/
EFI_STATUS
SendStreamAcl (VOID *Buffer)
{
    EFI_STATUS          Status;
    HCI_ACL_DATA_HEADER *Acl = (HCI_ACL_DATA_HEADER*)Buffer;
    UINT16              DataSize = ((L2CAP_B_FRAME_HEADER*)(Acl + 1))->PayloadLength + sizeof(L2CAP_B_FRAME_HEADER);

    Acl->Handle = ACL_HANDLE (BROADCAST_NO, BOUNDARY_AUTO_FLUSHABLE, gConnectionHandle);
    while (DataSize > MAX_ACL_BUFFER) {
        Acl->DataLength = MAX_ACL_BUFFER;
        Status = SendACLData(Acl);
        if (EFI_ERROR(Status)) {
            return Status;
        }
        DataSize -= MAX_ACL_BUFFER;
        Acl = (HCI_ACL_DATA_HEADER*)((UINT8*)Acl + MAX_ACL_BUFFER);
        Acl->Handle = ACL_HANDLE (BROADCAST_NO, BOUNDARY_CONTINUING_FRAGMENT, gConnectionHandle);
    }
    Acl->DataLength = DataSize;
    return SendACLData(Acl);
}

/**
   SignalChannelSend

   @param  SignalData  Pointer to SignalData

   @retval EFI_STATUS  SignalChannelSend result
 **/
EFI_STATUS
SignalChannelSend (L2CAP_SIGNAL_CHANNEL *SignalData)
{
    SignalData->BFrame.PayloadLength = SignalData->Signal.Length + sizeof(L2CAP_SIGNAL_HEADER);
    SignalData->BFrame.Cid = SIGNAL_CHANNEL_ID;

    return SendNonAutoFlushAcl(SignalData);
}

/**
   SignalChannelWaitCode

   @param  SignalData  Pointer to SignalData
   @param  Code        Code to wait for

   @retval EFI_STATUS  SignalChannelWaitCode result
 **/
EFI_STATUS
SignalChannelWaitCode(
    L2CAP_SIGNAL_CHANNEL *SignalData,
    UINT8 Code)
{
    EFI_STATUS Status;
    UINTN      BufferSize;

    while (TRUE) {
        if (Escape()) {
            break;
        }
        BufferSize = MAX_ACL_BUFFER;
        Status = ReadACLData(SignalData, &BufferSize);
        if (Status == EFI_SUCCESS) {
            if (SignalData->BFrame.Cid != SIGNAL_CHANNEL_ID) {
                continue;
            }
            if (SignalData->Signal.Code != Code) {
                continue;
            }
            return EFI_SUCCESS;
        }
        if (Status != EFI_TIMEOUT) {
            return Status;
        }
    }
    return EFI_TIMEOUT;
}

EFI_STATUS
SignalChannelSendAndWait (L2CAP_SIGNAL_CHANNEL *SignalData, UINT8 Code)
{
    EFI_STATUS Status;

    Status = SignalChannelSend(SignalData);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    return SignalChannelWaitCode(SignalData, Code);
}

/**
   ConnectChannel

   @param  Psm         Psm number
   @param  SCid        Source Channel Id
   @param  DCid        Buffer to Destination Channel Id

   @retval EFI_STATUS  ConnectChannel result
 **/
EFI_STATUS
ConnectChannel (UINT16 Psm, UINT16 SCid, UINT16 *DCid)
{
    EFI_STATUS           Status;
    L2CAP_SIGNAL_CHANNEL SignalData;

    SignalData.ConnectReq.Psm = Psm;
    SignalData.ConnectReq.SCid = SCid;

    SignalData.Signal.Code = L2CAP_CONNECTION_REQUEST_CODE;
    SignalData.Signal.Identifier = gIdentifier++;
    SignalData.Signal.Length = sizeof(L2CAP_CONNECTION_REQUEST);

    Status = SignalChannelSendAndWait(&SignalData, L2CAP_CONNECTION_RESPONSE_CODE);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    while ((SignalData.ConnectRes.Result != 0) || (SignalData.ConnectRes.Status != 0)) {
        SignalChannelWaitCode(&SignalData, L2CAP_CONNECTION_RESPONSE_CODE);
        if (Escape()) {
            return EFI_NOT_READY;
        }
    }

    *DCid = SignalData.ConnectRes.DCid;

    //
    // Config Channel
    //
    SignalData.ConfigReq.DCid = *DCid;
    SignalData.ConfigReq.Flag = 0x0;
    SignalData.Signal.Code = L2CAP_CONFIGURATION_REQUEST_CODE;
    SignalData.Signal.Identifier = gIdentifier++;
    SignalData.Signal.Length = sizeof(L2CAP_CONFIGURATION_REQUEST);

    Status = SignalChannelSend(&SignalData);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = SignalChannelWaitCode(&SignalData, L2CAP_CONFIGURATION_REQUEST_CODE);

    if (Status == EFI_SUCCESS) {
        SignalData.ConfigRes.SCid = *DCid;
        SignalData.ConfigRes.Flag = 0x00;
        SignalData.ConfigRes.Result = 0x00;
        SignalData.Signal.Code = L2CAP_CONFIGURATION_RESPONSE_CODE;
        SignalData.Signal.Length = sizeof(L2CAP_CONFIGURATION_RESPONSE);

        Status = SignalChannelSend(&SignalData);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }
    //
    // Ignore Config Responce
    //

    return EFI_SUCCESS;
}

/**
   DisconnectChannel

   @param  SCid        Source Channel Id
   @param  DCid        Destination Channel Id

   @retval EFI_STATUS  DisconnectChannel result
 **/
EFI_STATUS
DisconnectChannel (UINT16 DCid, UINT16 SCid)
{
    L2CAP_SIGNAL_CHANNEL SignalData;

    SignalData.DisconReq.DCid = DCid;
    SignalData.DisconReq.SCid = SCid;

    SignalData.Signal.Code = L2CAP_DISCONNECTION_REQUEST_CODE;
    SignalData.Signal.Identifier = gIdentifier++;
    SignalData.Signal.Length = sizeof(L2CAP_DISCONNECTION_REQUEST_CODE);

    return SignalChannelSendAndWait(&SignalData, L2CAP_DISCONNECTION_RESPONSE_CODE);
}

/**
   InquiryDevice

   @param  DevCount    Buffer to DevCount

   @retval EFI_STATUS  InquiryDevice result
 **/
EFI_STATUS
InquiryDevice (OUT UINT8 *DevCount)
{
    EFI_STATUS         Status;
    HCI_COMMAND_PACKET HciCmd;
    HCI_EVENT_PACKET   HciEvent;
    UINT8              ResultCount;

    if (DevCount == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    HciCmd.OpCode = BT_OPCODE(LINK_CONTROL, HCI_INQUIRY);
    HciCmd.ParameterTotalLength = sizeof(HCI_INQUIRY_DATA);
    HciCmd.Inquiry.Lap = DEFAULT_INQUIRY_LAP;
    HciCmd.Inquiry.InquiryLength = DEFAULT_INQUIRY_LENGTH;
    HciCmd.Inquiry.NumResponses = INQUIRT_RESULT_MAX;
    Status = SendHciCommand(&HciCmd);
    if (EFI_ERROR(Status)) {
        return 0;
    }

    for (gDeviceCount = 0; gDeviceCount < INQUIRT_RESULT_MAX;) {
        if (Escape()) {
            HciCmd.OpCode = BT_OPCODE(LINK_CONTROL, HCI_INQUIRY_CANCEL);
            HciCmd.ParameterTotalLength = 0;
            Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
            break;
        }

        Status = ReceiveHciEvent (&HciEvent);
        if (EFI_ERROR(Status)) {
            continue;
        }

        if (HciEvent.EventCode == INQUIRY_RESULT) {
            for (ResultCount = 0; ResultCount < HciEvent.Inquiry.RespNum;) {
                gDeviceList[gDeviceCount].BdAddr = HciEvent.Inquiry.Result[ResultCount].BdAddr;
                gDeviceList[gDeviceCount].ClockOffset = HciEvent.Inquiry.Result[ResultCount].ClockOffset;
                gDeviceList[gDeviceCount].PageScanRepMode = HciEvent.Inquiry.Result[ResultCount].PageScanRepMode;
                ResultCount++;
                gDeviceCount++;
            }
            continue;
        }
        if (HciEvent.EventCode == INQUIRY_COMPLETE) {
            break;
        }
    }
    *DevCount = gDeviceCount;
    return Status;
}

/**
   GetRemoteBtList

   @param  DeviceCount  DeviceCount

   @retval CHAR16       Pointer to Remote Device Name List
 **/
CHAR16**
GetRemoteBtList (IN OUT UINT8 *DeviceCount)
{
    EFI_STATUS         Status;
    UINT8              ResultCount;
    UINT8              Index;
    HCI_COMMAND_PACKET HciCmd;
    HCI_EVENT_PACKET   HciEvent;
    UINTN              NameLength;

    if (DeviceCount == NULL) {
        return NULL;
    }

    Status = InquiryDevice(&ResultCount);
    if (EFI_ERROR(Status)) {
        *DeviceCount = 0;
        return NULL;
    }

    CleanRemoteBtListBuffer();
    for (Index = 0; Index < ResultCount; Index++) {
        gBS->AllocatePool (EfiBootServicesData, sizeof(CHAR16) * BDADDR_NAME_LENGTH, &gDeviceNameList[Index]);
        UnicodeSPrint(
            gDeviceNameList[Index],
            sizeof(CHAR16) * BDADDR_NAME_LENGTH,
            BDADDR_NAME_STRING,
            gDeviceList[Index].BdAddrPtr[5],
            gDeviceList[Index].BdAddrPtr[4],
            gDeviceList[Index].BdAddrPtr[3],
            gDeviceList[Index].BdAddrPtr[2],
            gDeviceList[Index].BdAddrPtr[1],
            gDeviceList[Index].BdAddrPtr[0]
            );
    }

    for (Index = 0; Index < ResultCount; Index++) {
        if (Escape()) {
            break;
        }
        HciCmd.RemoteNameReq.BdAddr = gDeviceList[Index].BdAddr;
        HciCmd.RemoteNameReq.PageScanRepMode = gDeviceList[Index].PageScanRepMode;
        HciCmd.RemoteNameReq.Reserved = 0;
        HciCmd.RemoteNameReq.ClockOffset = gDeviceList[Index].ClockOffset;
        HciCmd.OpCode = BT_OPCODE(LINK_CONTROL, HCI_REMOTE_NAME_REQUEST);
        HciCmd.ParameterTotalLength = sizeof(HCI_REMOTE_NAME_REQUEST_DATA);
        Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, HCI_REMOTE_NAME_REQUEST_COMPLETE);
        if (EFI_ERROR(Status)) {
            continue;
        }
        NameLength = AsciiStrLen(HciEvent.RemoteName.Name) + 1;// Add 1 for End
        if (NameLength == 1) {
            continue;
        }
        gBS->FreePool(gDeviceNameList[Index]);
        gBS->AllocatePool (EfiBootServicesData, sizeof(CHAR16) * NameLength, &gDeviceNameList[Index]);
        AsciiStrToUnicodeStr(HciEvent.RemoteName.Name, gDeviceNameList[Index]);
    }
    *DeviceCount = ResultCount;
    return gDeviceNameList;
}

/**
   ConnectDevice

   @param  DeviceIndex  Device Index

   @retval EFI_STATUS   ConnectDevice result
 **/
EFI_STATUS
ConnectDevice (UINT8 DeviceIndex)
{
    EFI_STATUS         Status;
    UINT64             BdAddr = gDeviceList[DeviceIndex].BdAddr;
    UINT8              PageScanRepMode = gDeviceList[DeviceIndex].PageScanRepMode;
    UINT16             ClockOffset = gDeviceList[DeviceIndex].ClockOffset;
    HCI_COMMAND_PACKET HciCmd;
    HCI_EVENT_PACKET   HciEvent;

    // HCI_CREATE_CONNECTION
    HciCmd.OpCode = BT_OPCODE(LINK_CONTROL, HCI_CREATE_CONNECTION);
    HciCmd.ParameterTotalLength = sizeof(HCI_CREATE_CONNECTION_DATA);
    HciCmd.CreateConnection.BdAddr = BdAddr;
    HciCmd.CreateConnection.PacketType = ACL_PACKET_TYPE_DEFAULT;
    HciCmd.CreateConnection.PageScanRepMode = PageScanRepMode;
    HciCmd.CreateConnection.Reserved = 0;
    HciCmd.CreateConnection.ClockOffset = ClockOffset;
    HciCmd.CreateConnection.RoleSwith = 0x0;
    Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, CONNECTION_COMPLETE);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    gConnectionHandle = HciEvent.ConnectionComplete.Handle;

    // AUTHENTICATION_REQUEST
    HciCmd.OpCode = BT_OPCODE(LINK_CONTROL, AUTHENTICATION_REQUESTED);
    HciCmd.ParameterTotalLength = sizeof(AUTHENTICATION_REQUESTED_DATA);
    HciCmd.AuthRequested.Handle = gConnectionHandle;
    Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, LINK_KEY_REQUEST);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    // LINK_KEY_REQUEST_NEGATIVE_REPLY
    HciCmd.OpCode = BT_OPCODE(LINK_CONTROL, LINK_KEY_REQUEST_NEGATIVE_REPLY);
    HciCmd.ParameterTotalLength = sizeof(LINK_KEY_REQUEST_NEGATIVE_REPLY_DATA);
    gBS->CopyMem(
        HciCmd.LinkKeyNegative.BdAddrPtr,
        &BdAddr,
        sizeof(LINK_KEY_REQUEST_NEGATIVE_REPLY_DATA));
    Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, IO_CAPABILITY_REQUEST);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    // IO_CAPABILITY_REQUEST_REPLY
    HciCmd.OpCode = BT_OPCODE(LINK_CONTROL, IO_CAPABILITY_REQUEST_REPLY);
    HciCmd.ParameterTotalLength = sizeof(IO_CAPABILITY_REQUEST_REPLY_DATA);
    HciCmd.IoCapReqReply.BdAddr = BdAddr;
    HciCmd.IoCapReqReply.IoCapability = 0x01;
    HciCmd.IoCapReqReply.OobData = 0;
    HciCmd.IoCapReqReply.AuthRequirement = 0x03;
    Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, USER_CONFIRMATION_REQUEST);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    // USER_CONFIRMATION_REQUEST_REPLY
    HciCmd.OpCode = BT_OPCODE(LINK_CONTROL, USER_CONFIRMATION_REQUEST_REPLY);
    HciCmd.ParameterTotalLength = sizeof(USER_CONFIRMATION_REQUEST_REPLY_DATA);
    gBS->CopyMem(
        HciCmd.UserConfirmReqReply.BdAddrPtr,
        &BdAddr,
        sizeof(USER_CONFIRMATION_REQUEST_REPLY_DATA));
    Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, AUTHENTICATION_COMPLETE);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    // SET_CONNECTION_ENCRYPTION
    HciCmd.OpCode = BT_OPCODE(LINK_CONTROL, SET_CONNECTION_ENCRYPTION);
    HciCmd.ParameterTotalLength = sizeof(SET_CONNECTION_ENCRYPTION_DATA);
    HciCmd.SetConnectionEncpyt.Handle = gConnectionHandle;
    HciCmd.SetConnectionEncpyt.EncrypEnabled = 1;
    Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, ENCTYPTION_CHANGE);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    return AvdtpStart();
}

/**
   DisconnectDevice

   @param  None
 **/
VOID
DisconnectDevice(VOID)
{
    HCI_COMMAND_PACKET HciCmd;
    HCI_EVENT_PACKET   HciEvent;

    if (gConnectionHandle == 0) {
        return;
    }
    DrainRecievedData();
    AvdtpStop();
    HciCmd.OpCode = BT_OPCODE(LINK_CONTROL, HCI_DISCONNECT);
    HciCmd.ParameterTotalLength = sizeof(HCI_DISCONNECT);
    HciCmd.Disconnect.Handle = gConnectionHandle;
    HciCmd.Disconnect.Reason = 0x05;
    SendHciCommandWaitEvent(&HciCmd, &HciEvent, DISCONNECTION_COMPLETE);
    gConnectionHandle = 0;

    return;
}

/**
   BtSetup

   @retval EFI_STATUS  BtSetup result
 **/
EFI_STATUS
BtSetup (VOID)
{
    EFI_STATUS         Status;
    HCI_COMMAND_PACKET HciCmd;
    HCI_EVENT_PACKET   HciEvent;

    GetgBtUsbIo ();
    if (gBtUsbIo == NULL) {
        return EFI_NOT_FOUND;
    }

    // Reset
    HciCmd.OpCode = BT_OPCODE(0x03, 0x0003);
    HciCmd.ParameterTotalLength = 0;
    Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
    if (EFI_ERROR(Status)) {
        return Status;
    }
    if (1) {// Set Event Filter
        HciCmd.OpCode = BT_OPCODE(0x03, 0x0005);
        HciCmd.ParameterTotalLength = 1;
        HciCmd.Parameter[0] = 0;
        Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }
    if (1) {// Write Connection Accept Timeout 32000*0.625 = 20s
        HciCmd.OpCode = BT_OPCODE(0x03, 0x0016);
        HciCmd.ParameterTotalLength = 2;
        *((UINT16*)(&HciCmd.Parameter[0])) = 32000;
        Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }
    if (1) {// Write Simple Pairing Mode
        HciCmd.OpCode = BT_OPCODE(0x03, 0x0056);
        HciCmd.ParameterTotalLength = 1;
        HciCmd.Parameter[0] = 1;
        Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }
    if (1) {// Write Inquiry Mode
        HciCmd.OpCode = BT_OPCODE(0x03, 0x0045);
        HciCmd.ParameterTotalLength = 1;
        HciCmd.Parameter[0] = 0;
        Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }
    if (1) {// Set Event Mask
        HciCmd.OpCode = BT_OPCODE(0x03, 0x0001);
        HciCmd.ParameterTotalLength = 8;
        *((UINT64*)(&HciCmd.Parameter[0])) = 0x3dbff807fffbffff;
        Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }
    if (1) {// Write Default Link Policy Settings
        HciCmd.OpCode = BT_OPCODE(0x02, 0x000f);
        HciCmd.ParameterTotalLength = 2;
        HciCmd.Parameter[0] = 0xf;
        Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }
    if (1) {// Write Page Scan Type
        HciCmd.OpCode = BT_OPCODE(0x03, 0x0047);
        HciCmd.ParameterTotalLength = 1;
        HciCmd.Parameter[0] = 0;
        Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }
    if (1) {// Write Scan Enable
        HciCmd.OpCode = BT_OPCODE(0x03, 0x001a);
        HciCmd.ParameterTotalLength = 1;
        HciCmd.Parameter[0] = 3;
        Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }
    if (1) {// Write Class of Device
        HciCmd.OpCode = BT_OPCODE(0x03, 0x0024);
        HciCmd.ParameterTotalLength = 3;
        HciCmd.Parameter[0] = 0x1c;
        Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }
    if (1) {// Write Local Name
        HciCmd.OpCode = BT_OPCODE(0x03, 0x0013);
        HciCmd.ParameterTotalLength = 248;
        HciCmd.Parameter[0] = 'B';
        HciCmd.Parameter[1] = 'y';
        HciCmd.Parameter[2] = 's';
        HciCmd.Parameter[3] = 'T';
        HciCmd.Parameter[4] = 'e';
        HciCmd.Parameter[5] = 's';
        HciCmd.Parameter[6] = 't';
        HciCmd.Parameter[7] = 0;
        Status = SendHciCommandWaitEvent(&HciCmd, &HciEvent, COMMAND_COMPLETE);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }

    return Status;
}
