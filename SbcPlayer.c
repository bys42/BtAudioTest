/** @file
    SBC Player source.

 **/

#include <Guid/MdeModuleHii.h>
#include <Library/HiiLib.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiString.h>
#include "SbcPlayer.h"
#include "VfrDef.h"

extern UINT8 SbcPlayerUiBin[];
extern UINT8 SbcPlayerStrings[];

#define STR_BUFFER_SZIE (128)

#define REPEAT_MODE_NONE   (0)
#define REPEAT_MODE_SINGLE (1)
#define REPEAT_MODE_ALL    (2)
#define REPEAT_MODE_COUNT  (3)

EFI_HII_HANDLE gHiiHandle;
EFI_GUID       gSbcPlayerFormGuid = FORM_GUID_SBC_PLAYER;
UINT8          FileListCount = 0;
UINT8          FileSelection = 0;
UINT8          RepeatMode = 0;

EFI_STATUS
CreateOption (
    UINT16 VarOffset,
    EFI_QUESTION_ID QuestionId,
    EFI_STRING_ID PromptId,
    EFI_STRING_ID HelpId,
    UINT16 StartLabelId,
    UINT16 EndLabelId,
    UINT16 *TokenList,
    UINT8 ListCount
    )
{
    VOID               *StartOpCodeHandle = HiiAllocateOpCodeHandle ();;
    VOID               *EndOpCodeHandle = HiiAllocateOpCodeHandle ();;
    VOID               *OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();;
    VOID               *DefaultOpCodeHandle = HiiAllocateOpCodeHandle ();;
    EFI_IFR_GUID_LABEL *StartLabel;
    EFI_IFR_GUID_LABEL *EndLabel;
    UINT16             ListIndex;

    if ((StartOpCodeHandle == NULL) || (EndOpCodeHandle == NULL) || (OptionsOpCodeHandle == NULL) || (DefaultOpCodeHandle == NULL)) {
        goto CreateOptionEnd;
    }

    StartLabel = (EFI_IFR_GUID_LABEL*) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
    StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
    StartLabel->Number = StartLabelId;

    EndLabel = (EFI_IFR_GUID_LABEL*) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
    EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
    EndLabel->Number = EndLabelId;

    for (ListIndex = 0; ListIndex < ListCount; ListIndex++) {
        HiiCreateOneOfOptionOpCode(
            OptionsOpCodeHandle,
            *TokenList++,
            0,
            EFI_IFR_NUMERIC_SIZE_1,
            ListIndex
            );
    }

    HiiCreateDefaultOpCode (
        DefaultOpCodeHandle,
        EFI_HII_DEFAULT_CLASS_STANDARD,
        EFI_IFR_NUMERIC_SIZE_1,
        0
        );

    HiiCreateOneOfOpCode (
        StartOpCodeHandle,
        QuestionId,
        VAR_ID_SELECTION,
        VarOffset,
        PromptId,
        HelpId,
        EFI_IFR_FLAG_CALLBACK,
        EFI_IFR_NUMERIC_SIZE_1,
        OptionsOpCodeHandle,
        DefaultOpCodeHandle
        );

    HiiUpdateForm (
        gHiiHandle,
        &gSbcPlayerFormGuid,
        FORM_ID_SBC_PLAYER,
        StartOpCodeHandle,
        EndOpCodeHandle
        );

CreateOptionEnd:
    if (StartOpCodeHandle != NULL) {
        HiiFreeOpCodeHandle(StartOpCodeHandle);
    }
    if (EndOpCodeHandle != NULL) {
        HiiFreeOpCodeHandle(EndOpCodeHandle);
    }
    if (OptionsOpCodeHandle != NULL) {
        HiiFreeOpCodeHandle(OptionsOpCodeHandle);
    }
    if (DefaultOpCodeHandle != NULL) {
        HiiFreeOpCodeHandle(DefaultOpCodeHandle);
    }

    return EFI_SUCCESS;
}

EFI_STATUS
CreateFileOption(VOID)
{
    CHAR16 **FileList;
    UINT16 *TokenList;
    UINT8  Index;

    FileList = CreateSbcFileList(&FileListCount);
    if (FileList == NULL || FileListCount == 0) {
        return EFI_NOT_FOUND;
    }

    FileListCount++; // add a not selected option
    gBS->AllocatePool(EfiBootServicesData, sizeof(UINT16) * FileListCount, (VOID**)&TokenList);

    TokenList[0] = STRING_TOKEN(STR_NOT_SELECTED_TEXT);
    for (Index = 1; Index < FileListCount; Index++) {
        TokenList[Index] = HiiSetString (gHiiHandle, 0, FileList[Index - 1], NULL);
    }

    CreateOption (
        OFFSET_OF(VAR_STRUCT_SELECTION, File),
        QUESTION_ID_FILE_SELECTION,
        STRING_TOKEN(STR_CURRENT_FILE_TEXT),
        STRING_TOKEN(STR_CURRENT_FILE_HELP_TEXT),
        LABEL_FILE_SELECTION_START,
        LABEL_FILE_SELECTION_END,
        TokenList,
        FileListCount
        );

    gBS->FreePool(TokenList);

    return EFI_SUCCESS;
}

