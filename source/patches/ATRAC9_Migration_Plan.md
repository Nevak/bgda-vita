# ATRAC9 Migration Implementation Plan

## Goal
Replace slow software Vorbis decoding with hardware-accelerated ATRAC9 decoding using function hooking only (no game code modification).

## Performance Benefit
- **Current**: Software Vorbis ~20ms decode/frame → ~30% CPU usage per stream
- **Target**: Hardware ATRAC9 ~0.5ms decode/frame → <1% CPU usage per stream
- **Speedup**: ~40x faster decoding

---

## Constraint: Hook-Only Approach

We can only:
- ✅ Hook/replace entire functions using `hook_addr()`
- ✅ Create wrapper types that mimic original structures
- ✅ Intercept allocations and substitute our own objects
- ❌ Modify the original game code
- ❌ Change the calling conventions

---

## Phase 1: Create ATRAC9 Infrastructure and Types

### 1.1 Create AT9Stream Structure (Replaces OggStream)

**File**: `source/patches/atrac9_stream.h`

```c
#ifndef __ATRAC9_STREAM_H__
#define __ATRAC9_STREAM_H__

#include <stdint.h>
#include <psp2/audioout.h>
#include <psp2/atrac.h>

// Mimics OggStream structure size (0x2f8 bytes)
// Game expects this size when it calls operator_new(0x2f8)
typedef struct AT9Stream {
    // === Header (must match OggStream layout for compatibility) ===
    void* vtable;                    // 0x000 - Virtual function table (for destructor)

    // === File info ===
    char filepath[256];              // 0x004 - Full path to .at9 file
    int channelId;                   // 0x104 - Channel ID (0=music, 1=sfx, 2=dialog)

    // === Sample positions ===
    int startSample;                 // 0x108 - Start sample position
    int endSample;                   // 0x10C - End sample position
    int currentSample;               // 0x110 - Current playback position

    // === ATRAC9 decoder state ===
    SceAtracDecoderGroup decoderGroup; // 0x114 - Decoder group handle
    uint32_t atracHandle;            // 0x120 - ATRAC decoder handle
    void* workMem;                   // 0x124 - Work memory for decoder (1-16KB)
    uint32_t workMemSize;            // 0x128 - Work memory size

    // === Stream buffers ===
    uint8_t* mainBuffer;             // 0x12C - Main streaming buffer (16KB, 256-byte aligned)
    uint32_t mainBufferSize;         // 0x130 - Main buffer size
    uint8_t* outputBuffer;           // 0x134 - PCM output buffer (2-4KB, 256-byte aligned)
    uint32_t outputBufferSize;       // 0x138 - Output buffer size

    // === File I/O (FIOS) ===
    int fiosHandle;                  // 0x13C - FIOS file handle
    uint64_t fileSize;               // 0x140 - Total file size
    uint64_t fileOffset;             // 0x148 - Current file read position

    // === Playback state ===
    uint32_t isPlaying;              // 0x150 - Playing flag
    uint32_t isPaused;               // 0x154 - Paused flag
    uint32_t decoderStatus;          // 0x158 - Last decoder status
    uint32_t samplesDecoded;         // 0x15C - Samples decoded in last frame

    // === Audio output ===
    int audioPort;                   // 0x160 - sceAudioOut port (-1 if not opened)
    int audioGrain;                  // 0x164 - Granularity (512 samples typical)

    // === Loop control ===
    int loopCount;                   // 0x168 - Loop count (-1 = infinite)
    int loopStart;                   // 0x16C - Loop start sample
    int loopEnd;                     // 0x170 - Loop end sample

    // === Padding to match OggStream size (0x2f8) ===
    uint8_t padding[0x188];          // 0x174 - Pad to 0x2f8 total
} AT9Stream;
_Static_assert(sizeof(AT9Stream) == 0x2f8, "AT9Stream must be 0x2f8 bytes to match OggStream");

// Function declarations
AT9Stream* AT9Stream_Create(const char* filepath, int* startSample, int* endSample, int channelId);
void AT9Stream_Destroy(AT9Stream* stream);
int AT9Stream_Decode(AT9Stream* stream);
void AT9Stream_Pause(AT9Stream* stream);
void AT9Stream_Resume(AT9Stream* stream);
void AT9Stream_Stop(AT9Stream* stream);
int AT9Stream_Seek(AT9Stream* stream, int samplePosition);

#endif // __ATRAC9_STREAM_H__
```

