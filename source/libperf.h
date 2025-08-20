/*                 -*- mode: c; tab-width: 4; indent-tabs-mode: t; -*- */
/* SCE CONFIDENTIAL
 PlayStation(R)Vita Programmer Tool Runtime Library Release 03.570.011
 *
 *      Copyright (C) 2012 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 */
/*
 * 
 * 
 * 
 * 
 * 
 */

#ifndef _SCE_PERF_LIBPERF_H
#define _SCE_PERF_LIBPERF_H

//#include <kernel/types.h>

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif	/* defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus) */

/**
 * Error number definition
 */
#define SCE_PERF_ERROR_INVALID_ARGUMENT			-2141716480	/* 0x80580000 */
#define SCE_PERF_ERROR_BAD_TRACE_DATA			-2141716478 /* 0x80580002 */
#define SCE_PERF_ERROR_POP_WITHOUT_PUSH			-2141716477 /* 0x80580003 */
#define SCE_PERF_ERROR_TOO_MANY_PUSHES			-2141716476 /* 0x80580004 */
#define SCE_PERF_ERROR_NOT_INITIALIZED			-2141716475 /* 0x80580005 */
#define SCE_PERF_ERROR_ALREADY_STARTED			-2141716474 /* 0x80580006 */
#define SCE_PERF_ERROR_CANNOT_START				-2141716473 /* 0x80580007 */
#define SCE_PERF_ERROR_ALREADY_STOPPED			-2141716472 /* 0x80580008 */
#define SCE_PERF_ERROR_CANNOT_STOP				-2141716461 /* 0x80580009 */

/**
 * Counter number
 */
#define SCE_PERF_ARM_PMON_CYCLE_COUNTER			0x0000001FU	/* 31 */
#define SCE_PERF_ARM_PMON_COUNTER_5				0x00000005U	/*  5 */
#define SCE_PERF_ARM_PMON_COUNTER_4				0x00000004U	/*  4 */
#define SCE_PERF_ARM_PMON_COUNTER_3				0x00000003U	/*  3 */
#define SCE_PERF_ARM_PMON_COUNTER_2				0x00000002U	/*  2 */
#define SCE_PERF_ARM_PMON_COUNTER_1				0x00000001U /*  1 */
#define SCE_PERF_ARM_PMON_COUNTER_0				0x00000000U	/*  0 */

#define SCE_PERF_ARM_PMON_PMCNT_NUM				0x00000006U	/*  6 */

/**
 * Event Counter mask
 */
#define SCE_PERF_ARM_PMON_COUNTER_MASK_5			0x00000020U
#define SCE_PERF_ARM_PMON_COUNTER_MASK_4			0x00000010U
#define SCE_PERF_ARM_PMON_COUNTER_MASK_3			0x00000008U
#define SCE_PERF_ARM_PMON_COUNTER_MASK_2			0x00000004U
#define SCE_PERF_ARM_PMON_COUNTER_MASK_1			0x00000002U
#define SCE_PERF_ARM_PMON_COUNTER_MASK_0			0x00000001U

#define SCE_PERF_ARM_PMON_COUNTER_MASK_ALL			0x0000003fU

/**
 * Performance Counter Event
 */
