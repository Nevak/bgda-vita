# PS Vita ATRAC9 Streaming Library Analysis

## Document Purpose
Analysis of the PS Vita ATRAC9 streaming system to understand requirements for replacing OGG Vorbis streaming with ATRAC9 streaming. This document covers the ATRAC9 codec, streaming architecture, and related PS Vita audio APIs.

---

## Table of Contents
1. [ATRAC9 Overview](#atrac9-overview)
2. [Library Architecture](#library-architecture)
3. [Core Components](#core-components)
4. [Streaming Implementation](#streaming-implementation)
5. [API Reference](#api-reference)
6. [Memory Requirements](#memory-requirements)
7. [Comparison with Vorbis](#comparison-with-vorbis)

---

## ATRAC9 Overview

### What is ATRAC9?
ATRAC9™ (Adaptive TRansform Acoustic Coding 9) is Sony's audio compression codec designed specifically for PlayStation®Vita. It's optimized for:
- Low memory footprint
- Hardware-accelerated decoding
- Efficient streaming from storage
- Loop playback support
- Sample-accurate seeking

### Format Characteristics
- **Codec**: ATRAC9™ lossy compression
- **Container**: RIFF/WAV with ATRAC9 data chunks
- **Sample Rates**: 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000 Hz
- **Channels**: Mono or Stereo
- **Output Format**: 16-bit signed PCM (S16)
- **File Extension**: `.at9`

### Key Features
- **Streaming Playback**: Efficient streaming with configurable buffer sizes
- **Loop Playback**: Sample-accurate loop points with epilogue support
- **Seeking**: Dynamic playback position changes in sample units
- **Low Latency**: Optimized for real-time playback
- **Hardware Decode**: Uses PS Vita's hardware decoder (libaudiodec)

---

## Library Architecture

### Component Stack

```
┌─────────────────────────────────────────┐
│         Application Layer               │
│  (Game code, audio management)          │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│         libatrac (High-level)           │
│  - Stream management                    │
│  - Loop control                         │
│  - Seeking                              │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│    NGS AT9 Streamer (Middleware)        │
│  - Buffer management                    │
│  - Callbacks                            │
│  - Multiple voice support               │
└─────────────────────────────────────────┘
                    ↓
┌────────────────┬────────────────────────┐
│  libaudiodec   │    NGS System          │
│  (HW Decoder)  │  (Audio Synthesis)     │
└────────────────┴────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│      Audio Output (sceAudioOut)         │
│  - Hardware audio output                │
│  - Sample rate conversion               │
│  - Mixing                               │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│         FIOS2 (File I/O)                │
│  - Asynchronous file reading            │
│  - PS Archive (.psarc) support          │
└─────────────────────────────────────────┘
```

### Three-Tier Architecture

#### 1. libatrac (High-Level Streaming Library)
- **Purpose**: Simplifies ATRAC9 streaming and loop management
- **Footprint**: 36 KiB PRX module
- **Work Memory**: 1-16 KiB (application-provided)
- **Threading**: No internal threads (caller's thread executes)

#### 2. NGS (Next Generation Synthesizer)
- **Purpose**: Audio synthesis and routing system
- **Features**: Module-based audio graph, mixing, effects
- **Granularity**: Processes audio in configurable sample blocks (e.g., 512 samples)
- **Sample Rate**: Fixed at 48000 Hz

#### 3. Audio Output System
- **Ports**: MAIN (8 channels), BGM (1 channel), VOICE (1 channel)
- **Output**: 16-bit PCM, 48000 Hz
- **Features**: Hardware mixing, sample rate conversion, ALC (Auto Level Control)

---

## Core Components

### 1. libatrac Library

#### Decoder Group
The decoder group is the container for ATRAC9 decoders:

```c
typedef struct SceAtracDecoderGroup {
    uint32_t size;         // Structure size
    uint32_t wordLength;   // Word length (16-bit = 2)
    uint32_t totalCh;      // Total channels across all decoders
} SceAtracDecoderGroup;
```

#### Stream Information
Used for managing streaming data:

```c
typedef struct SceAtracStreamInfo {
    uint32_t pWritePosition;  // Pointer to write position in buffer
    uint32_t writableSamples; // Number of samples that can be written
    uint32_t readOffset;      // File offset to read from
    uint32_t readSize;        // Number of bytes to read
} SceAtracStreamInfo;
```

#### Content Information
Describes the ATRAC9 audio content:

```c
typedef struct SceAtracContentInfo {
    uint32_t channel;        // Number of channels
    uint32_t samplingRate;   // Sampling rate
    uint32_t loopStart;      // Loop start sample
    uint32_t loopEnd;        // Loop end sample
    uint32_t totalSamples;   // Total samples in file
} SceAtracContentInfo;
```

### 2. NGS AT9 Streamer

#### Configuration
Default streaming configuration:

```c
// From ngs_streamer_config.h
#define NUM_STREAM_BUFFERS (2)                    // Double buffering
#define DEFAULT_AT9_STREAM_BUFFER_SIZE (8 * 1024) // 8 KB per buffer
```

#### Key Structures
- **Voice**: Audio playback channel in NGS
- **Rack**: Container for voices in audio graph
- **Patch**: Connection between racks/voices
- **Module**: Processing unit (AT9 Player, Master Output, etc.)

### 3. FIOS2 (File I/O Scheduler)

#### Purpose
- Efficient asynchronous file I/O
- Priority-based scheduling
- PS Archive (.psarc) support
- Optimal for streaming multiple audio files

#### Read Modes
- **Synchronous**: `fiosHandlerReadSync()` - blocking read
- **Asynchronous**: `fiosHandlerReadAsync()` - non-blocking with callback

---

## Streaming Implementation

### Basic Streaming Flow

```
┌──────────────────────────────────────────────────────────────┐
│                    Initialization Phase                       │
└──────────────────────────────────────────────────────────────┘
    1. Create Decoder Group (work memory allocation)
    2. Open AT9 file with FIOS
    3. Allocate main buffer (256-byte aligned)
    4. Set data and acquire handle
    5. Create NGS voice for playback

┌──────────────────────────────────────────────────────────────┐
│                    Streaming Loop                             │
└──────────────────────────────────────────────────────────────┘
    Loop:
        1. Decode samples → sceAtracDecode()
        2. Check decoder status
        3. If data needed:
           a. Get stream info → sceAtracGetStreamInfo()
           b. Read AT9 data from file (FIOS)
           c. Add stream data → sceAtracAddStreamData()
        4. Output PCM to NGS voice
        5. NGS processes audio (synthesis, mixing)
        6. Audio output to hardware

┌──────────────────────────────────────────────────────────────┐
│                    Cleanup Phase                              │
└──────────────────────────────────────────────────────────────┘
    1. Stop playback
    2. Release handle → sceAtracReleaseHandle()
    3. Delete decoder group
    4. Close file, free buffers
```

### Memory Layout

```
┌─────────────────────────────────────────────────────────┐
│  Work Memory (1-16 KiB, 256-byte aligned)               │
│  - Allocated by application                             │
│  - Used by decoder group                                │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│  Main Buffer (streaming buffer, 256-byte aligned)       │
│  ┌───────────────┬───────────────┐                      │
│  │   Buffer 0    │   Buffer 1    │  (Double buffered)   │
│  │   8 KB        │   8 KB        │                      │
│  └───────────────┴───────────────┘                      │
│  - Holds compressed AT9 data                            │
│  - Size: typically 8 KB × 2 buffers                     │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│  Output Buffer (PCM output, 256-byte aligned)           │
│  - Holds decoded 16-bit PCM samples                     │
│  - Size: granularity × channels × 2 bytes              │
│  - Example: 512 samples × 2 ch × 2 = 2048 bytes        │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│  Sub Buffer (optional, for loop epilogue)               │
│  - Used when loop has epilogue section                  │
│  - Holds data for smooth loop transitions               │
│  - 256-byte aligned                                     │
└─────────────────────────────────────────────────────────┘
```

### Buffer Size Calculation

#### Minimum Stream Buffer Size
To avoid audio starvation:

```c
min_buffer_size = ((granularity * numChannels * MAX_PITCH_RATIO / AT9_SAMPLES_PER_PACKET)
                   * packetEncodedSize) + packetEncodedSize;
```

Where:
- `granularity`: NGS system granularity (e.g., 512 samples)
- `numChannels`: 1 (mono) or 2 (stereo)
- `MAX_PITCH_RATIO`: Maximum pitch adjustment (default: 4, or 1 for fixed playback)
- `AT9_SAMPLES_PER_PACKET`: Samples per AT9 packet (from file header)
- `packetEncodedSize`: Compressed packet size (nBlockAlign from 'fmt' chunk)

#### Buffer Alignment Requirements
- Buffers must align to packet or superpacket boundaries
- When using `nSamplesDiscardStart`: align to superpacket start
- When using `nSamplesDiscardEnd`: align to both start and end of superpacket

---

## API Reference

### libatrac Core APIs

#### Initialization

```c
// 1. Query required work memory size
int32_t sceAtracQueryDecoderGroupMemSize(
    SceAtracDecoderGroup *pDecoderGroup,
    uint32_t *pWorkMemSize
);

// 2. Create decoder group
int32_t sceAtracCreateDecoderGroup(
    uint32_t decoderType,
    SceAtracDecoderGroup *pDecoderGroup,
    void *pWorkMem,
    uint32_t workMemSize
);

// 3. Set AT9 data and get handle
int32_t sceAtracSetDataAndAcquireHandle(
    SceAtracDecoderGroup *pDecoderGroup,
    uint8_t *pMainBuffer,
    uint32_t mainBufferSize,
    uint32_t *pAtracHandle
);
```

#### Decoding

```c
// Decode one frame of audio
int32_t sceAtracDecode(
    uint32_t atracHandle,
    void *pOutputBuffer,
    uint32_t *pDecoderStatus,
    uint32_t *pSamplesDecoded
);

// Decoder status values:
// - SCE_ATRAC_DECODER_STATUS_NORMAL: Normal operation
// - SCE_ATRAC_DECODER_STATUS_NEED_DATA: Need more stream data
// - SCE_ATRAC_DECODER_STATUS_END: End of stream
```

#### Streaming

```c
// Get streaming information
int32_t sceAtracGetStreamInfo(
    uint32_t atracHandle,
    SceAtracStreamInfo *pStreamInfo
);

// Add streamed data to buffer
int32_t sceAtracAddStreamData(
    uint32_t atracHandle,
    uint32_t bytesAdded
);
```

#### Loop Control

```c
// Set loop count (-1 for infinite)
int32_t sceAtracSetLoopNum(
    uint32_t atracHandle,
    int32_t loopNum
);

// Get loop information
int32_t sceAtracGetLoopInfo(
    uint32_t atracHandle,
    uint32_t *pLoopStart,
    uint32_t *pLoopEnd,
    uint32_t *pLoopStatus
);
```

#### Seeking

```c
// Reset playback position
int32_t sceAtracResetNextOutputPosition(
    uint32_t atracHandle,
    uint32_t samplePosition
);
```

#### Cleanup

```c
// Release handle
int32_t sceAtracReleaseHandle(
    uint32_t atracHandle
);

// Delete decoder group
int32_t sceAtracDeleteDecoderGroup(
    uint32_t decoderType,
    SceAtracDecoderGroup *pDecoderGroup
);
```

### NGS AT9 Streamer APIs

#### High-Level Wrapper Functions

```c
// Initialize AT9 streamer
int32_t sceNgsAt9StreamerInit(
    const char *filename,
    SceNgsHVoice voice,
    void *pStreamBuffers,
    uint32_t streamBufferSize,
    SceNgsAt9StreamerHandle *pHandle
);

// Set loop configuration
int32_t sceNgsAt9StreamerSetLoop(
    SceNgsAt9StreamerHandle handle,
    uint32_t loopStart,
    uint32_t loopEnd,
    uint32_t loopCount,
    uint32_t useHeaderInfo
);

// Seek to position
int32_t sceNgsAt9StreamerSeek(
    SceNgsAt9StreamerHandle handle,
    uint32_t sampleOffset,
    uint32_t numSamples
);

// Start playback
int32_t sceNgsAt9StreamerPlay(
    SceNgsAt9StreamerHandle handle
);

// Stop playback
int32_t sceNgsAt9StreamerStop(
    SceNgsAt9StreamerHandle handle
);

// Handle player callbacks (call from audio data thread)
int32_t sceNgsAt9StreamerHandlePlayerCallback(
    SceNgsAt9StreamerHandle handle
);

// Release streamer
int32_t sceNgsAt9StreamerRelease(
    SceNgsAt9StreamerHandle handle
);
```

### Audio Output APIs

```c
// Open audio port
int32_t sceAudioOutOpenPort(
    SceAudioOutPortType portType,  // MAIN, BGM, or VOICE
    int32_t len,                   // Granularity (samples per output)
    int32_t freq,                  // Sample rate
    SceAudioOutMode mode           // STEREO or MONO
);

// Output audio data
int32_t sceAudioOutOutput(
    int32_t portId,
    const void *pBuffer  // PCM data (NULL to wait for completion)
);

// Set volume
int32_t sceAudioOutSetVolume(
    int32_t portId,
    SceAudioOutChannelFlag channelFlag,
    int32_t *pVolumes  // Volume per channel (0-32768)
);

// Release port
int32_t sceAudioOutReleasePort(
    int32_t portId
);
```

---

## Memory Requirements

### Per-Stream Memory Breakdown

#### Decoder Group
- **Work Memory**: 1-16 KiB (depends on configuration)
- **Alignment**: 256 bytes

#### Main Buffer (Streaming)
- **Size**: 8 KB × 2 buffers = 16 KB (typical configuration)
- **Alignment**: 256 bytes
- **Purpose**: Hold compressed AT9 data for streaming

#### Output Buffer (PCM)
- **Size**: `granularity × channels × 2 bytes`
- **Example**: 512 samples × 2 channels × 2 = 2048 bytes (~2 KB)
- **Alignment**: 256 bytes
- **Buffering**: Double or triple buffering recommended

#### Sub Buffer (Optional)
- **Size**: Variable (depends on loop configuration)
- **Usage**: Only needed for loops with epilogue
- **Alignment**: 256 bytes

### Total Memory Example (Stereo Stream)
```
Work Memory:        ~8 KB
Main Buffer:        16 KB (2 × 8 KB)
Output Buffer:      4 KB (double buffered)
Sub Buffer:         ~4 KB (if needed)
-----------------------------------------
Total:              ~32 KB per stream
```

### Optimization Notes
- **Buffer Size vs. Performance**: Smaller buffers reduce latency but increase FIOS overhead
- **FIOS Performance**: Larger reads (8 KB+) perform better, especially with PS Archive
- **Multiple Streams**: Share decoder group when possible
- **Alignment**: All buffers must be 256-byte aligned

---

## Comparison with Vorbis

### Vorbis (Original Xbox Implementation)

#### API Pattern
```c
// Open file
File* oggFile = fopen("music.ogg", "rb");

// Initialize decoder
OggVorbis_File vf;
ov_open(oggFile, &vf, NULL, 0);

// Decode loop
while (playing) {
    long bytes = ov_read(&vf, buffer, bufferSize, &bitstream);
    if (bytes <= 0) break;

    // Output to DirectSound
    IDirectSoundBuffer_Lock(...);
    memcpy(audioBuffer, buffer, bytes);
    IDirectSoundBuffer_Unlock(...);
}

// Cleanup
ov_clear(&vf);
fclose(oggFile);
```

#### Seeking
```c
// Seek to sample position
ov_raw_seek(&vf, samplePosition);
```

#### Looping
```c
// Manual loop implementation
if (currentSample >= loopEnd) {
    ov_raw_seek(&vf, loopStart);
}
```

### ATRAC9 (PS Vita Implementation)

#### API Pattern
```c
// Open with FIOS
FiosHandle file = fiosHandlerOpen("music.at9");

// Create decoder group
SceAtracDecoderGroup group;
sceAtracQueryDecoderGroupMemSize(&group, &workSize);
void* workMem = malloc(workSize);
sceAtracCreateDecoderGroup(SCE_ATRAC_TYPE_AT9, &group, workMem, workSize);

// Set data and get handle
sceAtracSetDataAndAcquireHandle(&group, mainBuffer, bufferSize, &handle);

// Decode loop
while (playing) {
    uint32_t status, samplesDecoded;
    sceAtracDecode(handle, outputBuffer, &status, &samplesDecoded);

    if (status == SCE_ATRAC_DECODER_STATUS_NEED_DATA) {
        SceAtracStreamInfo info;
        sceAtracGetStreamInfo(handle, &info);
        fiosHandlerReadSync(file, info.readOffset, info.readSize, mainBuffer);
        sceAtracAddStreamData(handle, info.readSize);
    }

    // Output to NGS/AudioOut
    sceAudioOutOutput(portId, outputBuffer);
}

// Cleanup
sceAtracReleaseHandle(handle);
sceAtracDeleteDecoderGroup(SCE_ATRAC_TYPE_AT9, &group);
fiosHandlerClose(file);
```

#### Seeking
```c
// Seek to sample position
sceAtracResetNextOutputPosition(handle, samplePosition);
// Then refill buffers with new data from file
```

#### Looping
```c
// Built-in loop support (can read from file header)
sceAtracSetLoopNum(handle, -1);  // Infinite loop
// Library handles loop automatically
```

### Key Differences

| Feature | Vorbis (Xbox) | ATRAC9 (PS Vita) |
|---------|---------------|------------------|
| **Decoder** | Software (CPU) | Hardware-accelerated |
| **API Complexity** | Simple, linear | More complex, state-driven |
| **Memory** | ~20-30 KB/stream | ~32 KB/stream |
| **Seeking** | Direct file seek | Reset position + refill buffer |
| **Looping** | Manual implementation | Built-in support |
| **Buffer Management** | Application-controlled | Library-assisted |
| **File I/O** | Standard fread() | FIOS2 (async, scheduled) |
| **Audio Output** | DirectSound | sceAudioOut/NGS |
| **Threading** | Application threads | Caller's thread + optional audio thread |

### Migration Complexity

#### What Stays the Same
- Overall streaming architecture (read, decode, output)
- Double/triple buffering concept
- Thread synchronization needs
- Audio mixing requirements

#### What Changes
1. **File I/O**: Replace fopen/fread with FIOS2 APIs
2. **Decoder**: Replace ov_* calls with sceAtrac* calls
3. **Buffer Management**: More explicit buffer state management
4. **Audio Output**: Replace DirectSound with sceAudioOut
5. **Loop Control**: Use built-in loop APIs instead of manual implementation
6. **Seeking**: Two-step process (reset position + refill data)
7. **Memory Alignment**: All buffers must be 256-byte aligned

#### Estimated Effort
- **Low**: If using NGS AT9 Streamer wrapper (simplified API)
- **Medium**: If using libatrac directly (more control, more complexity)
- **High**: If also converting audio files from OGG to AT9

---

## Sample Implementation Workflow

### Recommended Approach for Conversion

#### Phase 1: Setup (One-time)
1. Convert OGG files to AT9 format using Sony's encoder
2. Update build system to package .at9 files
3. Optionally create .psarc archive for multiple files

#### Phase 2: Code Changes (Per Stream Type)

**Music Streams (Background Music)**
```c
// Old: Vorbis
OggVorbis_File vf;
ov_open(file, &vf, NULL, 0);
while (playing) {
    ov_read(&vf, buffer, size, &stream);
    output_to_directsound(buffer);
}
ov_clear(&vf);

// New: ATRAC9 (simplified with NGS wrapper)
SceNgsAt9StreamerHandle streamer;
sceNgsAt9StreamerInit("music.at9", voice, buffers, bufferSize, &streamer);
sceNgsAt9StreamerSetLoop(streamer, loopStart, loopEnd, -1, true);
sceNgsAt9StreamerPlay(streamer);
// Library handles streaming in callbacks
// When done:
sceNgsAt9StreamerStop(streamer);
sceNgsAt9StreamerRelease(streamer);
```

**Dialog Streams (24kHz Mono)**
- Similar pattern, but use different NGS voice
- Set sample rate to 24000 Hz
- Use mono channel configuration

**Ambience Streams (24kHz Stereo)**
- Similar to music, different sample rate
- Can share decoder group with music

#### Phase 3: Integration
1. Initialize NGS system at startup
2. Create audio output ports (MAIN for SFX, BGM for music)
3. Create audio update thread (for NGS updates)
4. Create audio data thread (for stream buffer refills)
5. Integrate with existing sound manager

---

## Performance Considerations

### CPU Usage
- **Hardware Decode**: Minimal CPU overhead for decoding
- **Streaming**: Main cost is file I/O and buffer management
- **NGS Updates**: Fixed cost per frame (depends on granularity)

### Latency
- **Granularity**: Smaller = lower latency, higher CPU overhead
- **Typical**: 512 samples @ 48kHz = ~10.6 ms latency
- **Recommendation**: Match granularity across all streams for efficiency

### File I/O
- **FIOS Priority**: Use high priority for music, normal for ambience
- **Buffer Size**: 8 KB is good balance for streaming
- **PS Archive**: Acceptable for pre-compressed AT9 (don't double-compress)

### Memory Bandwidth
- **Streaming**: ~48-96 KB/sec per stereo stream @ 128 kbps
- **PCM Output**: 192 KB/sec (48kHz stereo 16-bit)

---

## Limitations and Notes

### Format Limitations
- **No ATRAC9 Band Extension**: libatrac doesn't support extended format
  - Check `dwVersionInfo` in RIFF header to verify
- **Minimum Loop Size**: Loop section must be >3072 samples
- **Multiple Loops**: Only first loop is supported
- **Alignment**: All buffers require 256-byte alignment

### Threading Considerations
- **No Internal Threads**: libatrac uses caller's thread
- **Blocking Calls**: `sceAtracDecode()` and `sceAudioOutOutput()` block
- **Synchronization**: Application must handle thread sync for buffer access
- **Recommended**: Separate audio update thread and file I/O thread

### Buffer Management
- **Double Buffering**: Minimum for continuous playback
- **Triple Buffering**: Recommended for safety margin
- **Don't Rewrite**: Never modify buffer while it's being played
- **Sub Buffer**: Required for loops with epilogue, handle carefully

---

## Next Steps for Integration

### Prerequisites
1. ✅ Understand ATRAC9 streaming architecture (this document)
2. ⬜ Convert audio assets from OGG to AT9
3. ⬜ Set up PS Vita build environment
4. ⬜ Create proof-of-concept with single stream

### Implementation Plan
1. **Initialize Audio System**
   - Set up NGS system
   - Create audio output ports
   - Initialize FIOS2

2. **Create Stream Manager**
   - Port existing sound manager structures
   - Replace Vorbis types with ATRAC9 equivalents
   - Implement state machine for stream lifecycle

3. **Implement Streaming Loop**
   - Decoder group management
   - Buffer management (main, output, sub)
   - FIOS integration for file reads
   - NGS voice control

4. **Add Loop Support**
   - Parse AT9 loop points
   - Implement seamless looping
   - Handle epilogue sections

5. **Integrate with Game**
   - Replace existing music system
   - Update dialog system
   - Test all audio scenarios

---

## References

### Sony Documentation
- **ATRAC9 Simple Streaming Tutorial** (000010341632)
- **libatrac Overview** (000010341632)
- **Audio Output Function Overview** (000010341632)
- NGS Overview and Reference
- libfios2 Overview and Reference
- libaudiodec Overview

### Code Samples
- `sample_code/audio_video/tutorial_at9_simple_streaming/`
- `sample_code/audio_video/api_libatrac/basic/`
- `sample_code/audio_video/api_audioout/stream/`

---

## Glossary

- **AT9**: ATRAC9 file extension
- **ALC**: Automatic Level Control (dynamic normalizer)
- **FIOS**: File I/O Scheduler (PS Vita async I/O library)
- **Granularity**: Number of samples processed per audio update
- **NGS**: Next Generation Synthesizer (PS Vita audio synthesis library)
- **PCM**: Pulse Code Modulation (uncompressed audio)
- **Port**: Audio output channel (MAIN, BGM, VOICE)
- **Rack**: NGS container for voices
- **SRC**: Sample Rate Conversion
- **Voice**: NGS playback channel

---

**Document Status**: Complete - ATRAC9 Analysis
**Next Phase**: Audio Asset Conversion and Integration Planning
