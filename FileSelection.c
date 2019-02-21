/** @file
    FileSelection source.

 **/
#include <Protocol/Shell.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/SimpleFileSystem.h>
#include "AvdtpDef.h"
#include "SbcPlayer.h"

//
// Global Vars
//
EFI_SHELL_PROTOCOL *gShellProtocol;                        ///< EFI_SHELL_PROTOCOL
CHAR16             gFullFileName[MAX_FILENAME_LENGTH + 1]; ///< Current File Name with path, ex: fs0:/foo.sbc
CHAR16             *gFileNamePtr;                          ///< Current File Name, ex: foo.sbc
SHELL_FILE_HANDLE  gFileHandle = NULL;                     ///< Current File Handle
CHAR16             *gFileNameList[MAX_FILE_COUNT];         ///< File Name List
UINT8              gFileCount = 0;                         ///< Number of File Name List

/**
   CleanFileListBuffer.
 */
VOID
CleanFileListBuffer (VOID)
{
    UINT8 Index;

    for (Index = 0; Index < gFileCount; Index++) {
        if (gFileNameList[Index] != NULL) {
            gBS->FreePool(gFileNameList[Index]);
            gFileNameList[Index] = NULL;
        }
    }
    gFileCount = 0;
    return;
}

/**
   OpenSelectedFile.

   @param  FileSize    File Size

   @retval EFI_STATUS  Result
 */
EFI_STATUS
OpenSelectedFile (UINT64 *FileSize)
{
    if (gFileHandle != NULL) {
        return EFI_ALREADY_STARTED;
    }
    gShellProtocol->OpenFileByName (gFullFileName, &gFileHandle, EFI_FILE_MODE_READ);
    if (FileSize != NULL) {
        gShellProtocol->GetFileSize (gFileHandle, FileSize);
    }
    return EFI_SUCCESS;
}

/**
   ReadSelectedFile.

   @param  Position     File Position
   @param  ReadSize     Read Size
   @param  Buffer       Buffer

   @retval EFI_STATUS  Result
 */
EFI_STATUS
ReadSelectedFile (UINT64 Position, UINT64 *ReadSize, VOID *Buffer)
{
    if (gFileHandle == NULL) {
        return EFI_NOT_STARTED;
    }
    gShellProtocol->SetFilePosition (gFileHandle, Position);
    gShellProtocol->ReadFile (gFileHandle, ReadSize, Buffer);
    return EFI_SUCCESS;
}

/**
   CloseSelectedFile.

   @retval EFI_STATUS  Result
 */
EFI_STATUS
CloseSelectedFile (VOID)
{
    if (gFileHandle == NULL) {
        return EFI_NOT_STARTED;
    }
    gShellProtocol->CloseFile (gFileHandle);
    gFileHandle = NULL;
    return EFI_SUCCESS;
}

/**
   CheckSbcFrameHeader.

   @param  SbcFile      Pointer to FrameHeader

   @retval EFI_STATUS   CheckSbcFrameHeader Result
 **/
EFI_STATUS
CheckSbcFrameHeader (SBC_FRAME_HEADER *FrameHeader)
{
    PrintSbcFrameHeader(FrameHeader);

    if (FrameHeader->SyncWord != SBC_SYNCWORD) {
        return EFI_UNSUPPORTED;
    }

    if ((FrameHeader->Setting.ChannelMode != STEREO) && (FrameHeader->Setting.ChannelMode != JOINT_STEREO)) {
        return EFI_UNSUPPORTED;
    }

    if ((FrameHeader->Setting.Bitpool > DEFAULT_BITPOOL_MAX) || (FrameHeader->Setting.Bitpool < DEFAULT_BITPOOL_MIN)) {
        return EFI_UNSUPPORTED;
    }

    return EFI_SUCCESS;
}

/**
   CreateSbcFileList.

   @param  FileCount   Pointer to FileCount buffer

   @retval CHAR16**    Pointer to SBC FileList
   @retval FileCount   Number of elements in FileList
 **/
CHAR16**
CreateSbcFileList (UINT8 *FileCount)
{
    EFI_STATUS          Status;
    CONST CHAR16        *DirName;
    UINTN               NameLength;
    UINTN               NameLengthMax = MAX_FILENAME_LENGTH;
    SHELL_FILE_HANDLE   DirHandle;
    EFI_SHELL_FILE_INFO *FileList;
    LIST_ENTRY          *CurLink;
    LIST_ENTRY          *EndLink;
    UINTN               ReadSize;
    SBC_FRAME_HEADER    FrameHeader;

    Status = gBS->LocateProtocol(&gEfiShellProtocolGuid, NULL, &gShellProtocol);
    if (EFI_ERROR(Status)) {
        return NULL;
    }

    DirName = gShellProtocol->GetCurDir(NULL);
    if (DirName == NULL) {
        return NULL;
    }

    NameLength = StrLen(DirName);
    if (NameLength > NameLengthMax) {
        return NULL;
    }

    StrCpy (gFullFileName, DirName);
    gFileNamePtr = &gFullFileName[NameLength++];
    *gFileNamePtr++ = L'\\';
    *gFileNamePtr = 0;
    NameLengthMax -= NameLength;

    Status = gShellProtocol->OpenFileByName (gFullFileName, &DirHandle, EFI_FILE_MODE_READ);
    if (EFI_ERROR(Status)) {
        return NULL;
    }

    Status = gShellProtocol->FindFilesInDir(DirHandle, &FileList);
    if (EFI_ERROR(Status)) {
        gShellProtocol->CloseFile (DirHandle);
        return NULL;
    }

    CurLink = &FileList->Link;
    EndLink = CurLink->BackLink;
    do {
        CurLink = CurLink->ForwardLink;
        FileList = BASE_CR(CurLink, EFI_SHELL_FILE_INFO, Link);
        if (FileList->Info->Attribute & EFI_FILE_DIRECTORY) {
            continue;
        }
        NameLength = StrLen(FileList->FileName);
        if (NameLength > NameLengthMax) {
            continue;
        }
        StrCpy(gFileNamePtr, FileList->FileName);
        OpenSelectedFile(NULL);
        ReadSize = sizeof(SBC_FRAME_HEADER);
        ReadSelectedFile(0, &ReadSize, &FrameHeader);
        CloseSelectedFile();
        if ((ReadSize != sizeof(SBC_FRAME_HEADER)) || EFI_ERROR(CheckSbcFrameHeader(&FrameHeader))) {
            continue;
        }
        gBS->AllocatePool(EfiBootServicesData, sizeof(CHAR16) * (NameLength + 1), (VOID**)&gFileNameList[gFileCount]);
        StrCpy (gFileNameList[gFileCount++], gFileNamePtr);
    } while(CurLink != EndLink);

    gShellProtocol->CloseFile (DirHandle);
    *FileCount = gFileCount;

    return gFileNameList;
}

/**
   SelectSbcFile, read selected file to a temporary buffer.

   @param  Index       Select Index.

   @retval EFI_STATUS  SelectSbcFile result
 **/
EFI_STATUS
SelectSbcFile (IN UINT8 Index)
{
    if (Index > gFileCount) {
        return EFI_INVALID_PARAMETER;
    }
    StrCpy(gFileNamePtr, gFileNameList[Index]);

    return EFI_SUCCESS;
}

/**
   ShowFilesList

   @param  None
 **/
VOID
ShowFilesList (VOID)
{
    UINT8 Index = 0;
    Print(L"\n[Current Folder File List]\n");
    for (Index = 0; Index < gFileCount; Index++) {
        Print(L"#%03d %s", Index, gFileNameList[Index]);
        Print(L"\n");
    }
    return;
}