EFI_STATUS
CreateRemoteBtOption(VOID)
{
    CHAR16        **DeviceList;
    UINT8         DeviceCount = 0;
    UINT16        *TokenList;
    UINT8         Index;
    EFI_INPUT_KEY Key;

    DeviceList = GetRemoteBtList(&DeviceCount);
    if (DeviceList == NULL) {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Inquiry Result: Not Found", NULL);
        return EFI_NOT_FOUND;
    }
    CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Inquiry Complete", NULL);

    DeviceCount++; // add a not selected option
    gBS->AllocatePool(EfiBootServicesData, sizeof(UINT16) * DeviceCount, (VOID**)&TokenList);
    TokenList[0] = STRING_TOKEN(STR_NOT_SELECTED_TEXT);
    for (Index = 1; Index < DeviceCount; Index++) {
        TokenList[Index] = HiiSetString (gHiiHandle, 0, DeviceList[Index - 1], NULL);
    }

    CreateOption (
        OFFSET_OF(VAR_STRUCT_SELECTION, Device),
        QUESTION_ID_DEVICE_SELECTION,
        STRING_TOKEN(STR_REMOTE_DEVICE_TEXT),
        STRING_TOKEN(STR_REMOTE_DEVICE_HELP_TEXT),
        LABEL_DEVICE_SELECTION_START,
        LABEL_DEVICE_SELECTION_END,
        TokenList,
        DeviceCount
        );
    gBS->FreePool(TokenList);

    return EFI_SUCCESS;
}

VOID
PlayStream (VOID)
{
    switch(RepeatMode) {
    case REPEAT_MODE_NONE:
        SendStreamFile(FALSE);
        break;
    case REPEAT_MODE_SINGLE:
        SendStreamFile(TRUE);
        break;
    case REPEAT_MODE_ALL:
        while (SendStreamFile(FALSE) == EFI_SUCCESS) {
            do {
                if (++FileSelection == (FileListCount - 1)) {
                    FileSelection = 0;
                }
            } while (EFI_ERROR(SelectSbcFile(FileSelection)));
        }
        break;
    default:
        break;
    }
    return;
}

EFI_STATUS
Callback (
    IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
    IN EFI_BROWSER_ACTION Action,
    IN EFI_QUESTION_ID QuestionId,
    IN UINT8 Type,
    IN OUT EFI_IFR_TYPE_VALUE *Value,
    OUT EFI_BROWSER_ACTION_REQUEST *ActionRequest
    )
{
    if (((Value == NULL) && (Action != EFI_BROWSER_ACTION_FORM_OPEN) && (Action != EFI_BROWSER_ACTION_FORM_CLOSE)) ||
        (ActionRequest == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    if (Action == EFI_BROWSER_ACTION_CHANGING) {
        switch (QuestionId)
        {
        case QUESTION_ID_INUIRY_START:
            CreateRemoteBtOption();
            break;
        case QUESTION_ID_DEVICE_SELECTION:
            DisconnectDevice();
            if (Value->u8) {
                EFI_INPUT_KEY Key;
                UINT16        ConnectResult[100];
                UnicodeSPrint(ConnectResult, 100, L"Connect Status: %r", ConnectDevice(Value->u8 - 1));
                CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, ConnectResult, NULL);
            }
            break;
        case QUESTION_ID_FILE_SELECTION:
            if (Value->u8) {
                FileSelection = Value->u8 - 1;
                SelectSbcFile(FileSelection);
            }
            break;
        case QUESTION_ID_REPEAT_MODE:
            RepeatMode = Value->u8;
            break;
        case QUESTION_ID_START_PLAY: //active iff (FileSelection != 0) && (DevSelection != 0)
            PlayStream();
            break;
        default:
            break;
        }
    }
    return EFI_SUCCESS;
}

EFI_STATUS
FormBrowserInit(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
    )
{
    EFI_STATUS                     Status;
    EFI_HII_CONFIG_ACCESS_PROTOCOL ConfigAccess = {NULL, NULL, Callback};
    EFI_HANDLE                     AppHandle = NULL;
    EFI_FORM_BROWSER2_PROTOCOL     *FormBrowser2;

    Status = BtSetup ();
    if (EFI_ERROR(Status)) {
        Print (L"BtSetup: %r\n", Status);
        return Status;
    }

    Status = gBS->LocateProtocol(&gEfiFormBrowser2ProtocolGuid, NULL, (VOID**) &FormBrowser2);
    if (EFI_ERROR (Status)) {
        Print (L"Locate gEfiFormBrowser2ProtocolGuid: %r\n", Status);
        return Status;
    }

    Status = gBS->InstallProtocolInterface(&AppHandle, &gEfiHiiConfigAccessProtocolGuid, EFI_NATIVE_INTERFACE, &ConfigAccess);
    if (EFI_ERROR (Status)) {
        Print (L"Install gEfiHiiConfigAccessProtocolGuid: %r\n", Status);
        return Status;
    }

    gHiiHandle = HiiAddPackages(&gSbcPlayerFormGuid, AppHandle, SbcPlayerStrings, SbcPlayerUiBin, NULL);
    if (gHiiHandle == NULL) {
        Print (L"HiiAddPackages gSbcPlayerFormGuid: %r\n", EFI_OUT_OF_RESOURCES);
        return EFI_OUT_OF_RESOURCES;
    }

    Status = CreateFileOption();
    if (EFI_ERROR (Status)) {
        Print (L"CreateFileOption: %r\n", Status);
        return Status;
    }

    Status = FormBrowser2->SendForm(FormBrowser2, &gHiiHandle, 1, &gSbcPlayerFormGuid, FORM_ID_SBC_PLAYER, NULL, NULL);

    HiiRemovePackages (gHiiHandle);
    DisconnectDevice();
    CleanRemoteBtListBuffer();
    CleanFileListBuffer();
    gST->ConOut->ClearScreen (gST->ConOut);

    return Status;
}
