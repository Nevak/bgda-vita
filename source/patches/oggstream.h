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

#ifdef PROFILER_ENABLED
#include <utils/prof.h>
#include <libperf.h>
#endif

int g_curLanguage;
extern so_module so_mod;

// Per-frame statistics for ov_read profiling
static int g_ovReadCallCount = 0;
static float g_ovReadTotalTimeMs = 0.0f;
static long g_ovReadTotalBytesRequested = 0;
static long g_ovReadTotalBytesReturned = 0;
static int g_ovReadMinBufferSize = 999999;
static int g_ovReadMaxBufferSize = 0;
static int g_ovReadInternalCallCount = 0;  // Track internal batching
static int g_sndFrameNumber = 0;  // Track which frame we're on

typedef struct StreamSlot { /* Individual sound stream slot within a channel */
    char path[16];
    int startSample;
    int roundedSampleCount; /* Rounded sample count: (endSample / 0x48) * 0x48 */
    int startSample2;
    int endSample;
    int statusOrFlags; /* Stream flags:                              //        bit 0: playing/active                              //        bit 1: fade command                              //        bit 2: unknown                              //        bit 3: paused (set/clear by continue stream)     */
    void *oggStream;
}StreamSlot;

typedef struct SoundChannel {
    uint8_t field0_0x0;
    uint8_t field1_0x1;
    uint8_t field2_0x2;
    uint8_t field3_0x3;
    uint8_t field4_0x4;
    uint8_t field5_0x5;
    uint8_t field6_0x6;
    uint8_t field7_0x7;
    uint8_t field8_0x8;
    uint8_t field9_0x9;
    uint8_t field10_0xa;
    uint8_t field11_0xb;
    uint8_t field12_0xc;
    uint8_t field13_0xd;
    uint8_t field14_0xe;
    uint8_t field15_0xf;
    int timeAccumulator; /* Created by retype action */
    int countdown; /* Created by retype action */
    uint64_t field18_0x18;
    uint8_t field19_0x20;
    uint8_t field20_0x21;
    uint8_t field21_0x22;
    uint8_t field22_0x23;
    uint *filePtr;
    int field24_0x28;
    uint field25_0x2c;
    uint8_t field26_0x30;
    uint8_t field27_0x31;
    uint8_t field28_0x32;
    uint8_t field29_0x33;
    uint8_t field30_0x34;
    uint8_t field31_0x35;
    uint8_t field32_0x36;
    uint8_t field33_0x37;
    int bufferSize; /* Created by retype action */
    uint8_t field35_0x3c;
    uint8_t field36_0x3d;
    uint8_t field37_0x3e;
    uint8_t field38_0x3f;
    uint8_t field39_0x40;
    uint8_t field40_0x41;
    uint8_t field41_0x42;
    uint8_t field42_0x43;
    uint8_t field43_0x44;
    uint8_t field44_0x45;
    uint8_t field45_0x46;
    uint8_t field46_0x47;
    uint8_t field47_0x48;
    uint8_t field48_0x49;
    uint8_t field49_0x4a;
    uint8_t field50_0x4b;
    uint8_t field51_0x4c;
    uint8_t field52_0x4d;
    uint8_t field53_0x4e;
    uint8_t field54_0x4f;
    uint8_t field55_0x50;
    uint8_t field56_0x51;
    uint8_t field57_0x52;
    uint8_t field58_0x53;
    uint8_t field59_0x54;
    uint8_t field60_0x55;
    uint8_t field61_0x56;
    uint8_t field62_0x57;
    uint8_t field63_0x58;
    uint8_t field64_0x59;
    uint8_t field65_0x5a;
    uint8_t field66_0x5b;
    uint8_t field67_0x5c;
    uint8_t field68_0x5d;
    uint8_t field69_0x5e;
    uint8_t field70_0x5f;
    uint8_t field71_0x60;
    uint8_t field72_0x61;
    uint8_t field73_0x62;
    uint8_t field74_0x63;
    uint8_t field75_0x64;
    uint8_t field76_0x65;
    uint8_t field77_0x66;
    uint8_t field78_0x67;
    uint8_t field79_0x68;
    uint8_t field80_0x69;
    uint8_t field81_0x6a;
    uint8_t field82_0x6b;
    uint8_t field83_0x6c;
    uint8_t field84_0x6d;
    uint8_t field85_0x6e;
    uint8_t field86_0x6f;
    uint8_t field87_0x70;
    uint8_t field88_0x71;
    uint8_t field89_0x72;
    uint8_t field90_0x73;
    uint8_t field91_0x74;
    uint8_t field92_0x75;
    uint8_t field93_0x76;
    uint8_t field94_0x77;
    uint8_t field95_0x78;
    uint8_t field96_0x79;
    uint8_t field97_0x7a;
    uint8_t field98_0x7b;
    uint8_t field99_0x7c;
    uint8_t field100_0x7d;
    uint8_t field101_0x7e;
    uint8_t field102_0x7f;
    uint8_t field103_0x80;
    uint8_t field104_0x81;
    uint8_t field105_0x82;
    uint8_t field106_0x83;
    uint8_t field107_0x84;
    uint8_t field108_0x85;
    uint8_t field109_0x86;
    uint8_t field110_0x87;
    uint8_t field111_0x88;
    uint8_t field112_0x89;
    uint8_t field113_0x8a;
    uint8_t field114_0x8b;
    uint8_t field115_0x8c;
    uint8_t field116_0x8d;
    uint8_t field117_0x8e;
    uint8_t field118_0x8f;
    uint8_t field119_0x90;
    uint8_t field120_0x91;
    uint8_t field121_0x92;
    uint8_t field122_0x93;
    uint8_t field123_0x94;
    uint8_t field124_0x95;
    uint8_t field125_0x96;
    uint8_t field126_0x97;
    uint8_t field127_0x98;
    uint8_t field128_0x99;
    uint8_t field129_0x9a;
    uint8_t field130_0x9b;
    uint8_t field131_0x9c;
    uint8_t field132_0x9d;
    uint8_t field133_0x9e;
    uint8_t field134_0x9f;
    uint8_t field135_0xa0;
    uint8_t field136_0xa1;
    uint8_t field137_0xa2;
    uint8_t field138_0xa3;
    uint8_t field139_0xa4;
    uint8_t field140_0xa5;
    uint8_t field141_0xa6;
    uint8_t field142_0xa7;
    uint8_t field143_0xa8;
    uint8_t field144_0xa9;
    uint8_t field145_0xaa;
    uint8_t field146_0xab;
    uint8_t field147_0xac;
    uint8_t field148_0xad;
    uint8_t field149_0xae;
    uint8_t field150_0xaf;
    uint8_t field151_0xb0;
    uint8_t field152_0xb1;
    uint8_t field153_0xb2;
    uint8_t field154_0xb3;
    uint8_t field155_0xb4;
    uint8_t field156_0xb5;
    uint8_t field157_0xb6;
    uint8_t field158_0xb7;
    uint8_t field159_0xb8;
    uint8_t field160_0xb9;
    uint8_t field161_0xba;
    uint8_t field162_0xbb;
    uint8_t field163_0xbc;
    uint8_t field164_0xbd;
    uint8_t field165_0xbe;
    uint8_t field166_0xbf;
    uint8_t field167_0xc0;
    uint8_t field168_0xc1;
    uint8_t field169_0xc2;
    uint8_t field170_0xc3;
    uint8_t field171_0xc4;
    uint8_t field172_0xc5;
    uint8_t field173_0xc6;
    uint8_t field174_0xc7;
    uint8_t field175_0xc8;
    uint8_t field176_0xc9;
    uint8_t field177_0xca;
    uint8_t field178_0xcb;
    uint8_t field179_0xcc;
    uint8_t field180_0xcd;
    uint8_t field181_0xce;
    uint8_t field182_0xcf;
    uint8_t field183_0xd0;
    uint8_t field184_0xd1;
    uint8_t field185_0xd2;
    uint8_t field186_0xd3;
    uint8_t field187_0xd4;
    uint8_t field188_0xd5;
    uint8_t field189_0xd6;
    uint8_t field190_0xd7;
    uint8_t field191_0xd8;
    uint8_t field192_0xd9;
    uint8_t field193_0xda;
    uint8_t field194_0xdb;
    uint8_t field195_0xdc;
    uint8_t field196_0xdd;
    uint8_t field197_0xde;
    uint8_t field198_0xdf;
    uint8_t field199_0xe0;
    uint8_t field200_0xe1;
    uint8_t field201_0xe2;
    uint8_t field202_0xe3;
    uint8_t field203_0xe4;
    uint8_t field204_0xe5;
    uint8_t field205_0xe6;
    uint8_t field206_0xe7;
    uint8_t field207_0xe8;
    uint8_t field208_0xe9;
    uint8_t field209_0xea;
    uint8_t field210_0xeb;
    uint8_t field211_0xec;
    uint8_t field212_0xed;
    uint8_t field213_0xee;
    uint8_t field214_0xef;
    uint8_t field215_0xf0;
    uint8_t field216_0xf1;
    uint8_t field217_0xf2;
    uint8_t field218_0xf3;
    uint8_t field219_0xf4;
    uint8_t field220_0xf5;
    uint8_t field221_0xf6;
    uint8_t field222_0xf7;
    uint8_t field223_0xf8;
    uint8_t field224_0xf9;
    uint8_t field225_0xfa;
    uint8_t field226_0xfb;
    uint8_t field227_0xfc;
    uint8_t field228_0xfd;
    uint8_t field229_0xfe;
    uint8_t field230_0xff;
    uint8_t field231_0x100;
    uint8_t field232_0x101;
    uint8_t field233_0x102;
    uint8_t field234_0x103;
    uint8_t field235_0x104;
    uint8_t field236_0x105;
    uint8_t field237_0x106;
    uint8_t field238_0x107;
    uint8_t field239_0x108;
    uint8_t field240_0x109;
    uint8_t field241_0x10a;
    uint8_t field242_0x10b;
    uint8_t field243_0x10c;
    uint8_t field244_0x10d;
    uint8_t field245_0x10e;
    uint8_t field246_0x10f;
    uint8_t field247_0x110;
    uint8_t field248_0x111;
    uint8_t field249_0x112;
    uint8_t field250_0x113;
    uint8_t field251_0x114;
    uint8_t field252_0x115;
    uint8_t field253_0x116;
    uint8_t field254_0x117;
    uint8_t field255_0x118;
    uint8_t field256_0x119;
    uint8_t field257_0x11a;
    uint8_t field258_0x11b;
    uint8_t field259_0x11c;
    uint8_t field260_0x11d;
    uint8_t field261_0x11e;
    uint8_t field262_0x11f;
    uint8_t field263_0x120;
    uint8_t field264_0x121;
    uint8_t field265_0x122;
    uint8_t field266_0x123;
    uint8_t field267_0x124;
    uint8_t field268_0x125;
    uint8_t field269_0x126;
    uint8_t field270_0x127;
    uint8_t field271_0x128;
    uint8_t field272_0x129;
    uint8_t field273_0x12a;
    uint8_t field274_0x12b;
    uint8_t field275_0x12c;
    uint8_t field276_0x12d;
    uint8_t field277_0x12e;
    uint8_t field278_0x12f;
    uint8_t field279_0x130;
    uint8_t field280_0x131;
    uint8_t field281_0x132;
    uint8_t field282_0x133;
    uint8_t field283_0x134;
    uint8_t field284_0x135;
    uint8_t field285_0x136;
    uint8_t field286_0x137;
    uint8_t field287_0x138;
    uint8_t field288_0x139;
    uint8_t field289_0x13a;
    uint8_t field290_0x13b;
    uint8_t field291_0x13c;
    uint8_t field292_0x13d;
    uint8_t field293_0x13e;
    uint8_t field294_0x13f;
    uint8_t field295_0x140;
    uint8_t field296_0x141;
    uint8_t field297_0x142;
    uint8_t field298_0x143;
    uint8_t field299_0x144;
    uint8_t field300_0x145;
    uint8_t field301_0x146;
    uint8_t field302_0x147;
    uint8_t field303_0x148;
    uint8_t field304_0x149;
    uint8_t field305_0x14a;
    uint8_t field306_0x14b;
    uint8_t field307_0x14c;
    uint8_t field308_0x14d;
    uint8_t field309_0x14e;
    uint8_t field310_0x14f;
    uint8_t field311_0x150;
    uint8_t field312_0x151;
    uint8_t field313_0x152;
    uint8_t field314_0x153;
    uint8_t field315_0x154;
    uint8_t field316_0x155;
    uint8_t field317_0x156;
    uint8_t field318_0x157;
    uint8_t field319_0x158;
    uint8_t field320_0x159;
    uint8_t field321_0x15a;
    uint8_t field322_0x15b;
    uint8_t field323_0x15c;
    uint8_t field324_0x15d;
    uint8_t field325_0x15e;
    uint8_t field326_0x15f;
    uint8_t field327_0x160;
    uint8_t field328_0x161;
    uint8_t field329_0x162;
    uint8_t field330_0x163;
    uint8_t field331_0x164;
    uint8_t field332_0x165;
    uint8_t field333_0x166;
    uint8_t field334_0x167;
    uint8_t field335_0x168;
    uint8_t field336_0x169;
    uint8_t field337_0x16a;
    uint8_t field338_0x16b;
    uint8_t field339_0x16c;
    uint8_t field340_0x16d;
    uint8_t field341_0x16e;
    uint8_t field342_0x16f;
    uint8_t field343_0x170;
    uint8_t field344_0x171;
    uint8_t field345_0x172;
    uint8_t field346_0x173;
    uint8_t field347_0x174;
    uint8_t field348_0x175;
    uint8_t field349_0x176;
    uint8_t field350_0x177;
    uint8_t field351_0x178;
    uint8_t field352_0x179;
    uint8_t field353_0x17a;
    uint8_t field354_0x17b;
    uint8_t field355_0x17c;
    uint8_t field356_0x17d;
    uint8_t field357_0x17e;
    uint8_t field358_0x17f;
    uint8_t field359_0x180;
    uint8_t field360_0x181;
    uint8_t field361_0x182;
    uint8_t field362_0x183;
    uint8_t field363_0x184;
    uint8_t field364_0x185;
    uint8_t field365_0x186;
    uint8_t field366_0x187;
    uint8_t field367_0x188;
    uint8_t field368_0x189;
    uint8_t field369_0x18a;
    uint8_t field370_0x18b;
    uint8_t field371_0x18c;
    uint8_t field372_0x18d;
    uint8_t field373_0x18e;
    uint8_t field374_0x18f;
    uint8_t field375_0x190;
    uint8_t field376_0x191;
    uint8_t field377_0x192;
    uint8_t field378_0x193;
    uint8_t field379_0x194;
    uint8_t field380_0x195;
    uint8_t field381_0x196;
    uint8_t field382_0x197;
    StreamSlot streamSlots[8];
    int streamCount; /* Created by retype action */
}SoundChannel;