### 1.2 Create Global ATRAC9 Manager

**File**: `source/patches/atrac9_manager.h`

```c
#ifndef __ATRAC9_MANAGER_H__
#define __ATRAC9_MANAGER_H__

#include <psp2/atrac.h>
#include <fios/fios.h>

typedef struct {
    // FIOS initialization
    int fiosInitialized;
    SceFiosParams fiosParams;

    // Global decoder group (shared across all streams)
    SceAtracDecoderGroup sharedDecoderGroup;
    void* sharedWorkMem;
    uint32_t sharedWorkMemSize;

    // Audio output ports
    int musicPort;    // For 48kHz stereo music
    int dialogPort;   // For 24kHz mono dialog
    int sfxPort;      // For 24kHz stereo SFX

    // Statistics
    int totalStreamsCreated;
    int activeStreamCount;
    float totalDecodeTimeMs;

} ATRAC9Manager;

// Global instance
extern ATRAC9Manager g_atrac9Manager;

// Manager functions
void ATRAC9Manager_Init(void);
void ATRAC9Manager_Shutdown(void);

#endif // __ATRAC9_MANAGER_H__
```

### 1.3 Implementation Files

Create:
- `source/patches/atrac9_stream.c` - AT9Stream implementation
- `source/patches/atrac9_manager.c` - Manager implementation

---

## Phase 2: Hook SND_Init to Initialize ATRAC9 System

### 2.1 Hook SND_Init

**File**: `source/patch.c` (add to existing hooks)

```c
#include "patches/atrac9_manager.h"

so_hook SND_Init_hook;
void SND_Init_patched(float param_1) {
    // Initialize ATRAC9 system FIRST
    ATRAC9Manager_Init();

    // Then call original SND_Init (it will still create DirectSound buffers etc.)
    // We'll intercept the actual streaming later
    SO_CONTINUE(void, SND_Init_hook, param_1);

    logv_info("[ATRAC9] SND_Init completed with ATRAC9 backend");
}
```

**In `patch_function()` add**:
```c
SND_Init_hook = hook_addr(so_symbol(&so_mod, "_Z8SND_Initf"),
                          (uintptr_t)&SND_Init_patched);
```

### 2.2 ATRAC9Manager_Init Implementation

```c
void ATRAC9Manager_Init(void) {
    memset(&g_atrac9Manager, 0, sizeof(ATRAC9Manager));

    // 1. Initialize FIOS2 for async file I/O
    SceFiosParams params = SCE_FIOS_PARAMS_INITIALIZER;
    params.opStorage.pVprintf = sceClibPrintf;
    params.pathMax = 256;
    sceFiosInitialize(&params);
    g_atrac9Manager.fiosInitialized = 1;

    // 2. Create shared decoder group
    g_atrac9Manager.sharedDecoderGroup.size = sizeof(SceAtracDecoderGroup);
    g_atrac9Manager.sharedDecoderGroup.wordLength = 2; // 16-bit
    g_atrac9Manager.sharedDecoderGroup.totalCh = 6; // Support up to 3 stereo streams

    sceAtracQueryDecoderGroupMemSize(&g_atrac9Manager.sharedDecoderGroup,
                                     &g_atrac9Manager.sharedWorkMemSize);

    g_atrac9Manager.sharedWorkMem = memalign(256, g_atrac9Manager.sharedWorkMemSize);

    sceAtracCreateDecoderGroup(SCE_ATRAC_TYPE_AT9,
                               &g_atrac9Manager.sharedDecoderGroup,
                               g_atrac9Manager.sharedWorkMem,
                               g_atrac9Manager.sharedWorkMemSize);

    // 3. Open audio output ports
    g_atrac9Manager.musicPort = sceAudioOutOpenPort(
        SCE_AUDIO_OUT_PORT_TYPE_BGM, 512, 48000, SCE_AUDIO_OUT_MODE_STEREO);

    g_atrac9Manager.dialogPort = sceAudioOutOpenPort(
        SCE_AUDIO_OUT_PORT_TYPE_VOICE, 512, 24000, SCE_AUDIO_OUT_MODE_MONO);

    g_atrac9Manager.sfxPort = sceAudioOutOpenPort(
        SCE_AUDIO_OUT_PORT_TYPE_MAIN, 512, 24000, SCE_AUDIO_OUT_MODE_STEREO);

    logv_info("[ATRAC9] Manager initialized: ports=%d,%d,%d",
              g_atrac9Manager.musicPort, g_atrac9Manager.dialogPort,
              g_atrac9Manager.sfxPort);
}
```

