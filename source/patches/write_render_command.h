#ifndef WRITE_RENDER_COMMAND_H
#define WRITE_RENDER_COMMAND_H

#include <so_util/so_util.h>

typedef struct {
    void* field0_0x0;
    uint32_t size;
} ParamBlock;

typedef struct {
    void* cmdBufferWritePtr;
    int frameNumberOfCmdWritter;
    void* cmdBufferStart;
    int frameNumberOfCmdReader;
    void* tmpCmdWrite;
	uint8_t unknown[14]; // padding or unknown data
	uint8_t frameBitToToggle;
	uint8_t unknown2;
    void* field16_0x24;
    void* cmdBufferEnd;
    int commandSize;
} D3DDevice_2;

_Static_assert(offsetof(D3DDevice_2, tmpCmdWrite) == 0x10,
			   "`tmpCmdWrite` is not at offset 0x10 – fix the struct or add packed!");
// assert that field16_0x24 is at offset 0x24
_Static_assert(offsetof(D3DDevice_2, field16_0x24) == 0x24,
			   "`field16_0x24` is not at offset 0x24 – fix the struct or add packed!");

so_hook WriteCommand_hook;
void WriteCommand_Optimized(D3DDevice_2* this, 
                                     const int* param_1, 
                                     const ParamBlock* paramsBlock, 
                                     const uint32_t* param_3) {
	// Cache frequently accessed struct members to reduce pointer dereferencing
	uint32_t *cmdBufWritePtr = (uint32_t *)this->cmdBufferWritePtr;
	uint32_t *cmdBufferStart = (uint32_t *)this->cmdBufferStart;
	uint32_t *cmdBufferEnd = (uint32_t *)this->cmdBufferEnd;
	int cmdWritterFrameNum = this->frameNumberOfCmdWritter;
	int cmdReaderFrameNum = this->frameNumberOfCmdReader;
	
	// Pre-calculate total size once
	const uint32_t paramSize = paramsBlock->size;
	const int totalSizeWords = ((paramSize + 3U) >> 2) + 4;
	
	// Fast path for common case: totalSizeWords = 8 (most frequent)
	if (__builtin_expect(totalSizeWords == 8, 1)) {
		// Fast buffer check for size 8
		if (__builtin_expect(cmdBufferEnd >= cmdBufWritePtr + 8, 1)) {
			// Common case: sufficient buffer space available
			this->commandSize = 8;
			
			// Direct sequential writes for optimal cache performance
			cmdBufWritePtr[0] = 0x819; // (8 << 8) | 0x19
			cmdBufWritePtr[1] = *param_1;
			
			const uint32_t paramWords = (paramSize + 3U) >> 2;
			cmdBufWritePtr[2] = paramWords;
			
			// Optimized copy for common param sizes: 64, 128, 192 bytes
			const uint32_t* src = (const uint32_t*)paramsBlock->field0_0x0;
			uint32_t* dst = &cmdBufWritePtr[3];
			
			// Fast unrolled copies for common sizes
			if (__builtin_expect(paramSize == 64, 1)) {
				// 64 bytes = 16 words - unrolled copy
				dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
				dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
				dst[8] = src[8]; dst[9] = src[9]; dst[10] = src[10]; dst[11] = src[11];
				dst[12] = src[12]; dst[13] = src[13]; dst[14] = src[14]; dst[15] = src[15];
			} else if (__builtin_expect(paramSize == 128, 1)) {
				// 128 bytes = 32 words - unrolled copy in blocks
				for (uint32_t i = 0; i < 32; i += 8) {
					dst[i] = src[i]; dst[i+1] = src[i+1]; dst[i+2] = src[i+2]; dst[i+3] = src[i+3];
					dst[i+4] = src[i+4]; dst[i+5] = src[i+5]; dst[i+6] = src[i+6]; dst[i+7] = src[i+7];
				}
			} else if (__builtin_expect(paramSize == 192, 1)) {
				// 192 bytes = 48 words - unrolled copy in blocks
				for (uint32_t i = 0; i < 48; i += 8) {
					dst[i] = src[i]; dst[i+1] = src[i+1]; dst[i+2] = src[i+2]; dst[i+3] = src[i+3];
					dst[i+4] = src[i+4]; dst[i+5] = src[i+5]; dst[i+6] = src[i+6]; dst[i+7] = src[i+7];
				}
			} else {
				// Fallback for other sizes
				__aeabi_memcpy(dst, paramsBlock->field0_0x0, paramSize);
			}
			
			cmdBufWritePtr[3 + paramWords] = *param_3;
			
			// Update pointers with single calculation
			this->tmpCmdWrite = (void*)&cmdBufWritePtr[4 + paramWords];
			this->cmdBufferWritePtr = (void*)(cmdBufWritePtr + 8);
			return;
		}
	}
	
	// Slow path for buffer wrapping or non-standard sizes
	if (__builtin_expect(cmdBufferEnd < cmdBufWritePtr + totalSizeWords, 0))
	{
		// Wait for buffer space, using cached frame numbers
		while ((cmdWritterFrameNum != cmdReaderFrameNum) && (cmdBufWritePtr == cmdBufferStart)) 
		{
			usleep(1000);
			cmdWritterFrameNum = this->frameNumberOfCmdWritter;
			cmdReaderFrameNum = this->frameNumberOfCmdReader;
			cmdBufWritePtr = (uint32_t *)this->cmdBufferWritePtr;
		}

		*cmdBufWritePtr = 10;
		cmdBufWritePtr = (uint32_t *)this->field16_0x24;
		cmdWritterFrameNum++;
		
		// Update struct members once
		this->cmdBufferWritePtr = cmdBufWritePtr;
		this->frameNumberOfCmdWritter = cmdWritterFrameNum;
	}
	
	// Set command size and temp write pointer
	this->commandSize = totalSizeWords;
	uint32_t *tmpCmdWrite = cmdBufWritePtr;
	
	// Optimized frame sync check
	if (__builtin_expect(cmdWritterFrameNum != cmdReaderFrameNum, 0)) 
	{
		uint32_t *cmdEndPtr = cmdBufWritePtr + totalSizeWords;
		do
		{
			if ((cmdEndPtr <= cmdBufferStart) || (cmdBufferStart < cmdBufWritePtr)) break;
			usleep(1000);
			tmpCmdWrite = (uint32_t *)this->tmpCmdWrite;
		} while (this->frameNumberOfCmdWritter != this->frameNumberOfCmdReader);
		cmdBufWritePtr = tmpCmdWrite;
	}
	
	// Write command header efficiently
	tmpCmdWrite = cmdBufWritePtr + 1;
	*cmdBufWritePtr = (totalSizeWords << 8) | 0x19;
	
	// Write param_1
	*tmpCmdWrite = *param_1;
	tmpCmdWrite++;
	
	// Calculate words needed for params, cache the calculation
	const uint32_t paramWords = (paramSize + 3U) >> 2;
	*tmpCmdWrite = paramWords;
	tmpCmdWrite++;
	
	// Direct memory copy using cached pointers - optimized for common sizes
	const uint32_t* src = (const uint32_t*)paramsBlock->field0_0x0;
	uint32_t* dst = tmpCmdWrite;
	
	if (__builtin_expect(paramSize == 64, 1)) {
		// 64 bytes = 16 words - unrolled copy
		dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
		dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
		dst[8] = src[8]; dst[9] = src[9]; dst[10] = src[10]; dst[11] = src[11];
		dst[12] = src[12]; dst[13] = src[13]; dst[14] = src[14]; dst[15] = src[15];
	} else if (__builtin_expect(paramSize == 128, 1)) {
		// 128 bytes = 32 words - unrolled copy in blocks
		for (uint32_t i = 0; i < 32; i += 8) {
			dst[i] = src[i]; dst[i+1] = src[i+1]; dst[i+2] = src[i+2]; dst[i+3] = src[i+3];
			dst[i+4] = src[i+4]; dst[i+5] = src[i+5]; dst[i+6] = src[i+6]; dst[i+7] = src[i+7];
		}
	} else if (__builtin_expect(paramSize == 192, 1)) {
		// 192 bytes = 48 words - unrolled copy in blocks
		for (uint32_t i = 0; i < 48; i += 8) {
			dst[i] = src[i]; dst[i+1] = src[i+1]; dst[i+2] = src[i+2]; dst[i+3] = src[i+3];
			dst[i+4] = src[i+4]; dst[i+5] = src[i+5]; dst[i+6] = src[i+6]; dst[i+7] = src[i+7];
		}
	} else {
		// Fallback for other sizes
		__aeabi_memcpy(dst, paramsBlock->field0_0x0, paramSize);
	}
	tmpCmdWrite += paramWords;
	
	// Write final parameter
	*tmpCmdWrite = *param_3;
	tmpCmdWrite++;
	
	// Update struct members with final values
	this->tmpCmdWrite = (void*)tmpCmdWrite;
	this->cmdBufferWritePtr = (void *)((uint8_t*)this->cmdBufferWritePtr + totalSizeWords * 4);
}

#endif