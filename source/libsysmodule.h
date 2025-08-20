/*
	Vita Development Suite Libraries
*/

#ifndef _PSP2_SYSMODULE_H_
#define _PSP2_SYSMODULE_H_

#include_next <libsysmodule.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Available modules ID for ::sceSysmoduleLoadModule */
typedef enum SceSysmoduleModuleId {
	SCE_SYSMODULE_TRIGGER_UTIL          = 0x0020, //!< TriggerUtil module
	SCE_SYSMODULE_VIDEO_EXPORT          = 0x0032, //!< Video Export module
	SCE_SYSMODULE_NOTIFICATION_UTIL     = 0x0033, //!< NotificationUtil module
	SCE_SYSMODULE_BG_APP_UTIL           = 0x0034, //!< BgAppUtil module
	SCE_SYSMODULE_INCOMING_DIALOG       = 0x0035, //!< Incoming Dialog module
	SCE_SYSMODULE_IPMI                  = 0x0036, //!< IPMI module
	SCE_SYSMODULE_AUDIOCODEC            = 0x0037, //!< AudioCodec module
	SCE_SYSMODULE_MARLIN                = 0x003A, //!< Marlin module
	SCE_SYSMODULE_MARLIN_DOWNLOADER     = 0x003B, //!< Marlin Downloader module
	SCE_SYSMODULE_MARLIN_APP_LIB        = 0x003C, //!< Marlin AppLib module
	SCE_SYSMODULE_TELEPHONY_UTIL        = 0x003D, //!< TelephonyUtil module
	SCE_SYSMODULE_DTCP_IP               = 0x0044, //!< DTCP/IP module
	SCE_SYSMODULE_VIDEO_SEARCH_EMPR     = 0x0045, //!< VideoSearch module
	SCE_SYSMODULE_BEISOBMF              = 0x0047, //!< BEISOBMF module
	SCE_SYSMODULE_BEMP2SYS              = 0x0048, //!< BEMP2SYS module
	SCE_SYSMODULE_MAIL_API              = 0x004E, //!< Mail Api module
	SCE_SYSMODULE_AVCDEC                = 0x0054, //!< AVCDec module
} SceSysmoduleModuleId;

