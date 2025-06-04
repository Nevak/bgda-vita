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
#include "utils/logger.h"

#define LOC(x) (int *)(so_mod.text_base + x - 0x00010000)

extern so_module so_mod;
so_hook ov_read_hook;
so_hook snd_frame_hook;
so_hook file_read_hook;
so_hook file_seek_hook;
so_hook file_tell_hook;
so_hook vorbis_synthesis_read_hook;
so_hook vorbis_synthesis_pcmout_hook;
so_hook func_00241a08_hook;
so_hook ogg_malloc_hook;
so_hook vorbis_synthesis_hook;

int frame_num = 0;
int main_thread_id = 0;

float total_vorbis_synthesis_time_ms = 0;
int vorbis_synthesis_count = 0;
int vorbis_synthesis_fake(void *v, void *op) {
	if (main_thread_id != sceKernelGetThreadId()) {
		//return SO_CONTINUE(int, vorbis_synthesis_hook, v, op);
		return vorbis_synthesis(v, op);
	}
	
	// log how long it took to read
	float timeNow = sceKernelGetProcessTimeWide();
	//int ret = SO_CONTINUE(int, vorbis_synthesis_hook, v, op);
	int ret = vorbis_synthesis(v, op);
	float timeAfter = sceKernelGetProcessTimeWide();

	float deltaMs = (timeAfter - timeNow) / 1000;
	if (deltaMs >= 0) {
		logv_error("[%d] vorbis_synthesis took %f ms", frame_num, (timeAfter - timeNow) / 1000);
	}

	total_vorbis_synthesis_time_ms += deltaMs;
	vorbis_synthesis_count++;

	return ret;
}


float total_malloc_proifled_time_ms = 0;
int malloc_proifled_count = 0;
void* malloc_proifled(size_t size) {
	if (main_thread_id != sceKernelGetThreadId()) {
		return vglMalloc(size);
	}

	//logv_error("malloc_proifled(%zu)", size);

	// log how long it took to read
	float timeNow = sceKernelGetProcessTimeWide();
	void* ret = vglMalloc(size);
	float timeAfter = sceKernelGetProcessTimeWide();
	float deltaMs = (timeAfter - timeNow) / 1000;
	if (deltaMs > 10) {
		logv_error("[%d] malloc_proifled took %f ms", frame_num, (timeAfter - timeNow) / 1000);
	}
	total_malloc_proifled_time_ms += deltaMs;
	malloc_proifled_count++;
	return ret;
}

float total_func_00241a08_time_ms = 0;
int func_00241a08_count = 0;
void func_00241a08(int param_1,int param_2)
{
	if (main_thread_id != sceKernelGetThreadId()) {
		SO_CONTINUE(void*, func_00241a08_hook, param_1, param_2);
	}
	
	// log how long it took to read
	float timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void*, func_00241a08_hook, param_1, param_2);
	float timeAfter = sceKernelGetProcessTimeWide();

	float deltaMs = (timeAfter - timeNow) / 1000;
	if (deltaMs > 10) {
		logv_error("[%d] func_00241a08 took %f ms", frame_num, (timeAfter - timeNow) / 1000);
	}

	total_func_00241a08_time_ms += deltaMs;
	func_00241a08_count++;
}

float total_vorbis_synthesis_pcmout_time_ms = 0;
int vorbis_synthesis_pcmout_count = 0;
int vorbis_synthesis_pcmout_fake(void *v, float ***pcm) {
	if (main_thread_id != sceKernelGetThreadId()) {
		//return SO_CONTINUE(int, vorbis_synthesis_pcmout_hook, v, pcm);
		return vorbis_synthesis_pcmout(v, pcm);
	}
	
	// log how long it took to read
	float timeNow = sceKernelGetProcessTimeWide();
	//int ret = SO_CONTINUE(int, vorbis_synthesis_pcmout_hook, v, pcm);
	int ret = vorbis_synthesis_pcmout(v, pcm);
	float timeAfter = sceKernelGetProcessTimeWide();

	float deltaMs = (timeAfter - timeNow) / 1000;
	if (deltaMs > 10) {
		logv_error("[%d] vorbis_synthesis_pcmout took %f ms", frame_num, (timeAfter - timeNow) / 1000);
	}

	total_vorbis_synthesis_pcmout_time_ms += deltaMs;
	vorbis_synthesis_pcmout_count++;

	return ret;
}


float total_vorbis_synthesis_read_time_ms = 0;
int vorbis_synthesis_read_count = 0;
int vorbis_synthesis_read_fake(void *v, int samples) {
	if (main_thread_id != sceKernelGetThreadId()) {
		//return SO_CONTINUE(int, vorbis_synthesis_read_hook, v, samples);
		return vorbis_synthesis_read(v, samples);
	}
	
	// log how long it took to read
	float timeNow = sceKernelGetProcessTimeWide();
	//int ret = SO_CONTINUE(int, vorbis_synthesis_read_hook, v, samples);
	int ret = vorbis_synthesis_read(v, samples);
	float timeAfter = sceKernelGetProcessTimeWide();

	float deltaMs = (timeAfter - timeNow) / 1000;
	if (deltaMs > 10) {
		logv_error("[%d] vorbis_synthesis_read took %f ms", frame_num, (timeAfter - timeNow) / 1000);
	}

	total_vorbis_synthesis_read_time_ms += deltaMs;
	vorbis_synthesis_read_count++;

	return ret;
}