typedef struct SoundSystem {
    SoundChannel channels[3];
}SoundSystem;

char* g_currentWorldName;
char* g_SoundFilePath;
SoundSystem g_soundSystemBase;

_Static_assert(offsetof(SoundChannel, streamCount) == 0x2d8,
               "`state` is not at the right offset - fix the struct or add packed!");

/*
// Batched + Profiled version of ov_read that fills buffers efficiently
long ov_read_profiled2(OggVorbis_File *vf, char *buffer, int length,
                      int bigendianp, int word, int sgned, int *bitstream) {

	// sceRazorCpuPushMarkerWithHud("ov_read", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);

	// long r = ov_read(vf, buffer, length, bigendianp, word, sgned, bitstream);
	// //long r = SO_CONTINUE(long, snd_frame_hook);
	// sceRazorCpuPopMarker();

	//return r;
	uint64_t startTime = sceKernelGetProcessTimeWide();

	long totalRead = 0;
	int internalCalls = 0;

	// Validate vorbis file handle before attempting to read
	// Prevents crash when FIOS closes idle file descriptors
	if (vf == NULL) {
		log_error("ov_read_profiled: NULL vorbis file handle, skipping read");
		return 0;
	}

	// Keep calling ov_read until buffer is full or stream ends
	// This batches multiple Vorbis packets into one call from the game's perspective
	while (totalRead < length) {
		SCE_PERF_ARM_PMON_STOP_ALL();
		sceRazorCpuPushMarkerWithHud("ov_read", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
		SCE_PERF_ARM_PMON_START_ALL();

		long result = ov_read(vf, buffer + totalRead,
		                     length - totalRead,
		                     bigendianp, word, sgned, bitstream);
		
		SCE_PERF_ARM_PMON_STOP_ALL();
		sceRazorCpuPopMarker();

		internalCalls++;

		if (result < 0) {
			// Error (possibly closed file descriptor) - stop reading
			logv_error("ov_read error: %ld (file descriptor may have been closed by FIOS)", result);
			break;
		}

		if (result == 0) {
			// EOF - stop reading
			break;
		}

		totalRead += result;

		// Stop if we've filled at least 90% of the requested buffer
		// This balances efficiency vs. latency
		if (totalRead >= (long)(length * 0.9)) {
			break;
		}

		// Safety: don't loop forever if something is wrong
		if (internalCalls > 1000) {
			break;
		}
	}

	int64_t endTime = sceKernelGetProcessTimeWide();
	float elapsedMs = (endTime - startTime) / 1000.0f;

	//Accumulate stats for this frame
	g_ovReadCallCount++;
	g_ovReadTotalTimeMs += elapsedMs;
	g_ovReadTotalBytesRequested += length;
	g_ovReadTotalBytesReturned += totalRead;
	g_ovReadInternalCallCount += internalCalls;

	if (length < g_ovReadMinBufferSize) g_ovReadMinBufferSize = length;
	if (length > g_ovReadMaxBufferSize) g_ovReadMaxBufferSize = length;

	return totalRead;
}

#define MAX_OV_READ_PER_FRAME 20000
int current_bitstream = -1;
int previous_bitstream = -1;

long ov_read_profiled3(OggVorbis_File *vf, char *buffer, int length,
                      int bigendianp, int word, int sgned, int *bitstream) {
	g_ovReadCallCount++;

	// if (g_ovReadCallCount >= MAX_OV_READ_PER_FRAME) {
    //     return 0;  // Pretend we hit end of stream temporarily
    // }

	SCE_PERF_ARM_PMON_STOP_ALL();
	sceRazorCpuPushMarkerWithHud("ov_read", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
	SCE_PERF_ARM_PMON_START_ALL();

	// // Also limit decode size per call
    // const int MAX_CHUNK = 0x9000;  // 36KB per call
    // if (length > MAX_CHUNK) {
    //     length = MAX_CHUNK;
    // }
    

	long result = ov_read(vf, buffer, length, bigendianp, word, sgned, bitstream);
	
	SCE_PERF_ARM_PMON_STOP_ALL();
	sceRazorCpuPopMarker();

	previous_bitstream = current_bitstream;
	current_bitstream = *bitstream;

	if (previous_bitstream != -1 && current_bitstream != previous_bitstream) {
        logv_error("Stream changed from %d to %d - DECODER REINITIALIZED!\n",
               previous_bitstream, current_bitstream);
    }

	//logv_error("ov_read stream = %d: ", *bitstream);


	return result;
}
*/
so_hook ov_read_hook;