---

## Phase 3: Hook OggStream Constructor to Create AT9Stream

### 3.1 Intercept operator_new for OggStream

The game calls `operator_new(0x2f8)` to allocate OggStream objects.
We'll hook the OggStream constructor instead.

**Find OggStream constructor**:
```bash
# In Ghidra, search for function that:
# - Takes size 0x2f8 as parameter
# - Calls operator_new
# - Initializes with filepath, samples, channelId
```

### 3.2 Hook OggStream Constructor

**File**: `source/patch.c`

```c
#include "patches/atrac9_stream.h"

so_hook OggStream_Constructor_hook;
void* OggStream_Constructor_patched(void* thisPtr, const char* filepath,
                                    int* startSample, int* endSample,
                                    int channelId) {
    // Convert OGG path to AT9 path
    char at9Path[256];
    strncpy(at9Path, filepath, 255);

    // Replace .ogg with .at9
    char* ext = strstr(at9Path, ".ogg");
    if (ext) {
        strcpy(ext, ".at9");
    }

    logv_info("[ATRAC9] Creating stream: %s (channel %d, samples %d-%d)",
              at9Path, channelId, *startSample, *endSample);

    // Create AT9Stream instead of OggStream
    AT9Stream* at9Stream = AT9Stream_Create(at9Path, startSample, endSample, channelId);

    // Return as OggStream* (same size, game won't know the difference)
    return (void*)at9Stream;
}
```

**In `patch_function()` add**:
```c
// Find OggStream constructor address in Ghidra first
uintptr_t oggStreamCtor = so_symbol(&so_mod, "_ZN9OggStreamC2EPKcPiS2_i");
OggStream_Constructor_hook = hook_addr(oggStreamCtor,
                                       (uintptr_t)&OggStream_Constructor_patched);
```

### 3.3 Implement AT9Stream_Create

**File**: `source/patches/atrac9_stream.c`

```c
AT9Stream* AT9Stream_Create(const char* filepath, int* startSample,
                            int* endSample, int channelId) {
    // Allocate AT9Stream (same size as OggStream: 0x2f8)
    AT9Stream* stream = (AT9Stream*)memalign(64, sizeof(AT9Stream));
    memset(stream, 0, sizeof(AT9Stream));

    // Copy parameters
    strncpy(stream->filepath, filepath, 255);
    stream->channelId = channelId;
    stream->startSample = startSample ? *startSample : 0;
    stream->endSample = endSample ? *endSample : -1;
    stream->currentSample = stream->startSample;

    // Allocate buffers (256-byte aligned for ATRAC9)
    stream->mainBufferSize = 16 * 1024; // 16KB main buffer
    stream->mainBuffer = (uint8_t*)memalign(256, stream->mainBufferSize);

    stream->outputBufferSize = 512 * 2 * 2; // 512 samples × 2 channels × 2 bytes
    stream->outputBuffer = (uint8_t*)memalign(256, stream->outputBufferSize);

    // Open file with FIOS
    stream->fiosHandle = sceFiosFHOpenSync(NULL, NULL, filepath, NULL);
    if (stream->fiosHandle < 0) {
        logv_error("[ATRAC9] Failed to open file: %s (error: 0x%x)",
                   filepath, stream->fiosHandle);
        free(stream->mainBuffer);
        free(stream->outputBuffer);
        free(stream);
        return NULL;
    }

    // Get file size
    SceFiosStat stat;
    sceFiosFHStatSync(NULL, stream->fiosHandle, &stat);
    stream->fileSize = stat.st_size;

    // Read initial data into main buffer
    int64_t bytesRead = sceFiosFHReadSync(NULL, stream->fiosHandle,
                                          stream->mainBuffer, stream->mainBufferSize);

    // Set data and acquire ATRAC handle
    int ret = sceAtracSetDataAndAcquireHandle(&g_atrac9Manager.sharedDecoderGroup,
                                              stream->mainBuffer,
                                              stream->mainBufferSize,
                                              &stream->atracHandle);
    if (ret < 0) {
        logv_error("[ATRAC9] Failed to acquire ATRAC handle: 0x%x", ret);
        sceFiosFHCloseSync(NULL, stream->fiosHandle);
        free(stream->mainBuffer);
        free(stream->outputBuffer);
        free(stream);
        return NULL;
    }

    // Get audio port based on channel
    switch (channelId) {
        case 0: stream->audioPort = g_atrac9Manager.musicPort; break;
        case 1: stream->audioPort = g_atrac9Manager.sfxPort; break;
        case 2: stream->audioPort = g_atrac9Manager.dialogPort; break;
        default: stream->audioPort = -1; break;
    }

    stream->audioGrain = 512;
    stream->isPlaying = 1;
    stream->loopCount = 0;

    g_atrac9Manager.activeStreamCount++;
    g_atrac9Manager.totalStreamsCreated++;

    logv_info("[ATRAC9] Stream created successfully: handle=%d, port=%d",
              stream->atracHandle, stream->audioPort);

    return stream;
}
```