float total_file_tell_time_ms = 0;
int file_tell_count = 0;
int File_Tell_fake(void *thisptr) {
	if (main_thread_id != sceKernelGetThreadId()) {
		return SO_CONTINUE(int, file_tell_hook, thisptr);
	}
	
	// log how long it took to read
	float timeNow = sceKernelGetProcessTimeWide();
	int ret = SO_CONTINUE(int, file_tell_hook, thisptr);
	float timeAfter = sceKernelGetProcessTimeWide();

	float deltaMs = (timeAfter - timeNow) / 1000;
	if (deltaMs > 10) {
		logv_error("[%d] file_tell took %f ms", frame_num, (timeAfter - timeNow) / 1000);
	}

	total_file_tell_time_ms += deltaMs;
	file_tell_count++;

	return ret;
}

float total_file_seek_time_ms = 0;
int file_seek_count = 0;
int File_Seek_fake(void *thisptr, int offset, int whence) {
	if (main_thread_id != sceKernelGetThreadId()) {
		return SO_CONTINUE(int, file_seek_hook, thisptr, offset, whence);
	}
	
	// log how long it took to read
	float timeNow = sceKernelGetProcessTimeWide();
	int ret = SO_CONTINUE(int, file_seek_hook, thisptr, offset, whence);
	float timeAfter = sceKernelGetProcessTimeWide();

	float deltaMs = (timeAfter - timeNow) / 1000;
	if (deltaMs > 10) {
		logv_error("[%d] file_seek took %f ms", frame_num, (timeAfter - timeNow) / 1000);
	}

	total_file_seek_time_ms += deltaMs;
	file_seek_count++;

	return ret;
}

float total_file_read_time_ms = 0;
int file_read_count = 0;
int File_Read_fake(void *thisptr, void *buffer, int size) {
	if (main_thread_id != sceKernelGetThreadId()) {
		return SO_CONTINUE(int, file_read_hook, thisptr, buffer, size);
	}
	
	// print the contents of addresses around thisptr to see if we can find the file name
	//logv_error("File_Read(%p, %p, %i)\n", thisptr, buffer, size);
	// print the contents of addresses around thisptr to see if we can find the file name
	// uintptr_t addr = (uintptr_t)thisptr;
	// for (int i = 0; i < 16; i++) {
	// 	uintptr_t addr2 = addr + i ;
	// 	char *str = (char *)addr2;
	// 	if (str[0] != '\0') {
	// 		logv_error("File_Read: %p: %s\n", addr2, str);
	// 	}
	// }

	// log how long it took to read
	float timeNow = sceKernelGetProcessTimeWide();
	int ret = SO_CONTINUE(int, file_read_hook, thisptr, buffer, size);
	float timeAfter = sceKernelGetProcessTimeWide();

	float deltaMs = (timeAfter - timeNow) / 1000;
	if (deltaMs > 10) {
		logv_error("[%d] file_read took %f ms", frame_num, (timeAfter - timeNow) / 1000);
	}

	total_file_read_time_ms += deltaMs;
	file_read_count++;

	return ret;
}

float total_ov_read_time_ms = 0;
int ov_read_count = 0;
int ov_read_fake(OggVorbis_File *vf, char *buffer, int length, int bigendianp, int word, int sgned, int *bitstream)
{
	if (main_thread_id != sceKernelGetThreadId()) {
		return ov_read(vf, buffer, length, bigendianp, word, sgned, bitstream);
	}
	// log how long it took to read
	float timeNow = sceKernelGetProcessTimeWide();
	//int ret = SO_CONTINUE(int, ov_read_hook, vf, buffer, length, bigendianp, word, sgned, bitstream);
	int ret = ov_read(vf, buffer, length, bigendianp, word, sgned, bitstream);
	float timeAfter = sceKernelGetProcessTimeWide();

	float deltaMs = (timeAfter - timeNow) / 1000;
	if (deltaMs > 10) {
		logv_error("[%d] ov_read took %f ms", frame_num, (timeAfter - timeNow) / 1000);
	}

	total_ov_read_time_ms += deltaMs;
	ov_read_count++;

	return ret;
}

// typedef struct {
//     uint64_t convert_us;
//     uint64_t pcmout_us;
//     uint64_t fetch_us;
//     uint64_t get_data_us;
//     uint64_t vorbis_synthesis_us;
//     uint64_t inverse_us;
//     uint64_t alloc_pcm_us;
//     uint64_t vorbis_synthesis_part_1us;
// } OvProf;

// extern OvProf g_ovp;
// void ov_prof_start(void)
// {
//     memset(&g_ovp, 0, sizeof g_ovp);
// }