long ov_read_profiled(void *vf, char *buffer, int length,
                      int bigendianp, int word, int sgned, int *bitstream) {
	g_ovReadCallCount++;

	// if (g_ovReadCallCount >= MAX_OV_READ_PER_FRAME) {
    //     return 0;  // Pretend we hit end of stream temporarily
    // }

	SCE_PERF_ARM_PMON_STOP_ALL();
	sceRazorCpuPushMarkerWithHud("ov_read", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
	SCE_PERF_ARM_PMON_START_ALL();

	// // Also limit decode size per call
    // const int MAX_CHUNK = 0x9000;  // 36KB per call
    // if (length > MAX_CHUNK) {
    //     length = MAX_CHUNK;
    // }
    

	long result = SO_CONTINUE(long, ov_read_hook, vf, buffer, length, bigendianp, word, sgned, bitstream);
	
	SCE_PERF_ARM_PMON_STOP_ALL();
	sceRazorCpuPopMarker();


	//logv_error("ov_read: vf=%p, len=%d, totalRead=%ld, stream = %d: ",vf, length, result, *bitstream);


	return result;
}
// Hook for SND_Frame - reports ov_read stats per frame
so_hook snd_frame_hook;
void snd_frame_profiled(void) {
	//return;

	g_sndFrameNumber++;

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
	sceRazorCpuPushMarkerWithHud("SND_Frame", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
	SO_CONTINUE(void*, snd_frame_hook);
	sceRazorCpuPopMarker();
}

so_hook ov_read_callback_hook;
void ov_read_callback(void *param_1, int param_2,int param_3,void *param_4)
{
	sceRazorCpuPushMarkerWithHud("ov_read_callback", SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD);
	SO_CONTINUE(void*, ov_read_callback_hook, param_1, param_2, param_3, param_4);
	sceRazorCpuPopMarker();
}

so_hook snd_start_stream_hook;
void snd_start_stream(int channelId,char *path,int streamFlags,int startSample,uint32_t len)
{
	// Get the caller address
	uintptr_t caller = (uintptr_t)__builtin_return_address(0);
	logv_error("[%p] snd_start_stream: channelId=%d, path=%s, streamFlags=0x%X, startSample=0x%X, len=0x%X", caller, channelId, path, streamFlags, startSample, len);



	// if (len == 0) {
	// 	return;
	// }
	// if (channelId == 0) {

	// }
	// if (channelId == 1 || channelId == 0)
	// {
	// 	return;
	// }

	SO_CONTINUE(void*, snd_start_stream_hook, channelId, path, streamFlags, startSample, len);

    int* streamCount0 = &g_soundSystemBase.channels[0].streamCount;
	int* streamCount1 = &g_soundSystemBase.channels[1].streamCount;
	int* streamCount2 = &g_soundSystemBase.channels[2].streamCount;

    int* streamCountPtr = &g_soundSystemBase.channels[channelId].streamCount;
    int streamSlotIndex = *streamCountPtr;
    
    OggStream* oggStreamPtr = g_SoundSystemBase.channels[channelId].streamSlots[streamSlotIndex].oggStreamPtr;
    logv_error("oggStreamPtr=%p", oggStreamPtr);
    //int oggStreamLen = oggStreamPtr->


	logv_error("streamCount[0]=%d", *streamCount0);
	logv_error("streamCount[1]=%d", *streamCount1);
	logv_error("streamCount[2]=%d", *streamCount2);
	logv_error("[%p] RETURNED snd_start_stream: channelId=%d, path=%s, streamFlags=0x%X, startSample=0x%X, len=0x%X", caller, channelId, path, streamFlags, startSample, len);
}

so_hook snd_start_music_hook;
void snd_start_music(char *path,int param)
{
	logv_error("snd_start_music: path=%s, param=0x%X", path, param);

	SO_CONTINUE(void*, snd_start_music_hook, path, param);
}



typedef struct dialog_dir
{
	char name[64];
	int startAt;
	int unk;
	int len;
}dialog_dir;

typedef struct {
	char name[48];
	int unk1;
	int unk2;
	int len;
	int unk3;
}lmp_entry;

so_hook lump_query_hook;

lmp_entry* lump_query(char* query)
{
	lmp_entry* res = SO_CONTINUE(lmp_entry*, lump_query_hook, query);
	return res;
}

so_hook lump_find_resource_hook;
lmp_entry* lump_find_resource(char *path,char *param)
{
	//uintptr_t caller = (uintptr_t)__builtin_return_address(0);
	//logv_error("[%p] lump_find_resource: path=%s, param=%s",caller, path, param);

	lmp_entry* res = SO_CONTINUE(lmp_entry*, lump_find_resource_hook,  path, param);
	//logv_error("[%p] lump_find_resource: path=%s, param=%s res=0x%X", caller, path, param, res);

	return res;
}

int get_dialog_file_size()
{
	char query[126];
	logv_error("world=%s", g_currentWorldName);
	sprintf(query, "%s.lmp", g_currentWorldName);
	
	lmp_entry* entry = lump_find_resource(query, "dialog.bin");

	return entry->len;
}

so_hook snd_get_dialog_dir_hook;

dialog_dir* snd_get_dialog_dir()
{
	uintptr_t caller = (uintptr_t)__builtin_return_address(0);
	logv_error("[%p]snd_get_dialog_dir called. g_curLanguage=(0x%X)", caller, g_curLanguage);

	dialog_dir* res = SO_CONTINUE(dialog_dir*, snd_get_dialog_dir_hook);

	logv_error("[%p]snd_get_dialog_dir returned=0x%X",caller, res);
	return res;

	int totalFileSize = get_dialog_file_size();

	// First pass: count entries
	dialog_dir* entry = res;
	int count = 0;
	while (entry->name[0] != '\0') {
		entry++;
		count++;
	}
	
	logv_error("Found %d dialog entries for world %s", count, g_currentWorldName);

	// Second pass: fix lengths by calculating from next entry's start
	entry = res;
	for (int i = 0; i < count; i++) {
		if (entry->len ==0) {
			int calculatedLen;
			
			if (i < count - 1) {
				// Calculate length from next entry's start
				dialog_dir* next_entry = entry + 1;
				calculatedLen = next_entry->startAt - entry->startAt;
			} else {
				if (strcmp(g_currentWorldName,"cellar1") == 0){
					calculatedLen = 0xC53FC;
				}
				//calculatedLen = totalFileSize - entry->startAt;
			}
			
			// Fix the len field
			entry->len = calculatedLen;
			
			logv_error("entry[%d]: name=%s, startAt=0x%X, unk=0x%X, len=0x%X (fixed)", 
					i, entry->name, entry->startAt, entry->unk, entry->len);
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

so_hook x_get_language_hook;
uint32_t x_get_language()
{
	uint32_t res = SO_CONTINUE(void*, x_get_language_hook);
	logv_error("x_get_language ret=0x%X", res);

	return res;
}

so_hook snd_get_dialog_filename_hook;
char* snd_get_dialog_filename(uint8_t loc)
{
	logv_error("1) g_SoundFilePath = %s", g_SoundFilePath);
	char* res = SO_CONTINUE(char*, snd_get_dialog_filename_hook, loc);

	logv_error("snd_get_dialog_filename (%x) ret=%s", loc, res);
	logv_error("2) g_SoundFilePath = %s", g_SoundFilePath);
	return res;
}

so_hook ov_raw_seek_hook;
int ov_raw_seek_local(void *vf, long pos)
{
    int res = SO_CONTINUE(int, ov_raw_seek_hook, vf, pos);
    return res;
}


so_hook ov_pcm_total_hook;
uint64_t ov_pcm_total_local(void* pf, long x)
{

    //SO_CONTINUE(int, ov_raw_seek_hook, pf, 0);

    uint64_t res = SO_CONTINUE(uint64_t, ov_pcm_total_hook, pf, x);

    logv_error("ov_pcm_total called(%p, %d)=%ld", pf, x, res);

    return res;
}

typedef struct
{
  int version;
  int channels;
  long rate;
  
  long bitrate_upper;
  long bitrate_nominal;
  long bitrate_lower;
  long bitrate_window;

  void *codec_setup;
}vorbis_info;

so_hook ov_info_hook;
vorbis_info *ov_info_local(void *vf, int link)
{
    vorbis_info* res = SO_CONTINUE(vorbis_info*, ov_info_hook, vf, link);

    logv_error("ov_info_local called(%p, %d)=channels%d", vf, link, res->channels);

    return res;
}

so_hook ogg_stream_hook;

// Match the exact signature from Ghidra
typedef void* (*OggStreamConstructor)(void* thisptr, char* filePath, int* startSample, int* length, int channelId);

void ogg_stream_patched(void* thisptr, char* filename, int* param_2, int* param_3, int param_4) {
    // Log parameters for debugging
    logv_error("OggStream: this=%p, file=%s, p2=%p, p3=%p, p4=%d\n", 
           thisptr, filename, param_2, param_3, param_4);
    
    // Restore original instructions
    kuKernelCpuUnrestrictedMemcpy((void *)ogg_stream_hook.addr, 
                                  ogg_stream_hook.orig_instr, 
                                  sizeof(ogg_stream_hook.orig_instr));
    kuKernelFlushCaches((void *)ogg_stream_hook.addr, sizeof(ogg_stream_hook.orig_instr));
    
    // Call original - use addr directly if it's ARM code
    OggStreamConstructor orig_func = (OggStreamConstructor)ogg_stream_hook.addr;
    void* r = orig_func(thisptr, filename, param_2, param_3, param_4);
    
    logv_error("OggStream returned: %p\n", r);
    
    // Re-apply hook
    kuKernelCpuUnrestrictedMemcpy((void *)ogg_stream_hook.addr, 
                                  ogg_stream_hook.patch_instr, 
                                  sizeof(ogg_stream_hook.patch_instr));
    kuKernelFlushCaches((void *)ogg_stream_hook.addr, sizeof(ogg_stream_hook.patch_instr));
    
   // return r;
}


void patch_vorbis(void) {
	// hook_addr(so_symbol(&so_mod, "vorbis_analysis"), (uintptr_t)vorbis_analysis);
	// hook_addr(so_symbol(&so_mod, "vorbis_analysis_blockout"), (uintptr_t)vorbis_analysis_blockout);
	// hook_addr(so_symbol(&so_mod, "vorbis_analysis_buffer"), (uintptr_t)vorbis_analysis_buffer);
	// hook_addr(so_symbol(&so_mod, "vorbis_analysis_headerout"), (uintptr_t)vorbis_analysis_headerout);
	// hook_addr(so_symbol(&so_mod, "vorbis_analysis_init"), (uintptr_t)vorbis_analysis_init);
	// hook_addr(so_symbol(&so_mod, "vorbis_analysis_wrote"), (uintptr_t)vorbis_analysis_wrote);
	// hook_addr(so_symbol(&so_mod, "vorbis_bitrate_addblock"), (uintptr_t)vorbis_bitrate_addblock);
	// hook_addr(so_symbol(&so_mod, "vorbis_bitrate_flushpacket"), (uintptr_t)vorbis_bitrate_flushpacket);
	// hook_addr(so_symbol(&so_mod, "vorbis_block_clear"), (uintptr_t)vorbis_block_clear);
	// hook_addr(so_symbol(&so_mod, "vorbis_block_init"), (uintptr_t)vorbis_block_init);
	// hook_addr(so_symbol(&so_mod, "vorbis_comment_add"), (uintptr_t)vorbis_comment_add);
	// hook_addr(so_symbol(&so_mod, "vorbis_comment_add_tag"), (uintptr_t)vorbis_comment_add_tag);
	// hook_addr(so_symbol(&so_mod, "vorbis_comment_clear"), (uintptr_t)vorbis_comment_clear);
	// hook_addr(so_symbol(&so_mod, "vorbis_comment_init"), (uintptr_t)vorbis_comment_init);
	// hook_addr(so_symbol(&so_mod, "vorbis_comment_query"), (uintptr_t)vorbis_comment_query);
	// hook_addr(so_symbol(&so_mod, "vorbis_comment_query_count"), (uintptr_t)vorbis_comment_query_count);
	// hook_addr(so_symbol(&so_mod, "vorbis_commentheader_out"), (uintptr_t)vorbis_commentheader_out);
	// hook_addr(so_symbol(&so_mod, "vorbis_dsp_clear"), (uintptr_t)vorbis_dsp_clear);
	// hook_addr(so_symbol(&so_mod, "vorbis_info_blocksize"), (uintptr_t)vorbis_info_blocksize);
	// hook_addr(so_symbol(&so_mod, "vorbis_info_clear"), (uintptr_t)vorbis_info_clear);
	// hook_addr(so_symbol(&so_mod, "vorbis_info_init"), (uintptr_t)vorbis_info_init);
	// hook_addr(so_symbol(&so_mod, "vorbis_packet_blocksize"), (uintptr_t)vorbis_packet_blocksize);
	// hook_addr(so_symbol(&so_mod, "vorbis_synthesis"), (uintptr_t)vorbis_synthesis);
	// hook_addr(so_symbol(&so_mod, "vorbis_synthesis_blockin"), (uintptr_t)vorbis_synthesis_blockin);
	// hook_addr(so_symbol(&so_mod, "vorbis_synthesis_headerin"), (uintptr_t)vorbis_synthesis_headerin);
	// hook_addr(so_symbol(&so_mod, "vorbis_synthesis_init"), (uintptr_t)vorbis_synthesis_init);
	// hook_addr(so_symbol(&so_mod, "vorbis_synthesis_pcmout"), (uintptr_t)vorbis_synthesis_pcmout);
	// hook_addr(so_symbol(&so_mod, "vorbis_synthesis_read"), (uintptr_t)vorbis_synthesis_read);
	// hook_addr(so_symbol(&so_mod, "vorbis_synthesis_trackonly"), (uintptr_t)vorbis_synthesis_trackonly);
	//ov_read_hook = hook_addr(so_symbol(&so_mod, "ov_read"), (uintptr_t)ov_read_profiled);
	snd_start_stream_hook = hook_addr(so_symbol(&so_mod, "_Z15SND_StartStreamiPKciii"), (uintptr_t)snd_start_stream);
	snd_start_music_hook = hook_addr(so_symbol(&so_mod, "_Z14SND_StartMusicPKci"), (uintptr_t)snd_start_music);
	//lump_find_resource_hook = hook_addr(so_symbol(&so_mod, "_Z16lumpFindResourcePKcS0_"), (uintptr_t)lump_find_resource);
	snd_get_dialog_dir_hook = hook_addr(so_symbol(&so_mod, "_Z16SND_GetDialogDirv"), (uintptr_t)snd_get_dialog_dir);
	lump_query_hook = hook_addr(so_symbol(&so_mod, "_Z9lumpQueryPKc"), (uintptr_t)lump_query);
	//ram_fs_open_hook = hook_addr(so_symbol(&so_mod, "_ZN3JBE4File6RamFSs4OpenEPKcRj"), (uintptr_t)ram_fs_open);
	x_get_language_hook = hook_addr(so_symbol(&so_mod, "XGetLanguage"), (uintptr_t)x_get_language);
	snd_get_dialog_filename_hook = hook_addr(so_symbol(&so_mod, "_Z21SND_GetDialogFilenameb"), (uintptr_t)snd_get_dialog_filename);

	ov_pcm_total_hook = hook_addr(so_symbol(&so_mod, "ov_pcm_total"), (uintptr_t)ov_pcm_total_local);
    ov_info_hook = hook_addr(so_symbol(&so_mod, "ov_info"), (uintptr_t)ov_info_local);
   // ov_raw_seek_hook = hook_addr(so_symbol(&so_mod, "ov_raw_seek"), (uintptr_t)ov_raw_seek_local);

	//ogg_stream_hook = hook_addr(so_symbol(&so_mod, "_ZN9OggStreamC2EPKcRiS2_i"), (uintptr_t)ogg_stream_patched);

	g_currentWorldName = (char*)LOC(0x054cad8);
	g_SoundFilePath = (char*)LOC(0x003e7758);
	
	int* ptr2 = LOC(0x0010a8b4);
	g_curLanguage = *ptr2;

	//create_file_a_hook = hook_addr(so_symbol(&so_mod, "CreateFileA"), (uintptr_t)create_file_a);


	//0010f154
	//ov_read_callback_hook = hook_addr(LOC(0x0010f154), (uintptr_t)&ov_read_callback);
	SoundSystem* ptr = (SoundSystem*)LOC(0x003dfc98);
	g_soundSystemBase = *ptr;


	// Hook SND_Frame to report ov_read stats per frame
	//snd_frame_hook = hook_addr(so_symbol(&so_mod, "_Z9SND_Framev"), (uintptr_t)&snd_frame_profiled);
}