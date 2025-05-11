/* vorbis_patch.c -- vorbis redirection
 *
 * Copyright (C) 2022 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/codec.h>

#include <so_util/so_util.h>

extern so_module so_mod;

void patch_vorbis(void) {
	hook_addr(so_symbol(&so_mod, "vorbis_analysis"), (uintptr_t)vorbis_analysis);
	hook_addr(so_symbol(&so_mod, "vorbis_analysis_blockout"), (uintptr_t)vorbis_analysis_blockout);
	hook_addr(so_symbol(&so_mod, "vorbis_analysis_buffer"), (uintptr_t)vorbis_analysis_buffer);
	hook_addr(so_symbol(&so_mod, "vorbis_analysis_headerout"), (uintptr_t)vorbis_analysis_headerout);
	hook_addr(so_symbol(&so_mod, "vorbis_analysis_init"), (uintptr_t)vorbis_analysis_init);
	hook_addr(so_symbol(&so_mod, "vorbis_analysis_wrote"), (uintptr_t)vorbis_analysis_wrote);
	hook_addr(so_symbol(&so_mod, "vorbis_bitrate_addblock"), (uintptr_t)vorbis_bitrate_addblock);
	hook_addr(so_symbol(&so_mod, "vorbis_bitrate_flushpacket"), (uintptr_t)vorbis_bitrate_flushpacket);
	hook_addr(so_symbol(&so_mod, "vorbis_block_clear"), (uintptr_t)vorbis_block_clear);
	hook_addr(so_symbol(&so_mod, "vorbis_block_init"), (uintptr_t)vorbis_block_init);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_add"), (uintptr_t)vorbis_comment_add);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_add_tag"), (uintptr_t)vorbis_comment_add_tag);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_clear"), (uintptr_t)vorbis_comment_clear);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_init"), (uintptr_t)vorbis_comment_init);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_query"), (uintptr_t)vorbis_comment_query);
	hook_addr(so_symbol(&so_mod, "vorbis_comment_query_count"), (uintptr_t)vorbis_comment_query_count);
	hook_addr(so_symbol(&so_mod, "vorbis_commentheader_out"), (uintptr_t)vorbis_commentheader_out);
	hook_addr(so_symbol(&so_mod, "vorbis_dsp_clear"), (uintptr_t)vorbis_dsp_clear);
	hook_addr(so_symbol(&so_mod, "vorbis_info_blocksize"), (uintptr_t)vorbis_info_blocksize);
	hook_addr(so_symbol(&so_mod, "vorbis_info_clear"), (uintptr_t)vorbis_info_clear);
	hook_addr(so_symbol(&so_mod, "vorbis_info_init"), (uintptr_t)vorbis_info_init);
	hook_addr(so_symbol(&so_mod, "vorbis_packet_blocksize"), (uintptr_t)vorbis_packet_blocksize);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis"), (uintptr_t)vorbis_synthesis);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_blockin"), (uintptr_t)vorbis_synthesis_blockin);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_headerin"), (uintptr_t)vorbis_synthesis_headerin);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_init"), (uintptr_t)vorbis_synthesis_init);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_pcmout"), (uintptr_t)vorbis_synthesis_pcmout);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_read"), (uintptr_t)vorbis_synthesis_read);
	hook_addr(so_symbol(&so_mod, "vorbis_synthesis_trackonly"), (uintptr_t)vorbis_synthesis_trackonly);
	
	hook_addr(so_symbol(&so_mod, "ov_bitrate"), (uintptr_t)ov_bitrate);
	hook_addr(so_symbol(&so_mod, "ov_bitrate_instant"), (uintptr_t)ov_bitrate_instant);
	hook_addr(so_symbol(&so_mod, "ov_clear"), (uintptr_t)ov_clear);
	hook_addr(so_symbol(&so_mod, "ov_comment"), (uintptr_t)ov_comment);
	hook_addr(so_symbol(&so_mod, "ov_crosslap"), (uintptr_t)ov_crosslap);
	hook_addr(so_symbol(&so_mod, "ov_fopen"), (uintptr_t)ov_fopen);
	hook_addr(so_symbol(&so_mod, "ov_halfrate"), (uintptr_t)ov_halfrate);
	hook_addr(so_symbol(&so_mod, "ov_halfrate_p"), (uintptr_t)ov_halfrate_p);
	hook_addr(so_symbol(&so_mod, "ov_info"), (uintptr_t)ov_info);
	hook_addr(so_symbol(&so_mod, "ov_open"), (uintptr_t)ov_open);
	hook_addr(so_symbol(&so_mod, "ov_open_callbacks"), (uintptr_t)ov_open_callbacks);
	hook_addr(so_symbol(&so_mod, "ov_pcm_seek"), (uintptr_t)ov_pcm_seek);
	hook_addr(so_symbol(&so_mod, "ov_pcm_seek_lap"), (uintptr_t)ov_pcm_seek_lap);
	hook_addr(so_symbol(&so_mod, "ov_pcm_seek_page"), (uintptr_t)ov_pcm_seek_page);
	hook_addr(so_symbol(&so_mod, "ov_pcm_seek_page_lap"), (uintptr_t)ov_pcm_seek_page_lap);
	hook_addr(so_symbol(&so_mod, "ov_pcm_tell"), (uintptr_t)ov_pcm_tell);
	hook_addr(so_symbol(&so_mod, "ov_pcm_total"), (uintptr_t)ov_pcm_total);
	hook_addr(so_symbol(&so_mod, "ov_raw_seek"), (uintptr_t)ov_raw_seek);
	hook_addr(so_symbol(&so_mod, "ov_raw_seek_lap"), (uintptr_t)ov_raw_seek_lap);
	hook_addr(so_symbol(&so_mod, "ov_raw_tell"), (uintptr_t)ov_raw_tell);
	hook_addr(so_symbol(&so_mod, "ov_raw_total"), (uintptr_t)ov_raw_total);
	hook_addr(so_symbol(&so_mod, "ov_read"), (uintptr_t)ov_read);
	hook_addr(so_symbol(&so_mod, "ov_read_float"), (uintptr_t)ov_read_float);
	hook_addr(so_symbol(&so_mod, "ov_seekable"), (uintptr_t)ov_seekable);
	hook_addr(so_symbol(&so_mod, "ov_serialnumber"), (uintptr_t)ov_serialnumber);
	hook_addr(so_symbol(&so_mod, "ov_streams"), (uintptr_t)ov_streams);
	hook_addr(so_symbol(&so_mod, "ov_test"), (uintptr_t)ov_test);
	hook_addr(so_symbol(&so_mod, "ov_test_callbacks"), (uintptr_t)ov_test_callbacks);
	hook_addr(so_symbol(&so_mod, "ov_test_open"), (uintptr_t)ov_test_open);
	hook_addr(so_symbol(&so_mod, "ov_time_seek"), (uintptr_t)ov_time_seek);
	hook_addr(so_symbol(&so_mod, "ov_time_seek_lap"), (uintptr_t)ov_time_seek_lap);
	hook_addr(so_symbol(&so_mod, "ov_time_seek_page"), (uintptr_t)ov_time_seek_page);
	hook_addr(so_symbol(&so_mod, "ov_time_seek_page_lap"), (uintptr_t)ov_time_seek_page_lap);
	hook_addr(so_symbol(&so_mod, "ov_time_tell"), (uintptr_t)ov_time_tell);
	hook_addr(so_symbol(&so_mod, "ov_time_total"), (uintptr_t)ov_time_total);
}