#define SCE_PERF_ARM_PMON_SOFT_INCREMENT			0x00U
#define SCE_PERF_ARM_PMON_ICACHE_MISS				0x01U
#define SCE_PERF_ARM_PMON_ITLB_MISS					0x02U
#define SCE_PERF_ARM_PMON_DCACHE_MISS				0x03U
#define SCE_PERF_ARM_PMON_DCACHE_ACCESS				0x04U
#define SCE_PERF_ARM_PMON_DTLB_MISS					0x05U
#define SCE_PERF_ARM_PMON_DATA_READ					0x06U
#define SCE_PERF_ARM_PMON_DATA_WRITE				0x07U
#define SCE_PERF_ARM_PMON_EXCEPTION_TAKEN			0x09U
#define SCE_PERF_ARM_PMON_EXCEPTION_RETURN			0x0AU
#define SCE_PERF_ARM_PMON_WRITE_CONTEXTID			0x0BU
#define SCE_PERF_ARM_PMON_SOFT_CHANGEPC				0x0CU
#define SCE_PERF_ARM_PMON_IMMEDIATE_BRANCH			0x0DU
#define SCE_PERF_ARM_PMON_UNALIGNED					0x0FU
#define SCE_PERF_ARM_PMON_BRANCH_MISPREDICT			0x10U
#define SCE_PERF_ARM_PMON_PREDICT_BRANCH			0x12U
#define SCE_PERF_ARM_PMON_COHERENT_LF_MISS			0x50U
#define SCE_PERF_ARM_PMON_COHERENT_LF_HIT			0x51U
#define SCE_PERF_ARM_PMON_ICACHE_STALL				0x60U
#define SCE_PERF_ARM_PMON_DCACHE_STALL				0x61U
#define SCE_PERF_ARM_PMON_MAINTLB_STALL				0x62U
#define SCE_PERF_ARM_PMON_STREX_PASSED				0x63U
#define SCE_PERF_ARM_PMON_STREX_FAILED				0x64U
#define SCE_PERF_ARM_PMON_DATA_EVICTION				0x65U
#define SCE_PERF_ARM_PMON_ISSUE_NO_DISPATCH			0x66U
#define SCE_PERF_ARM_PMON_ISSUE_EMPTY				0x67U
#define SCE_PERF_ARM_PMON_INST_RENAME				0x68U
#define SCE_PERF_ARM_PMON_PREDICT_FUNC_RET			0x6EU
#define SCE_PERF_ARM_PMON_MAIN_PIPE					0x70U
#define SCE_PERF_ARM_PMON_SECOND_PIPE				0x71U
#define SCE_PERF_ARM_PMON_LS_PIPE					0x72U
#define SCE_PERF_ARM_PMON_FPU_RENAME				0x73U
#define SCE_PERF_ARM_PMON_NEON_RENAME				0x74U	/* The H/W bug on this event is fixed at ES4.0 */
#define SCE_PERF_ARM_PMON_PLD_STALL					0x80U
#define SCE_PERF_ARM_PMON_WRITE_STALL				0x81U
#define SCE_PERF_ARM_PMON_INST_MAINTLB_STALL		0x82U
#define SCE_PERF_ARM_PMON_DATA_MAINTLB_STALL		0x83U
#define SCE_PERF_ARM_PMON_INST_UTLB_STALL			0x84U
#define SCE_PERF_ARM_PMON_DATA_UTLB_STALL			0x85U
#define SCE_PERF_ARM_PMON_DMB_STALL					0x86U
#define SCE_PERF_ARM_PMON_INTEGER_CLOCK				0x8AU
#define SCE_PERF_ARM_PMON_DATAENGINE_CLOCK			0x8BU
#define SCE_PERF_ARM_PMON_ISB						0x90U
#define SCE_PERF_ARM_PMON_DSB						0x91U
#define SCE_PERF_ARM_PMON_DMB						0x92U
#define SCE_PERF_ARM_PMON_EXT_INTERRUPT				0x93U
#define SCE_PERF_ARM_PMON_PLE_LINE_REQ_COMPLETED	0xA0U
#define SCE_PERF_ARM_PMON_PLE_CHANNEL_SKIPPED		0xA1U
#define SCE_PERF_ARM_PMON_PLE_FIFO_FLUSH			0xA2U
#define SCE_PERF_ARM_PMON_PLE_REQ_COMPLETED			0xA3U
#define SCE_PERF_ARM_PMON_PLE_FIFO_OVERFLOW			0xA4U
#define SCE_PERF_ARM_PMON_PLE_REQ_PROGRAMMED		0xA5U

/**
 * Thread ID
 */
#define SCE_PERF_ARM_PMON_THREAD_ID_SELF		((SceUID)0x00000000)
#define SCE_PERF_ARM_PMON_THREAD_ID_ALL			SCE_UID_THREAD_ID_PROCESS_ALL

/**
 * Prototype Declaration (Public API)
 */
typedef unsigned int SceUInt32;
typedef unsigned long long SceUInt64;


