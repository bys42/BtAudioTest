/** @file
    SBC player config.

 **/

#ifndef _SBC_PLAYER_CONFIG_H_
#define _SBC_PLAYER_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//
//  Configurations
//
#define WATI_TIME_OUT (100)
#define NO_BTTEST_DBG (TRUE)
#define MAX_FILENAME_LENGTH (127)
#define MAX_FILE_COUNT (128)
#define USB_TRANS_TIMEOUT_ACL_SEND (0x30)
#define USB_TRANS_TIMEOUT_ACL_READ (0x80)
#define USB_TRANS_TIMEOUT_HCI_COMMAND (0x30)
#define USB_TRANS_TIMEOUT_HCI_EVENT (0x80)
#define READ_TRIAL_COUNT (100)
#define DEFAULT_INQUIRY_LAP (0x9E8B33)
#define DEFAULT_INQUIRY_LENGTH (4) // 4 * 1.28s
#define MAX_HCI_CMD_EVT_BUFFER (256)
#define MAX_ACL_BUFFER (310) // Sould use HCI Read Buffer Size Command
#define INQUIRT_RESULT_MAX (10)
#define DEFAULT_AVDTP_SIGNAL_CHANNEL_ID (0x0042)
#define DEFAULT_AVDTP_STREAM_CHANNEL_ID (0x0142)
#define DEFAULT_BITPOOL_MIN (16)
#define DEFAULT_BITPOOL_MAX (53)
#define DEFAULT_SBC_SETTING (0x35BD)
#define SBCPACK_SKIP_COUNT (492) // 10s for 44100Hz, subband 8, block 16
#define SKIP_TIME_IN_SECOND (10) // 10s for 44100Hz, subband 8, block 16
#define ACL_COUNT_PER_SBC_PACKET (2)
#define BDADDR_NAME_STRING L"%02x:%02x:%02x:%02x:%02x:%02x" // %02x:%02x:%02x:%02x:%02x:%02x
#define BDADDR_NAME_LENGTH (18) // %02x:%02x:%02x:%02x:%02x:%02x

/****** DO NOT WRITE BELOW THIS LINE *******/
#ifdef __cplusplus
}
#endif

#endif
