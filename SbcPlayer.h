/** @file
    internal inc file.

 **/

#ifndef _SBC_PLAYER_H_
#define _SBC_PLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include "SbcPlayerConfig.h"

//
// Avdtp.c
//
/**
   AvdtpStart

   @retval EFI_STATUS  StartAvdtp result
 **/
EFI_STATUS
AvdtpStart(VOID);

/**
   AvdtpStop

   @retval EFI_STATUS  AvdtpStop result
 **/
EFI_STATUS
AvdtpStop (VOID);

/**
   SendStreamFile

   @param  Repeat  Indicates it play repeatly.
 **/
EFI_STATUS SendStreamFile (BOOLEAN Repeat);

//
// BtBase.c
//
/**
   BtSetup

   @retval EFI_STATUS  BtSetup result
 **/
EFI_STATUS BtSetup (VOID);

/**
   GetRemoteBtList

   @param  DeviceCount  DeviceCount

   @retval CHAR16       Pointer to Remote Device Name List
 **/
CHAR16** GetRemoteBtList (IN OUT UINT8 *DeviceCount);

/**
   CleanRemoteBtListBuffer

   @retval None
 **/
VOID CleanRemoteBtListBuffer (VOID);

/**
   ConnectDevice

   @param  DeviceIndex  Device Index

   @retval EFI_STATUS   ConnectDevice result
 **/
EFI_STATUS ConnectDevice (UINT8 DeviceIndex);

/**
   DisconnectDevice

   @param  None
 **/
VOID DisconnectDevice (VOID);

/**
   ConnectChannel

   @param  Psm         Psm number
   @param  SCid        Source Channel Id
   @param  DCid        Buffer to Destination Channel Id

   @retval EFI_STATUS  ConnectChannel result
 **/
EFI_STATUS
ConnectChannel (UINT16 Psm, UINT16 SCid, UINT16 *DCid);

/**
   DisconnectChannel

   @param  SCid        Source Channel Id
   @param  DCid        Destination Channel Id

   @retval EFI_STATUS  DisconnectChannel result
 **/
EFI_STATUS
DisconnectChannel (UINT16 DCid, UINT16 SCid);

/**
   SendNonAutoFlushAcl

   @param  StreamPack  Pointer to StreamPack

   @retval EFI_STATUS  SendNonAutoFlushAcl result
 **/
EFI_STATUS
SendNonAutoFlushAcl (VOID *Buffer);

/**
   SendStreamAcl

   @param  StreamPack  Pointer to StreamPack

   @retval EFI_STATUS  SendStreamAcl result
 **/
EFI_STATUS
SendStreamAcl (VOID *Buffer);

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
    );

/**
   DrainRecievedData

   @param  None
 **/
VOID
DrainRecievedData (VOID);

//
// Btprint.c
//
/**
   PrintData.

   @param  DataStart
   @param  DataSize
 **/
VOID
PrintData (
    UINT8 *DataStart,
    UINTN DataSize
    );

/**
   PrintHciCmdStatus.

   @param  HciCmd
   @param  Status
 **/
VOID
PrintHciCmdStatus(
    VOID *Buffer,
    EFI_STATUS Status
    );

/**
   PrintHciEvent.

   @param  HciEvent
 **/
VOID PrintHciEvent(VOID *Buffer);

/**
   PrintAclData.

   @param  Buffer
   @param  Direction
 **/
VOID
PrintAclData (
    IN VOID *Buffer,
    IN UINT8 Direction
    );

/**
   PrintSbcFrameHeader.

   @param  FrameHeader
 **/
VOID PrintSbcFrameHeader (VOID *Buffer);

//
// FileSelection.c
//
/**
   CreateSbcFileList.

   @param  FileCount   Pointer to FileCount buffer

   @retval CHAR16**    Pointer to SBC FileList
   @retval FileCount   Number of elements in FileList
 **/
CHAR16** CreateSbcFileList (UINT8 *FileCount);

/**
   CleanFileListBuffer.

   @param  None.
 **/
VOID CleanFileListBuffer (VOID);

/**
   SelectSbcFile

   @param  Index       Select Index.

   @retval EFI_STATUS  SelectSbcFile result
 **/
EFI_STATUS SelectSbcFile (IN UINT8 Index);

/**
   OpenSelectedFile.

   @param  FileSize    File Size

   @retval EFI_STATUS  Result
 */
EFI_STATUS OpenSelectedFile (UINT64 *FileSize);

/**
   ReadSelectedFile.

   @param  Position     File Position
   @param  ReadSize     Read Size
   @param  Buffer       Buffer

   @retval EFI_STATUS  Result
 */
EFI_STATUS
ReadSelectedFile (UINT64 Position, UINT64 *ReadSize, VOID *Buffer);

/**
   CloseSelectedFile.

   @retval EFI_STATUS  Result
 */
EFI_STATUS CloseSelectedFile (VOID);

/****** DO NOT WRITE BELOW THIS LINE *******/
#ifdef __cplusplus
}
#endif

#endif