---

## Phase 4: Hook SND_Frame to Process ATRAC9 Streams

### 4.1 Hook SND_Frame

The game calls `SND_Frame()` every frame to update audio. We need to:
1. Process ATRAC9 streams (decode, refill buffers)
2. Call original SND_Frame for sound effects

**File**: `source/utils/vorbis_patch.c` (modify existing hook)

```c
#include "patches/atrac9_stream.h"

extern so_hook snd_frame_hook; // Already exists

void snd_frame_atrac9(void) {
    // Process all active ATRAC9 streams
    // (We'll iterate through the game's SoundChannel structures)

    // Call original SND_Frame
    SO_CONTINUE(void*, snd_frame_hook);
}
```

### 4.2 Implement AT9Stream_Decode

**File**: `source/patches/atrac9_stream.c`

```c
int AT9Stream_Decode(AT9Stream* stream) {
    if (!stream || !stream->isPlaying || stream->isPaused) {
        return 0;
    }

    // Decode one frame
    int ret = sceAtracDecode(stream->atracHandle,
                            stream->outputBuffer,
                            &stream->decoderStatus,
                            &stream->samplesDecoded);

    if (ret < 0) {
        logv_error("[ATRAC9] Decode error: 0x%x", ret);
        return -1;
    }

    // Check if we need more data
    if (stream->decoderStatus == SCE_ATRAC_DECODER_STATUS_NEED_DATA) {
        SceAtracStreamInfo streamInfo;
        sceAtracGetStreamInfo(stream->atracHandle, &streamInfo);

        // Read more data from file
        sceFiosFHSeek(stream->fiosHandle, streamInfo.readOffset, SCE_FIOS_SEEK_SET);
        int64_t bytesRead = sceFiosFHReadSync(NULL, stream->fiosHandle,
                                              stream->mainBuffer + streamInfo.pWritePosition,
                                              streamInfo.readSize);

        // Add stream data
        sceAtracAddStreamData(stream->atracHandle, bytesRead);
        stream->fileOffset += bytesRead;
    }

    // Check for end of stream
    if (stream->decoderStatus == SCE_ATRAC_DECODER_STATUS_END) {
        if (stream->loopCount != 0) {
            // Loop back to start
            AT9Stream_Seek(stream, stream->loopStart);
            if (stream->loopCount > 0) stream->loopCount--;
        } else {
            stream->isPlaying = 0;
            return 0;
        }
    }

    // Output PCM to audio port
    if (stream->audioPort >= 0 && stream->samplesDecoded > 0) {
        sceAudioOutOutput(stream->audioPort, stream->outputBuffer);
    }

    stream->currentSample += stream->samplesDecoded;

    return stream->samplesDecoded;
}
```

---

## Phase 5: Handle Stream Lifecycle (Destructor Only)

### 5.1 Reality Check: No Pause/Resume/Stop Methods

**VERIFIED IN GHIDRA**: OggStream does **NOT** have explicit Pause/Resume/Stop methods.

Instead, the game manages playback state through **flags** in the `SoundStreamSlot` structure:
- Offset `+0x20`: `flags` field
  - Bit 0: playing/active
  - Bit 1: fade command
  - Bit 2: unknown
  - Bit 3: paused

**SND_Frame** checks these flags and decides whether to call `ov_read()` based on the state.