// int scePerfArmPmonReset(SceUID threadId);
// int scePerfArmPmonSelectEvent(SceUID threadId, SceUInt32 counter, SceUInt8 eventCode);
// int scePerfArmPmonStart(SceUID threadId);
// int scePerfArmPmonStop(SceUID threadId);
// int scePerfArmPmonGetCounterValue(SceUID threadId, SceUInt32 counter, SceUInt32 *pValue);
// int scePerfArmPmonSetCounterValue(SceUID threadId, SceUInt32 counter, SceUInt32 value);
// int scePerfArmPmonSoftwareIncrement(SceUInt32 mask);

SceUInt64 scePerfGetTimebaseValue(void);
SceUInt32 scePerfGetTimebaseFrequency(void);

/**
 * C Preprocessor macros for Own thread access
 */
#define SCE_PERF_ARM_PMON_START_ALL() { \
	SceUInt32 __sce_temp = 0x8000003FU; \
    __builtin_mcr(15, 0, 9, 12, 1, __sce_temp); \
}

#define SCE_PERF_ARM_PMON_STOP_ALL() { \
	SceUInt32 __sce_temp = 0x8000003FU; \
    __builtin_mcr(15, 0, 9, 12, 2, __sce_temp); \
}

#define SCE_PERF_ARM_PMON_SELECT_EVENT_COUNTER(arg) __builtin_mcr(15, 0, 9, 12, 5, arg)
#define SCE_PERF_ARM_PMON_GET_EVENT_COUNTER()       __builtin_mrc(15, 0, 9, 13, 2)
#define SCE_PERF_ARM_PMON_GET_CYCLE_COUNTER()       __builtin_mrc(15, 0, 9, 13, 0)

#define SCE_RAZOR_MARKER_DISABLE_HUD		0
#define SCE_RAZOR_MARKER_ENABLE_HUD			1

#define SCE_RAZOR_NOT_CAPTURING		0
#define SCE_RAZOR_CAPTURING			1

#define SCE_RAZOR_COLOR_RED			0x800000ff
#define SCE_RAZOR_COLOR_GREEN		0x8000ff00
#define SCE_RAZOR_COLOR_BLUE		0x80ff0000
#define SCE_RAZOR_COLOR_YELLOW		0x8000ffff
#define SCE_RAZOR_COLOR_MAGENTA		0x80ff00ff
#define SCE_RAZOR_COLOR_CYAN		0x80ffff00
#define SCE_RAZOR_COLOR_WHITE		0x80ffffff
#define SCE_RAZOR_COLOR_BLACK		0x80000000

typedef struct
{
	SceUInt32 header;
	SceUInt32 threadId;
	SceUInt32 stackLevel;
	SceUInt32 color;
	SceUInt64 timestamp;
} SceRazorCpuUserMarkerTracePacket;

typedef struct
{
	SceUInt64 system;
	SceUInt64 idle;
	SceUInt64 user;
	SceUInt64 frameIndex;
	SceUInt64 cpuId;
} SceRazorCpuActivityMonitorPacket;

int sceRazorCpuPushMarker(const char* szLabel) __attribute__ ((deprecated));
int sceRazorCpuPushMarkerWithHud( const char* szLabel, SceUInt32 color, SceUInt32 flags );
int sceRazorCpuPopMarker(void);
int sceRazorCpuSync(void);
int sceRazorCpuStartCapture(void);
int sceRazorCpuStopCapture(void);
SceUInt32 sceRazorCpuIsCapturing(void);

int sceRazorCpuStartUserMarkerTrace( void* pBufferBase, SceUInt32 bufferSize );
int sceRazorCpuStopUserMarkerTrace(void);
int sceRazorCpuStartActivityMonitor( void* pBufferBase, SceUInt32 bufferSize );
int sceRazorCpuStopActivityMonitor(void);
int sceRazorCpuGetUserMarkerTraceBuffer( SceRazorCpuUserMarkerTracePacket **pTrace );
int sceRazorCpuGetActivityMonitorTraceBuffer( SceRazorCpuActivityMonitorPacket **pTrace );

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif	/* defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus) */

#endif	/* _SCE_PERF_LIBPERF_H */


/* Local variables: */
/* tab-width: 4 */
/* End: */
/* vi:set tabstop=4: */