/** Available internal modules ID for ::sceSysmoduleLoadModuleInternal */
typedef enum SceSysmoduleInternalModuleId {
	SCE_SYSMODULE_INTERNAL_JPEG_ENC_ARM               = 0x80000001, //!< sceJpegEncArm module
	SCE_SYSMODULE_INTERNAL_AUDIOCODEC                 = 0x80000002, //!< AudioCodec module
	SCE_SYSMODULE_INTERNAL_JPEG_ARM                   = 0x80000003, //!< sceJpegArm module
	SCE_SYSMODULE_INTERNAL_G729                       = 0x80000004, //!< G729 module
	SCE_SYSMODULE_INTERNAL_BXCE                       = 0x80000005, //!< BXCE module
	SCE_SYSMODULE_INTERNAL_INI_FILE_PROCESSOR         = 0x80000006, //!< INI File Processor module
	SCE_SYSMODULE_INTERNAL_NP_ACTIVITY_NET            = 0x80000007, //!< NpActivityNet module
	SCE_SYSMODULE_INTERNAL_PAF                        = 0x80000008, //!< Paf module
	SCE_SYSMODULE_INTERNAL_SQLITE_VSH                 = 0x80000009, //!< sceSqliteVsh module
	SCE_SYSMODULE_INTERNAL_DBUTIL                     = 0x8000000A, //!< DBUtil module
	SCE_SYSMODULE_INTERNAL_ACTIVITY_DB                = 0x8000000B, //!< ActivityDB module
	SCE_SYSMODULE_INTERNAL_COMMON_GUI_DIALOG          = 0x8000000C, //!< Common Gui Dialog module
	SCE_SYSMODULE_INTERNAL_STORE_CHECKOUT             = 0x8000000D, //!< Store Checkout module
	SCE_SYSMODULE_INTERNAL_IME_DIALOG                 = 0x8000000E, //!< Ime Dialog module
	SCE_SYSMODULE_INTERNAL_PHOTO_IMPORT_DIALOG        = 0x8000000F, //!< Photo Import Dialog module
	SCE_SYSMODULE_INTERNAL_PHOTO_REVIEW_DIALOG        = 0x80000010, //!< Photo Review Dialog module
	SCE_SYSMODULE_INTERNAL_CHECKOUT_DIALOG            = 0x80000011, //!< Checkout Dialog module
	SCE_SYSMODULE_INTERNAL_COMMON_DIALOG_MAIN         = 0x80000012, //!< Common Dialog Main module
	SCE_SYSMODULE_INTERNAL_MSG_DIALOG                 = 0x80000013, //!< sceMsgDialog module
	SCE_SYSMODULE_INTERNAL_NET_CHECK_DIALOG           = 0x80000014, //!< NetCheck Dialog module
	SCE_SYSMODULE_INTERNAL_SAVEDATA_DIALOG            = 0x80000015, //!< SaveData Dialog module
	SCE_SYSMODULE_INTERNAL_NP_MESSAGE_DIALOG          = 0x80000016, //!< NpMessage Dialog module
	SCE_SYSMODULE_INTERNAL_TROPHY_SETUP_DIALOG        = 0x80000017, //!< Trophy Setup Dialog module
	SCE_SYSMODULE_INTERNAL_FRIEND_LIST_DIALOG         = 0x80000018, //!< FriendList Dialog module
	SCE_SYSMODULE_INTERNAL_NEAR_PROFILE               = 0x8000001A, //!< Near Profile module
	SCE_SYSMODULE_INTERNAL_NP_FRIEND_PRIVACY_LEVEL    = 0x8000001B, //!< NpFriend Privacy Level module
	SCE_SYSMODULE_INTERNAL_NP_COMMERCE2               = 0x8000001D, //!< NpCommerce2 module
	SCE_SYSMODULE_INTERNAL_NP_KDC                     = 0x8000001E, //!< NpKdc module
	SCE_SYSMODULE_INTERNAL_MUSIC_EXPORT               = 0x8000001F, //!< Music Export module
	SCE_SYSMODULE_INTERNAL_VIDEO_EXPORT               = 0x80000020, //!< Video Export module
	SCE_SYSMODULE_INTERNAL_NP_MESSAGE_DIALOG_IMPL     = 0x80000021, //!< NpMessageDlg module
	SCE_SYSMODULE_INTERNAL_NP_MESSAGE_CONTACTS        = 0x80000022, //!< NpMessageContacts module
	SCE_SYSMODULE_INTERNAL_DB_RECOVERY_UTILITY        = 0x80000023, //!< DBRecoveryUtil module
	SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL              = 0x80000024, //!< scePromoterUtil module
	SCE_SYSMODULE_INTERNAL_PARTY_MEMBER_LIST          = 0x80000026, //!< Party Member List module
	SCE_SYSMODULE_INTERNAL_ULT                        = 0x80000025, //!< Ult module
	SCE_SYSMODULE_INTERNAL_DRM_PSM_KDC                = 0x80000027, //!< DRM PSM KDC module
	SCE_SYSMODULE_INTERNAL_LOCATION_INTERNAL          = 0x80000028, //!< sceLocation module
	SCE_SYSMODULE_INTERNAL_LOCATION_FACTORY           = 0x80000029  //!< Location Factory module
} SceSysmoduleInternalModuleId;

/**
 * Load an internal module.
 *
 * @param[in] id - Module ID to load.
 *
 * @return 0 on success, <0 on error.
 */
int sceSysmoduleLoadModuleInternal(SceUInt32 id);

/**
 * Unload an internal module.
 *
 * @param[in] id - Module ID to unload.
 *
 * @return 0 on success, <0 on error.
 */
int sceSysmoduleUnloadModuleInternal(SceUInt32 id);

/**
 * Check if an internal module is loaded.
 *
 * @param[in] id - Module ID to check.
 *
 * @return 0 if loaded, <0 otherwise.
 */
int sceSysmoduleIsLoadedInternal(SceUInt32 id);

/**
 * Load an internal module with custom arguments.
 *
 * @param[in] id - Module ID to check.
 * @param[in] args - Size of passed arguments.
 * @param[in] argp - Pointer to arguments to pass.
 * @param[in] unk - Unknown value.
 *
 * @return 0 on success, <0 otherwise.
 */
int sceSysmoduleLoadModuleInternalWithArg(SceUInt32 id, SceSize args, void *argp, void *unk);

/**
 * Unload an internal module with custom arguments.
 *
 * @param[in] id - Module ID to check.
 * @param[in] args - Size of passed arguments.
 * @param[in] argp - Pointer to arguments to pass.
 * @param[in] unk - Unknown value.
 *
 * @return 0 on success, <0 otherwise.
 */
int sceSysmoduleUnloadModuleInternalWithArg(SceUInt32 id, SceSize args, void *argp, void *unk);

#ifdef __cplusplus
}
#endif

#endif /* _PSP2_SYSMODULE_H_ */