### 5.2 What We Actually Need to Hook

**Only the destructor needs hooking**:

From `SND_Frame` decompilation (lines showing cleanup):
```c
if (this[0x2f0] != (File)0x0) {
    ov_clear(this + 0x20);  // Clear vorbis file
}
JBE::File::MakeSub(this,0,0);
JBE::File::~File(this);      // Call File destructor
operator_delete(this);        // Free memory
```

### 5.3 Hook File Destructor for AT9Stream

Since `OggStream` inherits from `JBE::File`, we need to handle cleanup when the File destructor is called:

**File**: `source/patch.c`

```c
so_hook JBE_File_Destructor_hook;
void JBE_File_Destructor_patched(void* thisPtr) {
    // Check if this is actually an AT9Stream disguised as a File/OggStream
    AT9Stream* stream = (AT9Stream*)thisPtr;

    // Check our AT9Stream magic marker (we'll add this to the structure)
    if (stream->magic == 0x41543950) {  // "AT9P" in hex
        logv_info("[ATRAC9] Destroying AT9Stream at %p", stream);
        AT9Stream_Destroy(stream);
        return;  // Don't call original destructor
    }

    // Not our stream, call original File destructor
    SO_CONTINUE(void, JBE_File_Destructor_hook, thisPtr);
}
```

**In `patch_function()` add**:
```c
// Hook File destructor to catch AT9Stream cleanup
JBE_File_Destructor_hook = hook_addr(so_symbol(&so_mod, "_ZN3JBE4FileD2Ev"),
                                     (uintptr_t)&JBE_File_Destructor_patched);
```

### 5.4 State Management in SND_Frame Hook

Since pause/resume is managed by flags, our `SND_Frame` hook needs to respect those flags:

```c
int AT9Stream_Decode(AT9Stream* stream, SoundStreamSlot* slot) {
    // Check flags from the game's slot structure
    if (!stream || !stream->isPlaying) {
        return 0;
    }

    // Check pause flag (bit 3 in slot->flags)
    if (slot && (slot->flags & 0x8)) {
        return 0;  // Paused, don't decode
    }

    // Check fade flag (bit 1 in slot->flags)
    if (slot && (slot->flags & 0x2)) {
        // Handle fade (adjust volume)
    }

    // Normal decoding...
    // ... rest of decode implementation
}
```

### 5.5 Updated AT9Stream Structure

Add magic marker for identification:

```c
typedef struct AT9Stream {
    // === Magic marker (MUST BE FIRST for destructor hook) ===
    uint32_t magic;                  // 0x000 - 0x41543950 ("AT9P") to identify AT9Stream

    // === Header (mimic JBE::File layout) ===
    void* vtable;                    // 0x004 - Virtual function table

    // ... rest of structure
} AT9Stream;
```

---

## Phase 6: Convert Audio Files from OGG to AT9

### 6.1 Install Sony AT9 Encoder

Download from Sony DevNet (requires PS Vita dev license):
- `at9tool.exe` (Windows)
- `at9enc` (Linux/Mac)

### 6.2 Create Conversion Script

**File**: `extras/scripts/convert_audio_to_at9.sh`

```bash
#!/bin/bash
# Convert all OGG files to AT9 format

GAME_ASSETS="ux0:data/soulcalibur/assets"
BITRATE=96  # kbps for music (use 64 for dialog, 128 for high quality)

find "$GAME_ASSETS" -name "*.ogg" | while read ogg_file; do
    at9_file="${ogg_file%.ogg}.at9"

    echo "Converting: $ogg_file -> $at9_file"

    # Convert OGG to WAV first (using ffmpeg or oggdec)
    wav_file="/tmp/temp_audio.wav"
    ffmpeg -i "$ogg_file" -ar 48000 -ac 2 "$wav_file"

    # Encode to AT9
    at9enc -br $BITRATE "$wav_file" "$at9_file"

    rm "$wav_file"
done

echo "Conversion complete!"
```

### 6.3 Update Build System

Ensure `.at9` files are copied to the VPK package.

**File**: `CMakeLists.txt`

```cmake
# Add .at9 files to package
file(GLOB_RECURSE AT9_FILES "${CMAKE_SOURCE_DIR}/assets/**/*.at9")
install(FILES ${AT9_FILES} DESTINATION assets)
```

---

