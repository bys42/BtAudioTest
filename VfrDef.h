/** @file
    Vfr defines.

 **/

#ifndef _VFR_DEF_H_
#define _VFR_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FORM_GUID_SBC_PLAYER { 0x8bc90384, 0x540b, 0x472f, {0xa5, 0x71, 0xd2, 0x9e, 0x8, 0x17, 0xdf, 0x48}}
#define VAR_GUID_SELECTION   { 0x11e9a5c0, 0xd051, 0x4db2, {0xac, 0x82, 0x64, 0xbe, 0x9f, 0x28, 0x84, 0x4a}}

#define FORM_ID_SBC_PLAYER 1

#define QUESTION_ID_INUIRY_START     0x0042
#define QUESTION_ID_DEVICE_SELECTION 0x0043
#define QUESTION_ID_FILE_SELECTION   0x0044
#define QUESTION_ID_REPEAT_MODE      0x0045
#define QUESTION_ID_START_PLAY       0x0046

#define LABEL_DEVICE_SELECTION_START  0x0142
#define LABEL_DEVICE_SELECTION_END    0x0143
#define LABEL_FILE_SELECTION_START    0x0144
#define LABEL_FILE_SELECTION_END      0x0145

#define VAR_ID_SELECTION 0x0242

#pragma pack(1)
typedef struct {
    UINT8 Device;
    UINT8 File;
    UINT8 RepeatMode;
} VAR_STRUCT_SELECTION;
#pragma pack()

/****** DO NOT WRITE BELOW THIS LINE *******/
#ifdef __cplusplus
}
#endif

#endif