// void ov_prof_dump(void)
// {
//   sceClibPrintf("[OVPROF] fetch  = %8llu us\n", g_ovp.fetch_us);
//   sceClibPrintf("[OVPROF] pcmout = %8llu us\n", g_ovp.pcmout_us);
//   sceClibPrintf("[OVPROF] convert= %8llu us\n", g_ovp.convert_us);
//   sceClibPrintf("[OVPROF] get_data= %8llu us\n", g_ovp.get_data_us);
//   sceClibPrintf("[OVPROF] vorbis_synthesis_us = %8llu us\n", g_ovp.vorbis_synthesis_us);
//   sceClibPrintf("[OVPROF] inverse_us = %8llu us\n", g_ovp.inverse_us);
//   sceClibPrintf("[OVPROF] alloc_pcm_us = %8llu us\n", g_ovp.alloc_pcm_us);
//   sceClibPrintf("[OVPROF] vorbis_synthesis_part_1us = %8llu us\n", g_ovp.vorbis_synthesis_part_1us);
//   memset(&g_ovp, 0, sizeof g_ovp);
// }


void SND_Frame_fake()
{
	main_thread_id = sceKernelGetThreadId();

	// reset counters
	total_ov_read_time_ms = 0;
	ov_read_count = 0;
	total_file_read_time_ms = 0;
	file_read_count = 0;
	total_file_tell_time_ms = 0;
	file_tell_count = 0;
	total_file_seek_time_ms = 0;
	file_seek_count = 0;
	total_vorbis_synthesis_read_time_ms = 0;
	vorbis_synthesis_read_count = 0;
	total_vorbis_synthesis_pcmout_time_ms = 0;
	vorbis_synthesis_pcmout_count = 0;
	total_func_00241a08_time_ms = 0;
	func_00241a08_count = 0;
	total_malloc_proifled_time_ms = 0;
	malloc_proifled_count = 0;
	total_vorbis_synthesis_time_ms = 0;
	vorbis_synthesis_count = 0;

	//ov_prof_start();
	Profiler_Reset();
	Profiler_BeginSample("SND_Frame");
	//codec_prof_start();
	// log how long it took to read
	uint64_t timeNow = sceKernelGetProcessTimeWide();
	SO_CONTINUE(void*, snd_frame_hook);
	uint64_t timeAfter = sceKernelGetProcessTimeWide();

	Profiler_EndSample();
	uint64_t delta = (timeAfter - timeNow);
	double deltaMs = (double)delta / 1000.0f;
	if (deltaMs > 10) {
		logv_error("[%d] SND_Frame took %f ms", frame_num, deltaMs);

		// logv_error("[%d] \t ov_read: %fms in %i calls",frame_num, total_ov_read_time_ms, ov_read_count);
		// logv_error("[%d] \t \t vorbis_synthesis_read: %fms in %i calls",frame_num, total_vorbis_synthesis_read_time_ms, vorbis_synthesis_read_count);
		// logv_error("[%d] \t \t vorbis_synthesis_pcmout: %fms in %i calls",frame_num, total_vorbis_synthesis_pcmout_time_ms, vorbis_synthesis_pcmout_count);
		// logv_error("[%d] \t \t func_00241a08: %fms in %i calls",frame_num, total_func_00241a08_time_ms, func_00241a08_count);
		// logv_error("[%d] \t file_read: %fms in %i calls",frame_num, total_file_read_time_ms, file_read_count);
		// logv_error("[%d] \t malloc_proifled: %fms in %i calls",frame_num, total_malloc_proifled_time_ms, malloc_proifled_count);
		// logv_error("[%d] \t vorbis_synthesis: %fms in %i calls",frame_num, total_vorbis_synthesis_time_ms, vorbis_synthesis_count);
		
		// logv_error("[%d] \t file_tell: %fms in %i calls",frame_num, total_file_tell_time_ms, file_tell_count);
		// logv_error("[%d] \t file_seek: %fms in %i calls",frame_num, total_file_seek_time_ms, file_seek_count);

		//ov_prof_dump();
		//codec_prof_dump();
		Profiler_PrintSamples(-1, 0);
	}


	++frame_num;
}


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
	ov_read_hook = hook_addr(so_symbol(&so_mod, "ov_read"), (uintptr_t)ov_read);
	snd_frame_hook = hook_addr(so_symbol(&so_mod, "_Z9SND_Framev"), (uintptr_t)SND_Frame_fake);
	// file_read_hook = hook_addr(so_symbol(&so_mod, "_ZN3JBE4File4ReadEPvj"), (uintptr_t)File_Read_fake);
	// file_seek_hook = hook_addr(so_symbol(&so_mod, "_ZN3JBE4File4SeekEiNS0_10SeekWhenceE"), (uintptr_t)File_Seek_fake);
	// file_tell_hook = hook_addr(so_symbol(&so_mod, "_ZNK3JBE4File4TellEv"), (uintptr_t)File_Tell_fake);
	//func_00241a08_hook = hook_addr(LOC(0x00241a08), (uintptr_t)func_00241a08);
	//ogg_malloc_hook = hook_addr(so_symbol(&so_mod, "_ogg_malloc"), (uintptr_t)ogg_malloc_fake);
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