## Phase 7: Test and Optimize

### 7.1 Testing Checklist

- [ ] Background music plays correctly (48kHz stereo)
- [ ] Dialog plays correctly (24kHz mono)
- [ ] Sound effects work (24kHz stereo)
- [ ] Multiple streams play simultaneously
- [ ] Looping works correctly
- [ ] Volume/fade controls work
- [ ] Pause/resume works
- [ ] Game performance improved (check CPU usage)
- [ ] No audio stuttering or glitches
- [ ] No memory leaks (check with profiler)

### 7.2 Optimization Opportunities

1. **Buffer Sizes**: Tune `mainBufferSize` and `outputBufferSize` based on profiling
2. **Thread Affinity**: Pin audio decoding to specific CPU core
3. **FIOS Priorities**: Set higher priority for music streams
4. **Shared Decoder Group**: Reuse across streams when possible
5. **Preloading**: Preload AT9 data for frequently used sounds

### 7.3 Debugging

Add logging:
```c
#define ATRAC9_DEBUG 1

#if ATRAC9_DEBUG
    #define atrac9_log(fmt, ...) logv_info("[ATRAC9] " fmt, ##__VA_ARGS__)
#else
    #define atrac9_log(fmt, ...)
#endif
```

---

## File Structure Summary

```
source/patches/
├── atrac9_manager.h         - ATRAC9 manager interface
├── atrac9_manager.c         - ATRAC9 manager implementation
├── atrac9_stream.h          - AT9Stream type definition
├── atrac9_stream.c          - AT9Stream implementation
├── ATRAC9_Migration_Plan.md - This document
└── sound_mgmt.h             - Existing sound structures

source/
├── patch.c                  - Add hooks for SND_Init, OggStream constructor, etc.
└── utils/vorbis_patch.c     - Modify SND_Frame hook

extras/scripts/
└── convert_audio_to_at9.sh  - Audio conversion script
```

---

## Function Hooking Summary

| Original Function | Hook Target | Implementation | Notes |
|------------------|-------------|----------------|-------|
| `SND_Init` | `SND_Init_patched` | Initialize ATRAC9 manager | ✅ Required |
| `OggStream::OggStream` (constructor) | `OggStream_Constructor_patched` | Create AT9Stream instead | ✅ Required |
| `JBE::File::~File` (destructor) | `JBE_File_Destructor_patched` | Destroy AT9Stream when OggStream deleted | ✅ Required |
| `SND_Frame` | `snd_frame_atrac9` | Process AT9 streams each frame | ✅ Required |
| `XPhysicalAlloc` | `XPhysicalAlloc_patched` | Simple malloc wrapper | ⚠️ Optional (for compatibility) |
| ~~`OggStream::Pause`~~ | N/A | **Does not exist** - uses flags instead | ❌ Not needed |
| ~~`OggStream::Resume`~~ | N/A | **Does not exist** - uses flags instead | ❌ Not needed |
| ~~`OggStream::Stop`~~ | N/A | **Does not exist** - uses flags instead | ❌ Not needed |

---

## Expected Performance Impact

| Metric | Before (Vorbis) | After (ATRAC9) | Improvement |
|--------|----------------|----------------|-------------|
| Decode Time/Frame | ~20ms | ~0.5ms | 40x faster |
| CPU Usage (3 streams) | ~30-50% | <3% | 10-15x less |
| Memory per Stream | ~30KB | ~32KB | Similar |
| Battery Life | Baseline | +20-30% | Significant |

---

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| AT9 files not found | Add fallback to Vorbis if .at9 missing |
| Audio quality issues | Use higher bitrate (128kbps) for critical audio |
| Sync issues | Match audio grain to game's frame timing |
| Memory leaks | Implement proper cleanup in AT9Stream_Destroy |
| FIOS conflicts | Use separate FIOS DH for audio vs. other I/O |

---

## Next Steps

1. ✅ Review this plan
2. ⬜ Get Ghidra function addresses for OggStream methods
3. ⬜ Implement Phase 1 (infrastructure)
4. ⬜ Implement Phase 2 (SND_Init hook)
5. ⬜ Test basic initialization
6. ⬜ Continue with remaining phases

---

**Document Status**: Complete Implementation Plan
**Estimated Effort**: 8-16 hours implementation + 2-4 hours testing
**Priority**: High (major performance improvement)
