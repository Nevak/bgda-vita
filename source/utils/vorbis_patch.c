/* vorbis_patch.c -- vorbis redirection
 *
 * Copyright (C) 2022 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
//#include <vorbis/vorbisfile.h>
#include <so_util/so_util.h>
#include <psp2/kernel/processmgr.h>
#include "logger.h"
#include "utils/macros.h"
#include "utils/oggstream_types.h"
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <fcntl.h>      // For file access modes (O_RDONLY)
//#include <vorbis/prof.h>
#ifdef PROFILER_ENABLED
#include <utils/prof.h>
#include <libperf.h>
#endif

int g_curLanguage;
extern so_module so_mod;
extern so_module so_mod_libxmv;

// Per-frame statistics for ov_read profiling
static int g_ovReadCallCount = 0;
static float g_ovReadTotalTimeMs = 0.0f;
static long g_ovReadTotalBytesRequested = 0;
static long g_ovReadTotalBytesReturned = 0;
static int g_ovReadMinBufferSize = 999999;
static int g_ovReadMaxBufferSize = 0;
static int g_ovReadInternalCallCount = 0;  // Track internal batching
static int g_sndFrameNumber = 0;  // Track which frame we're on


char* g_currentWorldName;
char* g_SoundFilePath;
SoundSystem* g_soundSystemBasePtr;

_Static_assert(offsetof(SoundChannel, streamCount) == 0x2d8,
               "`state` is not at the right offset - fix the struct or add packed!");


so_hook ov_read_hook;
long ov_read_profiled(void *vf, char *buffer, int length,
                      int bigendianp, int word, int sgned, int *bitstream) {

	// if (g_ovReadCallCount >= MAX_OV_READ_PER_FRAME) {
    //     return 0;  // Pretend we hit end of stream temporarily
    // }

	//SCE_PERF_ARM_PMON_STOP_ALL();
	//sceRazorCpuPushMarkerWithHud("ov_read", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
	//SCE_PERF_ARM_PMON_START_ALL();

	// // Also limit decode size per call
    // const int MAX_CHUNK = 0x9000;  // 36KB per call
    // if (length > MAX_CHUNK) {
    //     length = MAX_CHUNK;
    // }
    

	long result = SO_CONTINUE(long, ov_read_hook, vf, buffer, length, bigendianp, word, sgned, bitstream);
	
	//SCE_PERF_ARM_PMON_STOP_ALL();
	//sceRazorCpuPopMarker();


	//logv_error("ov_read: vf=%p, len=%d, totalRead=%ld, stream = %d: ",vf, length, result, *bitstream);


	return result;
}

// Hook for SND_Frame - reports ov_read stats per frame
so_hook snd_frame_hook;
void snd_frame_profiled(void) {
	//return;

	//g_sndFrameNumber++;

	// // Report stats if ov_read was called this frame
	// if (g_ovReadCallCount > 0) {
	// 	float batchRatio = (float)g_ovReadInternalCallCount / (float)g_ovReadCallCount;
	// 	int fillPercent = (int)((100.0f * g_ovReadTotalBytesReturned) / g_ovReadTotalBytesRequested);
	// 	logv_error("SND_Frame #%d: ov_read x%d (batched %d internal, %.1fx), %.1f ms (avg %.1f ms), "
	// 	           "buf %d-%d bytes, req %ld, ret %ld (%d filled)",
	// 	           g_sndFrameNumber, g_ovReadCallCount, g_ovReadInternalCallCount, batchRatio,
	// 	           g_ovReadTotalTimeMs, g_ovReadTotalTimeMs / g_ovReadCallCount,
	// 	           g_ovReadMinBufferSize, g_ovReadMaxBufferSize,
	// 	           g_ovReadTotalBytesRequested, g_ovReadTotalBytesReturned,
	// 	           fillPercent);
	// }

	// // Reset counters for next frame
	// g_ovReadCallCount = 0;
	// g_ovReadTotalTimeMs = 0.0f;
	// g_ovReadTotalBytesRequested = 0;
	// g_ovReadTotalBytesReturned = 0;
	// g_ovReadMinBufferSize = 999999;
	// g_ovReadMaxBufferSize = 0;
	// g_ovReadInternalCallCount = 0;

	// Call original SND_Frame
	//sceRazorCpuPushMarkerWithHud("SND_Frame", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
	//Vorbis_Profiler_Reset();

	// store current time
	uint64_t startTime = sceKernelGetProcessTimeLow();
	SO_CONTINUE(void*, snd_frame_hook);


	// calculate elapsed time
	uint64_t endTime = sceKernelGetProcessTimeLow();

	// print elapsed time in milliseconds
	float elapsedMs = (endTime - startTime) / 1000.0f;
	// print only if elapsed time is greater than 1 ms to reduce log spam
	if (elapsedMs > 1.0f) {
		logv_error("SND_Frame #%d: %.1f ms", g_sndFrameNumber, elapsedMs);
	}

	//Vorbis_Profiler_PrintSamples(0,0);
	//log_error("=========================================");
	//sceRazorCpuPopMarker();
}

so_hook ov_read_callback_hook;
void ov_read_callback(void *param_1, int param_2,int param_3,void *param_4)
{
	//sceRazorCpuPushMarkerWithHud("ov_read_callback", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
	SO_CONTINUE(void*, ov_read_callback_hook, param_1, param_2, param_3, param_4);
	//sceRazorCpuPopMarker();
}

so_hook snd_start_stream_hook;
void snd_start_stream(int channelId, char *path, int streamFlags, int startSample, uint32_t len)
{
	// Get the caller address
	uintptr_t caller = (uintptr_t)__builtin_return_address(0);
	logv_debug("[%p] snd_start_stream: channelId=%d, path=%s, streamFlags=0x%X, startSample=0x%X, len=0x%X", caller, channelId, path, streamFlags, startSample, len);


	SO_CONTINUE(void*, snd_start_stream_hook, channelId, path, streamFlags, startSample, len);

    SoundSystem g_soundSystemBase = *g_soundSystemBasePtr;
    
    int streamCount0 = g_soundSystemBasePtr->channels[0].streamCount;
	int streamCount1 = g_soundSystemBasePtr->channels[1].streamCount;
	int streamCount2 = g_soundSystemBasePtr->channels[2].streamCount;

    int streamCountPtr = g_soundSystemBasePtr->channels[channelId].streamCount;
    int streamSlotIndex = streamCountPtr - 1;
	if (streamSlotIndex < 0)
	{
		streamSlotIndex = 0;
	}
	logv_debug("streamSlotIndex=%d", streamSlotIndex);
    
    OggStream* oggStreamPtr = g_soundSystemBase.channels[channelId].streamSlots[streamSlotIndex].oggStream;
	int endSample = g_soundSystemBase.channels[channelId].streamSlots[streamSlotIndex].endSample;
    logv_debug("  oggStreamPtr=%p", oggStreamPtr);
    logv_debug("  endSample=%d", endSample);
	if (oggStreamPtr != 0)
	{
	 	logv_debug("  isOgg=%hhx", oggStreamPtr->isOggS);
	}


	logv_debug("streamCount[0]=%d", streamCount0);
	logv_debug("streamCount[1]=%d", streamCount1);
	logv_debug("streamCount[2]=%d", streamCount2);
	logv_debug("[%p] RETURNED snd_start_stream: channelId=%d, path=%s, streamFlags=0x%X, startSample=0x%X, len=0x%X", caller, channelId, path, streamFlags, startSample, len);
}

so_hook snd_start_music_hook;
void snd_start_music(char *path,int param)
{
	logv_error("snd_start_music: path=%s, param=0x%X", path, param);

	SO_CONTINUE(void*, snd_start_music_hook, path, param);
}

typedef struct __attribute__((__packed__)) dialog_dir
{
	char name[64];
	int startAt;
	int unk;
	int len;
}dialog_dir;
_Static_assert(sizeof(struct dialog_dir) == 0x4C, "dialog_dir struct size should be 0x4C bytes");

typedef struct __attribute__((__packed__)) {
	char name[48];
	void* data;
	int unk1;
	int unk2;
	int len;
	int unk3;
}lmp_entry;
_Static_assert(offsetof(lmp_entry, len) == 0x3C,
               "`len` is not at the right offset");

so_hook lump_query_hook;

lmp_entry* lump_query(char* query)
{
	lmp_entry* res = SO_CONTINUE(lmp_entry*, lump_query_hook, query);
	return res;
}

so_hook lump_find_resource_hook;
lmp_entry* lump_find_resource(char *path, char *param)
{
	// Redirect spanish dialog to default dialog since spanish dialog file is bugged even in the android version
	if (strcmp(param, "s_dialog.bin") == 0)
	{
		//return SO_CONTINUE(lmp_entry*, lump_find_resource_hook, path, "dialog.bin");
	}

	lmp_entry* res = SO_CONTINUE(lmp_entry*, lump_find_resource_hook,  path, param);
	//logv_error("[%p] lump_find_resource: path=%s, param=%s res=0x%X", caller, path, param, res);

	return res;
}

so_hook snd_get_dialog_filename_hook;
char* snd_get_dialog_filename(uint8_t loc)
{
	//logv_error("1) g_SoundFilePath = %s, loc=0x%X", g_SoundFilePath, loc);
	char* res = SO_CONTINUE(char*, snd_get_dialog_filename_hook, loc);

	//logv_error("snd_get_dialog_filename (%x) ret=%s", loc, res);
	//logv_error("2) g_SoundFilePath = %s", g_SoundFilePath);
	return res;
}

int get_dialog_file_size()
{
    char realPath[256]; 
    snprintf(realPath, sizeof(realPath), "ux0:data/bgda/assets/res/%s", snd_get_dialog_filename(0));

    int fd = open_soloader(realPath, O_RDONLY);
    if (fd == -1) {
        logv_error("Error: open failed: %s", realPath); 
        return -1; 
    }
    
    off_t size = lseek_delegate(fd, 0, SEEK_END);
    if (size == (off_t)-1) {
        log_error("Error: lseek failed");
        close_soloader(fd); 
        return -1;
    }
    
    close_soloader(fd);
    return (int)size;
}

so_hook snd_get_dialog_dir_hook;

dialog_dir* snd_get_dialog_dir()
{
	uintptr_t caller = (uintptr_t)__builtin_return_address(0);
	logv_debug("[%p]snd_get_dialog_dir called. g_curLanguage=(0x%X)", caller, g_curLanguage);

	dialog_dir* res = SO_CONTINUE(dialog_dir*, snd_get_dialog_dir_hook);

	// logv_debug("[%p]snd_get_dialog_dir returned=0x%X",caller, res);

	// In the android version of the game many dialog entries have incorrect lengths (len=0)
	// This seems to glitch the audio and crash the game with OOM after running for a while in the vita
	// So we fix the lengths in the table by calculating the diff from the next entry's start offset
	// For the last entry we calculate from the total file size (this may not be accurate but it's better than 0)

	// Seems like .vat files are just a bunch of concatenated ogg files pointed at by offset/lenght from the dialog.bin tables from (dialog dir)
	int totalVatFileSize = get_dialog_file_size();
	logv_debug("totalVatFileSize=%d - 0x%X", totalVatFileSize, totalVatFileSize);

	// First pass: count entries
	dialog_dir* entry = res;
	int count = 0;
	while (entry->name[0] != '\0') {
		entry++;
		count++;
	}
	
	logv_debug("Found %d dialog entries for world %s", count, g_currentWorldName);

	// Second pass: fix lengths by calculating from next entry's start
	entry = res;
	for (int i = 0; i < count; i++) {
		//logv_debug("entry[%d]: name=%s, startAt=0x%X, len=0x%X (original)", i, entry->name, entry->startAt, entry->len);
		if (entry->len == 0) {
		 	int calculatedLen;
			
		 	if (i < count - 1) {
		 		// Calculate length from next entry's start
		 		dialog_dir* next_entry = entry + 1;
		 		calculatedLen = next_entry->startAt - entry->startAt;
		 	} else {
		 		// Last entry - calculate length from total file size
		 		calculatedLen = totalVatFileSize - entry->startAt;
		 	}
		
		 	// Fix the len field
		 	entry->len = calculatedLen;
		
		 	//logv_debug("entry[%d]: name=%s, startAt=0x%X, len=0x%X (fixed)", i, entry->name, entry->startAt, entry->len);
		 }
		entry++;
	}

	return res;
}

so_hook ram_fs_open_hook;
uint32_t ram_fs_open(void * thisptr, char* name, uint32_t*param)
{
	uint32_t res = SO_CONTINUE(uint32_t, ram_fs_open_hook, thisptr, name, param);

	logv_error("ram_fs_open called: name=%s, param=%p, res=0x%X", name, param, res);

	return res;
}

so_hook create_file_a_hook;
int create_file_a(char * file_name, int flags)
{
	int res = SO_CONTINUE(int, create_file_a_hook, file_name, flags);
	logv_error("create_file_a (%s, 0x%x) = 0x%x", file_name, flags, res);
	return res;
}

so_hook ov_pcm_total_hook;
uint64_t ov_pcm_total_local(void* pf, long x)
{
    uint64_t res = SO_CONTINUE(uint64_t, ov_pcm_total_hook, pf, x);
    logv_error("ov_pcm_total called(%p, %d)=%ld", pf, x, res);
    return res;
}

so_hook ov_info_hook;
vorbis_info *ov_info_local(void *vf, int link)
{
    vorbis_info* res = SO_CONTINUE(vorbis_info*, ov_info_hook, vf, link);

	if (res != 0)
	{
    	logv_error("ov_info_local called(%p, %d)=channels=%d", vf, link, res->channels);
		logv_error("ov_info_local called(%p, %d)=rate=%d", vf, link, res->rate);
	}
	else
		log_error("ov_info_local returned 0!");

    return res;
}

#define USE_VITASDK_VORBIS 1
void patch_vorbis(void)
{
#if USE_VITASDK_VORBIS

	log_error("patch_vorbis: Installing direct vitasdk vorbis symbol hooks...");

	// --- vorbisfile API ---
	hook_addr(so_symbol(&so_mod, "ov_clear"), (uintptr_t)ov_clear);

	hook_addr(so_symbol(&so_mod, "ov_open"), (uintptr_t)ov_open);
	hook_addr(so_symbol(&so_mod, "ov_open_callbacks"), (uintptr_t)ov_open_callbacks);
	hook_addr(so_symbol(&so_mod, "ov_fopen"), (uintptr_t)ov_fopen);
	hook_addr(so_symbol(&so_mod, "ov_test_open"), (uintptr_t)ov_test_open);
	hook_addr(so_symbol(&so_mod, "ov_test"), (uintptr_t)ov_test);
	hook_addr(so_symbol(&so_mod, "ov_test_callbacks"), (uintptr_t)ov_test_callbacks);
	hook_addr(so_symbol(&so_mod, "ov_bitrate"), (uintptr_t)ov_bitrate);
	hook_addr(so_symbol(&so_mod, "ov_bitrate_instant"), (uintptr_t)ov_bitrate_instant);
	hook_addr(so_symbol(&so_mod, "ov_streams"), (uintptr_t)ov_streams);
	hook_addr(so_symbol(&so_mod, "ov_seekable"), (uintptr_t)ov_seekable);
	hook_addr(so_symbol(&so_mod, "ov_serialnumber"), (uintptr_t)ov_serialnumber);
	hook_addr(so_symbol(&so_mod, "ov_raw_total"), (uintptr_t)ov_raw_total);
	hook_addr(so_symbol(&so_mod, "ov_pcm_total"), (uintptr_t)ov_pcm_total);
	hook_addr(so_symbol(&so_mod, "ov_time_total"), (uintptr_t)ov_time_total);
	hook_addr(so_symbol(&so_mod, "ov_raw_seek"), (uintptr_t)ov_raw_seek);
	hook_addr(so_symbol(&so_mod, "ov_pcm_seek"), (uintptr_t)ov_pcm_seek);
	hook_addr(so_symbol(&so_mod, "ov_pcm_seek_page"), (uintptr_t)ov_pcm_seek_page);
	hook_addr(so_symbol(&so_mod, "ov_time_seek"), (uintptr_t)ov_time_seek);
	hook_addr(so_symbol(&so_mod, "ov_time_seek_page"), (uintptr_t)ov_time_seek_page);
	hook_addr(so_symbol(&so_mod, "ov_raw_seek_lap"), (uintptr_t)ov_raw_seek_lap);
	hook_addr(so_symbol(&so_mod, "ov_pcm_seek_lap"), (uintptr_t)ov_pcm_seek_lap);
	hook_addr(so_symbol(&so_mod, "ov_pcm_seek_page_lap"), (uintptr_t)ov_pcm_seek_page_lap);
	hook_addr(so_symbol(&so_mod, "ov_time_seek_lap"), (uintptr_t)ov_time_seek_lap);
	hook_addr(so_symbol(&so_mod, "ov_time_seek_page_lap"), (uintptr_t)ov_time_seek_page_lap);
	hook_addr(so_symbol(&so_mod, "ov_raw_tell"), (uintptr_t)ov_raw_tell);
	hook_addr(so_symbol(&so_mod, "ov_pcm_tell"), (uintptr_t)ov_pcm_tell);
	hook_addr(so_symbol(&so_mod, "ov_time_tell"), (uintptr_t)ov_time_tell);
	hook_addr(so_symbol(&so_mod, "ov_info"), (uintptr_t)ov_info);
	hook_addr(so_symbol(&so_mod, "ov_comment"), (uintptr_t)ov_comment);
	hook_addr(so_symbol(&so_mod, "ov_read"), (uintptr_t)ov_read);
	hook_addr(so_symbol(&so_mod, "ov_read_float"), (uintptr_t)ov_read_float);
	hook_addr(so_symbol(&so_mod, "ov_crosslap"), (uintptr_t)ov_crosslap);
	hook_addr(so_symbol(&so_mod, "ov_halfrate"), (uintptr_t)ov_halfrate);
	hook_addr(so_symbol(&so_mod, "ov_halfrate_p"), (uintptr_t)ov_halfrate_p);

	// --- vorbis codec API ---
	hook_addr(so_symbol(&so_mod, "vorbis_info_init"), (uintptr_t)vorbis_info_init);
	hook_addr(so_symbol(&so_mod, "vorbis_info_clear"), (uintptr_t)vorbis_info_clear);
	hook_addr(so_symbol(&so_mod, "vorbis_info_blocksize"), (uintptr_t)vorbis_info_blocksize);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_init"), (uintptr_t)vorbis_comment_init);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_add"), (uintptr_t)vorbis_comment_add);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_add_tag"), (uintptr_t)vorbis_comment_add_tag);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_query"), (uintptr_t)vorbis_comment_query);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_query_count"), (uintptr_t)vorbis_comment_query_count);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_clear"), (uintptr_t)vorbis_comment_clear);
	hook_addr(so_symbol(&so_mod, "vorbis_block_init"), (uintptr_t)vorbis_block_init);
	hook_addr(so_symbol(&so_mod, "vorbis_block_clear"), (uintptr_t)vorbis_block_clear);
	hook_addr(so_symbol(&so_mod, "vorbis_dsp_clear"), (uintptr_t)vorbis_dsp_clear);
	hook_addr(so_symbol(&so_mod, "vorbis_granule_time"), (uintptr_t)vorbis_granule_time);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_idheader"), (uintptr_t)vorbis_synthesis_idheader);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_headerin"), (uintptr_t)vorbis_synthesis_headerin);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_init"), (uintptr_t)vorbis_synthesis_init);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_restart"), (uintptr_t)vorbis_synthesis_restart);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis"), (uintptr_t)vorbis_synthesis);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_trackonly"), (uintptr_t)vorbis_synthesis_trackonly);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_blockin"), (uintptr_t)vorbis_synthesis_blockin);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_pcmout"), (uintptr_t)vorbis_synthesis_pcmout);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_lapout"), (uintptr_t)vorbis_synthesis_lapout);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_read"), (uintptr_t)vorbis_synthesis_read);
	hook_addr(so_symbol(&so_mod, "vorbis_packet_blocksize"), (uintptr_t)vorbis_packet_blocksize);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_halfrate"), (uintptr_t)vorbis_synthesis_halfrate);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_halfrate_p"), (uintptr_t)vorbis_synthesis_halfrate_p);

	log_error("patch_vorbis: Vitasdk vorbis hooks installed!");
#endif
	//ov_read_hook = hook_addr(so_symbol(&so_mod, "ov_read"), (uintptr_t)ov_read_profiled);
	//lump_find_resource_hook = hook_addr(so_symbol(&so_mod, "_Z16lumpFindResourcePKcS0_"), (uintptr_t)lump_find_resource);
	
	// Disable this for now. It was meant to fix some background audio loops not playing,
	// but it's crashing the game in the final cinematic video. Will revisit
	//snd_get_dialog_dir_hook = hook_addr(so_symbol(&so_mod, "_Z16SND_GetDialogDirv"), (uintptr_t)snd_get_dialog_dir);
	
	//lump_query_hook = hook_addr(so_symbol(&so_mod, "_Z9lumpQueryPKc"), (uintptr_t)lump_query);
	snd_get_dialog_filename_hook = hook_addr(so_symbol(&so_mod, "_Z21SND_GetDialogFilenameb"), (uintptr_t)snd_get_dialog_filename);
	//snd_start_stream_hook = hook_addr(so_symbol(&so_mod, "_Z15SND_StartStreamiPKciii"), (uintptr_t)snd_start_stream);
	//snd_start_music_hook = hook_addr(so_symbol(&so_mod, "_Z14SND_StartMusicPKci"), (uintptr_t)snd_start_music);

	//ov_pcm_total_hook = hook_addr(so_symbol(&so_mod, "ov_pcm_total"), (uintptr_t)ov_pcm_total_local);
    //ov_info_hook = hook_addr(so_symbol(&so_mod, "ov_info"), (uintptr_t)ov_info_local);

	//ogg_stream_hook = hook_addr(so_symbol(&so_mod, "_ZN9OggStreamC2EPKcRiS2_i"), (uintptr_t)ogg_stream_patched);

	g_currentWorldName = (char*)LOC(0x054cad8);
	g_SoundFilePath = (char*)LOC(0x003e7758);

	int* ptr2 = LOC(0x0029e5c8);
	g_curLanguage = *ptr2;

	//create_file_a_hook = hook_addr(so_symbol(&so_mod, "CreateFileA"), (uintptr_t)create_file_a);


	//0010f154
	//ov_read_callback_hook = hook_addr(LOC(0x0010f154), (uintptr_t)&ov_read_callback);
	SoundSystem* ptr = (SoundSystem*)LOC(0x003dfc98);
	g_soundSystemBasePtr = ptr;


	// Hook SND_Frame to report ov_read stats per frame
	//snd_frame_hook = hook_addr(so_symbol(&so_mod, "_Z9SND_Framev"), (uintptr_t)&snd_frame_profiled);
}