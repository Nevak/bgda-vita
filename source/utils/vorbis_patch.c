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

/*
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
*/


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
	
	// Disable this for now. It was meant to fix some background audio loops not playing,
	// but it's crashing the game in the final cinematic video. Will revisit
	//snd_get_dialog_dir_hook = hook_addr(so_symbol(&so_mod, "_Z16SND_GetDialogDirv"), (uintptr_t)snd_get_dialog_dir);

}