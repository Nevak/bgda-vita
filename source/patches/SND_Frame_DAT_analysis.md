# SND_Frame DAT_XXXX Variable Analysis

## Overview
Analysis of DAT_XXXX variables in `SND_Frame` function at `0x0010c5a4` to determine if they're part of a larger structure.

## Address Ranges

### Group 1: Float Constants (0x0010c9ac - 0x0010ca54)
- `DAT_0010c9ac` - Dialog volume fade target (0.5 or similar)
- `DAT_0010c9b0` - Dialog volume fade rate
- `DAT_0010ca54` - Music volume fade rate

### Group 2: Float Constants (0x0010d334 - 0x0010d344)
- `DAT_0010d334` - Random factor for sound variation
- `DAT_0010d338` - Frame time/delta multiplier
- `DAT_0010d33c` - Minimum volume threshold
- `DAT_0010d340` - Volume scaling factor (denominator)
- `DAT_0010d344` - Volume scaling factor (numerator)

### Group 3: Global Sound Manager Structure (0x0010d394 - 0x0010d6f0)
**This appears to be a large structure - possibly a global sound manager singleton**

#### Offset Analysis (Sequential 4-byte pointers):

**0x0010d394 - 0x0010d3ac Range:**
- `DAT_0010d394 + 0x10c5d0` → Stack check value
- Usage: `**(int **)(DAT_0010d394 + 0x10c5d0)` - double pointer dereference

**DirectSound Buffer Management (0x0010d5d8 - 0x0010d648):**
- `DAT_0010d5d8 + 0x10c5e0` → DirectSoundBuffer pointer
  - Used with: `IDirectSoundBuffer_GetCurrentPosition`
- `DAT_0010d5e0 + 0x10c5f8` → Sound write cursor tracking
- `DAT_0010d5dc + 0x10c5f4` → Current playback position
- `DAT_0010d5f8 + 0x10c620` → Volume fade pointer/value
  - Used as: `pfVar11 = (float *)(DAT_0010d5f8 + 0x10c620)`

**Float Volume Constants (0x0010d5e4 - 0x0010d5f4):**
- `DAT_0010d5e4` - Minimum volume threshold
- `DAT_0010d5e8` - Volume divisor
- `DAT_0010d5ec` - Volume multiplier
- `DAT_0010d5f0` - Frame time multiplier
- `DAT_0010d5f4` - Global volume scale

**Stream Slot Management (0x0010d5fc - 0x0010d648):**
- `DAT_0010d5fc + 0x10c6a0` → Stream slot base address
  - Loop iterates 0x400 bytes (3 streams × 0x2dc each = ~0x888, but array is 0x400)
- `DAT_0010d600 + 0x10c6b8` → Pause/play state flag
- `DAT_0010d604 + 0x10c6ec` → Max sound instances?
- `DAT_0010d608 + 0x10c71c` → Busy flags (CD/texture loading)
- `DAT_0010d60c + 0x10c74c` → Stream ready flag
- `DAT_0010d610 + 0x10c768` → Current stream index
- `DAT_0010d614 + 0x10c778` → Stream data array base
  - Array of 0x2dc-sized structs
- `DAT_0010d618 + 0x10c78c` → Bytes read from stream
- `DAT_0010d61c + 0x10c7a4` → Total bytes read
- `DAT_0010d620 + 0x10c8ac` → Stream slot extended data
- `DAT_0010d624 + 0x10c7c4` → Buffer size tracking
- `DAT_0010d628 + 0x10c7cc` → Reset counter
- `DAT_0010d62c + 0x10c810` → Current stream index (duplicate?)
- `DAT_0010d630 + 0x10c82c` → Stream error state
- `DAT_0010d634 + 0x10c834` → Stream flags/state
- `DAT_0010d638 + 0x10c858` → OGG Vorbis seek data
- `DAT_0010d63c + 0x10c8e8` → Stream slot copy buffer (0x28 bytes)
  - Contains 8 bytes × 5 = 40 bytes (5 doubles or 10 ints)
- `DAT_0010d640 + 0x10c910` → Stream buffer size array
- `DAT_0010d644 + 0x10c938` → Main stream processing loop base
- `DAT_0010d648 + 0x10cb98` → Buffer size thresholds (per stream)

**OGG Vorbis Decoding (0x0010d64c - 0x0010d668):**
- `DAT_0010d64c + 0x10ccc8` → Current stream being decoded
- `DAT_0010d650 + 0x10ccd0` → Decode buffer size
- `DAT_0010d654 + 0x10ccd8` → Decode start flag/offset
- `DAT_0010d658 + 0x10cce8` → Decode completion flag
- `DAT_0010d65c + 0x10cd68` → Stream index for second track
- `DAT_0010d660 + 0x10cd84` → Decode offset/position
- `DAT_0010d664 + 0x10cd8c` → Bytes decoded
- `DAT_0010d668 + 0x10cd98` → Decode active flag

**Animation Sound Playback (0x0010d66c - 0x0010d684):**
- `DAT_0010d66c + 0x10cdc0` → Animation sound base
  - Loop: 0x980 bytes, stride 0x4c (50 slots × 0x4c = 0xF00, but only 0x980 used = ~30 slots)
- `DAT_0010d670 + 0x10cdc8` → Current blend weight
- `DAT_0010d674 + 0x10cdcc` → Current animation sound index
- `DAT_0010d678 + 0x10cdd0` → Stream slot pointer
- `DAT_0010d67c + 0x10cc3c` → Alternative base address for stream processing
- `DAT_0010d680 + 0x10ce30` → Pause state for animations
- `DAT_0010d684 + 0x10d4e0` → Sound device/manager interface
  - Used with: `(**(code **)(**(int **)(DAT_0010d684 + 0x10d4e0) + 0xc))`
  - Virtual function call (vtable + 0xc)

**Dialog System (0x0010d688 - 0x0010d6c8):**
- `DAT_0010d688 + 0x10d4d4` → Active dialog count/flag
- `DAT_0010d68c + 0x10d7ac` → Secondary dialog check
- `DAT_0010d690 + 0x10cf30` → Dialog queue count
- `DAT_0010d694 + 0x10cf4c` → Dialog queue base
  - Stride: 0x2a8 per dialog entry
- `DAT_0010d698 + 0x10cfc8` → Random seed pointer for dialog
- `DAT_0010d69c + 0x10d070` → Random seed pointer (duplicate)
- `DAT_0010d6a0 + 0x10d0dc` → Dialog volume scale base
- `DAT_0010d6a4 + 0x10d0e8` → Dialog volume multiplier
- `DAT_0010d6a8 + 0x10d160` → Dialog DirectSoundStream interface (channel 0)
- `DAT_0010d6ac + 0x10d14c` → Dialog volume for channel 1
- `DAT_0010d6b0 + 0x10d49c` → Dialog DirectSoundStream interface (channel 1)
- `DAT_0010d6b4 + 0x10d1a4` → Dialog volume for channel 2
- `DAT_0010d6b8 + 0x10d7d0` → Dialog DirectSoundStream interface (channel 2)
- `DAT_0010d6bc + 0x10d2a0` → Dialog filename buffer
- `DAT_0010d6c0 + 0x10d220` → Dialog data base
- `DAT_0010d6c4 + 0x10cf88` → Dialog array count
- `DAT_0010d6c8 + 0x10d2b0` → Frame counter

**Music Streaming (0x0010d6cc - 0x0010d6f0):**
- `DAT_0010d6cc + 0x10d2f4` → Music target volume level
- `DAT_0010d6cc + 0x10d2f8` → Music current volume level
- `DAT_0010d6cc + 0x10d304` → Music stream interface (track 0)
- `DAT_0010d6cc + 0x10d5d0` → Music target volume (track 1)
- `DAT_0010d6cc + 0x10d5d4` → Music current volume (track 1)
- `DAT_0010d6cc + 0x10d8ac` → Music target volume (track 2)
- `DAT_0010d6cc + 0x10d8b0` → Music current volume (track 2)
- `DAT_0010d6cc + 0x10d8bc` → Music stream interface (track 2)
- `DAT_0010d6d0 + 0x10d2d0` → Frame update counter
- `DAT_0010d6d4 + 0x10d31c` → Computed music volume (track 0)
- `DAT_0010d6d8 + 0x10d37c` → Computed music volume (track 1)
- `DAT_0010d6dc + 0x10d3bc` → Music volume scale
- `DAT_0010d6dc + 0x10d3c0` → Volume multiplier (track 1)
- `DAT_0010d6dc + 0x10d3c4` → Volume multiplier (track 2)
- `DAT_0010d6e0 + 0x10d400` → Computed music volume (track 2)
- `DAT_0010d6e4 + 0x10d434` → Volume blend factor
- `DAT_0010d6e8 + 0x10d4cc` → Master volume scale
- `DAT_0010d6ec + 0x10d4dc` → Master volume multiplier
- `DAT_0010d6f0 + 0x10d5bc` → Stack check comparison value

## Structure Hypothesis

These variables appear to be **offset calculations from a relocatable base address**. The pattern is:
```c
globalData = DAT_0010dXXX + FIXED_OFFSET
```

This suggests these are **NOT individual variables**, but rather:
1. A **relocation table** or **global offset table (GOT)**
2. Each DAT_XXXX holds a base pointer that's adjusted at runtime
3. The constant offsets are compile-time known structure member offsets

### Proposed Structure

```c
typedef struct SoundManagerGlobals {
    // 0x10c5d0: Stack check
    int** stackCheckPtr;

    // 0x10c5e0 - 0x10c620: DirectSound buffer management
    IDirectSoundBuffer* primaryBuffer;      // +0x10c5e0
    uint writePosition;                      // +0x10c5f4
    uint lastPosition;                       // +0x10c5f8
    float musicVolumeFade;                   // +0x10c620

    // 0x10c6a0 - 0x10c900: Stream management (3 streams × 0x2dc each)
    StreamSlot streamSlots[32];              // +0x10c6a0, size 0x400
    char pauseFlag;                          // +0x10c6b8
    int maxSoundInstances;                   // +0x10c6ec
    byte busyFlags;                          // +0x10c71c

    // Stream processing state
    char streamReadyFlag;                    // +0x10c74c
    uint currentStreamIndex;                 // +0x10c768
    StreamData streamData[3];                // +0x10c778, 0x2dc each

    // 0x10c8e8 - 0x10c910: Stream slot snapshot (40 bytes)
    uint64_t streamSnapshot[5];              // +0x10c8e8
    uint streamBufferSizes[3];               // +0x10c910 (one per stream)

    // 0x10c938 onwards: Main stream structures
    MusicStream musicStreams[3];             // +0x10c938, stride 0x2dc

    // OGG decode state
    int currentDecodeStream;                 // +0x10ccc8
    int decodeBufferSize;                    // +0x10ccd0
    int decodeOffset;                        // +0x10ccd8
    char decodeActive;                       // +0x10cce8

    // Animation sounds
    AnimSound animSounds[30];                // +0x10cdc0, stride 0x4c, total 0x980
    float currentBlendWeight;                // +0x10cdc8
    int currentAnimSoundIndex;               // +0x10cdcc
    void* animStreamSlotPtr;                 // +0x10cdd0

    // Dialog system
    int activeDialogCount;                   // +0x10d4d4
    int dialogQueueCount;                    // +0x10cf30
    DialogEntry dialogQueue[MAX_DIALOGS];    // +0x10cf4c, stride 0x2a8
    uint* randomSeed;                        // +0x10cfc8
    float dialogVolumeScale;                 // +0x10d0dc
    float dialogVolumeMult;                  // +0x10d0e8
    IDirectSoundStream* dialogStreams[3];    // +0x10d160, +0x10d49c, +0x10d7d0
    float dialogChannelVolumes[3];           // +0x10d14c, +0x10d1a4, ...
    char dialogFilenameBuffer[256];          // +0x10d2a0

    // Music volume control (3 tracks)
    int musicTargetVolume[3];                // +0x10d2f4, +0x10d5d0, +0x10d8ac
    int musicCurrentVolume[3];               // +0x10d2f8, +0x10d5d4, +0x10d8b0
    IDirectSoundStream* musicStreamIfaces[3]; // +0x10d304, (track1 in buffer), +0x10d8bc

    float computedMusicVolumes[3];           // +0x10d31c, +0x10d37c, +0x10d400
    float musicVolumeScales[3];              // +0x10d3bc, +0x10d3c0, +0x10d3c4
    float volumeBlendFactor;                 // +0x10d434
    float masterVolumeScale;                 // +0x10d4cc
    float masterVolumeMult;                  // +0x10d4dc

    // Sound device interface (vtable)
    ISoundDevice* soundDevice;               // +0x10d4e0

    // Frame tracking
    int frameCounter;                        // +0x10d2b0, +0x10d2d0

    // Stack check end
    int stackCheckValue;                     // +0x10d5bc

} SoundManagerGlobals;
```

## Pattern Analysis

### Offset Calculation Pattern
The code uses: `DAT_0010dXXX + OFFSET`

This is typical of:
1. **Position Independent Code (PIC)** - Base + Offset addressing
2. **Multiple base pointers** for different subsystems
3. **Compiler-generated relocation** for global data

### Why Multiple DAT Variables?

Looking at the addresses:
- They increment by 4 bytes each (typical pointer size)
- There are ~90 different DAT variables in the 0x0010d394-0x0010d6f0 range
- That's about 0x35C bytes = 215 pointers

This could mean:
1. **Array of base pointers**: Each DAT is an entry in a relocation table
2. **Compiler optimization**: Splitting one large structure into subsections
3. **Runtime patching**: Each DAT can be independently relocated

### Most Likely Scenario

**Hypothesis**: This is a **Global Offset Table (GOT) pattern** where:
- The original code had a single global `SoundManager` struct
- The compiler/linker split it into multiple relocatable segments
- Each `DAT_0010dXXX` is actually accessing the same base but compiler uses different GOT entries
- At runtime, all these DATs likely resolve to related/sequential addresses

### Recommended Structure Name
```c
typedef struct SoundManager {
    // All fields from above combined
    // The DAT variables are just different ways to access this structure
} SoundManager;

// Global instance (what all the DATs are pointing to different parts of)
extern SoundManager g_SoundManager;
```

## Next Steps for Ghidra

1. **Create the SoundManager structure** with all fields at proper offsets
2. **Find the actual base address** - where is this structure allocated?
3. **Retype the DAT variables** as `SoundManager*` or subsection pointers
4. **Look for initialization** - find where these DAT values are set
5. **Check for relocation table** - there may be a startup routine that sets all these pointers

## Confirmation from SND_Init (0x0010d714)

The `SND_Init` function confirms many of our hypotheses:

### Key Findings:

1. **Same DAT pattern** - Uses `DAT_0010daXX + OFFSET` throughout (different GOT section for init code)
2. **Three music streams created** - Loop creates 3 IDirectSoundStream objects:
   ```c
   // Loop at offset 0x10d924, iterates 3 times
   piVar8 = (int *)(DAT_0010da8c + 0x10d924);
   // Stride: piVar8 = piVar8 + 0xb7 (after each iteration)
   // 0xb7 * 4 bytes = 0x2DC - MATCHES SND_Frame stream stride!
   ```

3. **Stream configurations**:
   - Stream 0: 48000 Hz, 2 channels (stereo)
   - Stream 1: 24000 Hz, 1 channel (mono)
   - Stream 2: 24000 Hz, 2 channels (stereo)

4. **Memory allocation**:
   ```c
   iVar9 = *(int *)(DAT_0010da90 + 0x10d9b8 + iVar6 * 4);  // Array of buffer sizes
   iVar1 = iVar9 + 0x1040;
   iVar7 = XPhysicalAlloc(iVar1, 0xffffffff, 0, 4);  // Allocate physical memory
   // Store buffer at: piVar8[0] = aligned_buffer
   // Store size at: piVar8[1] = iVar9
   ```

5. **DirectSound buffer array**:
   - Loop creates multiple sound buffers (0xBC bytes worth, stride 4 = 47 buffers)
   - Stored at: `DAT_0010da5c + 0x10d7a4`

6. **Primary sound buffer**:
   - Created at: `DAT_0010da68 + 0x10d834`
   - 16-bit stereo, 48kHz
   - 0x4000 byte ring buffer
   - Set to loop continuously

### MusicStream Structure (0x2DC bytes each):

Based on SND_Init allocation and usage:

```c
typedef struct MusicStream {
    // Offset -0x40 from where piVar8 points (piVar8[-0x10])
    char isActive;                      // +0x00 (piVar8[-0x10])
    char pad[3];
    int unknown1;                       // +0x04 (piVar8[-0xC])
    // ... other fields ...

    // Where piVar8 points during init:
    void* audioBuffer;                  // +0x40 (piVar8[0])
    int bufferSize;                     // +0x44 (piVar8[1])
    IDirectSoundStream* streamInterface; // +0x48 (piVar8[2])

    // Total size: 0x2DC bytes (confirmed by stride)
} MusicStream;
```

### Structure Layout Refinement

The init code shows the streams are stored sequentially:
```
Base (DAT_0010da8c + 0x10d924):
  +0x000: Stream 0 (0x2DC bytes) - 48kHz stereo music
  +0x2DC: Stream 1 (0x2DC bytes) - 24kHz mono dialog/effects
  +0x5B8: Stream 2 (0x2DC bytes) - 24kHz stereo ambience
```

This matches the offsets seen in SND_Frame:
- `DAT_0010d6cc + 0x10d304` → Stream 0 interface
- Music stream separation: 0x10d5d0 - 0x10d304 = 0x2CC (close to 0x2DC)
- Full stream: 0x10d8bc - 0x10d304 = 0x5B8 (exactly 2 × 0x2DC)

## Recommended Ghidra Structure Definitions

### 1. MusicStream (0x2DC bytes)

```c
struct MusicStream {
    char name[20];                       // +0x000 - Stream name/path
    int field_0x14;                      // +0x014 - Frame counter or position
    int field_0x18;                      // +0x018 - Bytes to decode/read
    int field_0x1c;                      // +0x01C - Error or state flag
    byte field_0x20;                     // +0x020 - Status flags
    byte padding_1[3];
    File* oggFile;                       // +0x024 - JBE::File* for OGG file
    int field_0x28;                      // +0x028 - Accumulated bytes read
    int bufferSize;                      // +0x02C - Target buffer size
    byte padding_2[0x10];
    void* audioBuffer;                   // +0x040 - PCM audio buffer
    int allocatedSize;                   // +0x044 - Allocated buffer size
    IDirectSoundStream* streamInterface; // +0x048 - DirectSound stream object
    byte padding_3[0x14C];

    // +0x198 onwards - appears to be a queue of stream requests (5 entries)
    uint64_t streamRequest_0[5];         // +0x198 - Stream name (8 bytes each)
    uint64_t streamRequest_1[5];         // +0x1A0
    uint64_t streamRequest_2[5];         // +0x1A8
    uint64_t streamRequest_3[5];         // +0x1B0
    uint field_0x1b8;                    // +0x1B8 - Stream flags (bit 2: paused, bit 3: ???, bit 4: ???)

    byte padding_4[0x11C];
    int requestCount;                    // +0x2D8 - Number of queued stream requests
};
```

### 2. AnimSound (0x4C bytes)

```c
struct AnimSound {
    void* soundData;                     // +0x00 - Pointer to sound data
    int field_0x04;                      // +0x04 - Sound ID or type
    int minDelay;                        // +0x08 - Minimum delay frames
    int maxDelay;                        // +0x0C - Maximum delay frames
    int currentDelay;                    // +0x10 - Current delay countdown
    int field_0x14;                      // +0x14 - Volume position X
    int field_0x18;                      // +0x18 - Volume position Y
    int remainingLoops;                  // +0x1C - Remaining loop count
    float field_0x20;                    // +0x20 - Current animation time
    short field_0x26;                    // +0x26 - Animation index
    void* animationState;                // +0x28 - AnimationState pointer
    Point3 position3D;                   // +0x2C - 3D position (12 bytes)
    float field_0x34;                    // +0x34 - Some multiplier
    int field_0x38;                      // +0x38 - ???
    int isActive;                        // +0x3C - Active flag
    byte field_0x3a;                     // +0x3A - Custom stream flag
    byte field_0x3b;                     // +0x3B - Pause flag
    byte field_0x40;                     // +0x40 - Playing flag
    byte padding[7];
};
```

### 3. DialogEntry (0x2A8 bytes)

```c
struct DialogEntry {
    byte inUse;                          // +0x00 - Entry in use flag
    byte padding_1[0xB];
    int delayMin;                        // +0x0C - Frame delay countdown
    int field_0x10;                      // +0x10 - ???
    int field_0x14;                      // +0x14 - Position X
    char dialogName[41];                 // +0x18 - Dialog identifier string
    byte padding_2[0x27F];
};
```

### 4. StreamSlot (0x20 bytes) - for the 0x400 byte array

```c
struct StreamSlot {
    void* soundDataPtr;                  // +0x00 - Sound data pointer
    int soundId;                         // +0x04 - Sound identifier
    int field_0x08[2];                   // +0x08 - Unknown
    int field_0x10[2];                   // +0x10 - Unknown
    int pendingBytes;                    // +0x18 - Bytes pending
    int errorOrState;                    // +0x1C - Error or state
};
```

### 5. Main SoundManager Structure

```c
struct SoundManager {
    // === DirectSound Core (0x10c5d0 - 0x10c620) ===
    int** stackGuard;                    // +0x10c5d0
    byte padding_0[0xC];
    IDirectSoundBuffer* primaryBuffer;   // +0x10c5e0
    byte padding_1[0x10];
    uint playbackPosition;               // +0x10c5f4
    uint lastPosition;                   // +0x10c5f8
    byte padding_2[0x24];
    float musicVolumeFade;               // +0x10c620
    byte padding_3[0x7C];

    // === Stream Slots (0x10c6a0 - 0x10c71C) ===
    StreamSlot streamSlots[32];          // +0x10c6a0 (0x400 bytes)
    byte padding_4[0x14];
    char pauseFlag;                      // +0x10c6b8 (within padding)
    byte padding_5[0x30];
    int maxInstances;                    // +0x10c6ec
    byte padding_6[0x2C];
    byte busyFlags;                      // +0x10c71c
    byte padding_7[0x2C];

    // === Stream State (0x10c74c - 0x10c910) ===
    char streamReadyFlag;                // +0x10c74c
    byte padding_8[0x18];
    uint currentStreamIndex;             // +0x10c768
    byte padding_9[0xC];
    MusicStream streamData[3];           // +0x10c778 (3 × 0x2DC = 0x894)
    byte padding_10[0x14];
    uint64_t streamSnapshot[5];          // +0x10c8e8 (0x28 bytes)
    uint streamBufferSizes[3];           // +0x10c910
    byte padding_11[0x24];

    // === Main Music Streams (0x10c938 - 0x10D1C4) ===
    MusicStream musicStreams[3];         // +0x10c938 (3 × 0x2DC = 0x894)

    // === OGG Decode State (0x10ccc8 - 0x10cd98) ===
    int currentDecodeStream;             // +0x10ccc8
    int decodeBufferSize;                // +0x10ccd0
    byte padding_12[4];
    int decodeOffset;                    // +0x10ccd8
    byte padding_13[0xC];
    char decodeActive;                   // +0x10cce8
    byte padding_14[0x7C];
    int stream2Index;                    // +0x10cd68
    byte padding_15[0x18];
    int decode2Offset;                   // +0x10cd84
    int bytesDecoded;                    // +0x10cd8c
    byte padding_16[0x8];
    char decode2Active;                  // +0x10cd98
    byte padding_17[0x24];

    // === Animation Sounds (0x10cdc0 - 0x10d740) ===
    AnimSound animSounds[30];            // +0x10cdc0 (30 × 0x4C = 0x8D0)
    float currentBlendWeight;            // +0x10cdc8 (within first animsound)
    int currentAnimIndex;                // +0x10cdcc
    void* animStreamSlotPtr;             // +0x10cdd0
    byte padding_18[0x700];

    // === Dialog System (0x10cf30 - 0x10d7D0) ===
    int dialogQueueCount;                // +0x10cf30
    byte padding_19[0x18];
    DialogEntry dialogQueue[16];         // +0x10cf4c (estimate, stride 0x2A8)
    uint* randomSeedPtr;                 // +0x10cfc8 (within dialog array)
    byte padding_20[0x110];
    float dialogVolumeScale;             // +0x10d0dc
    float dialogVolumeMult;              // +0x10d0e8
    byte padding_21[0x60];
    float dialogVolume1;                 // +0x10d14c
    byte padding_22[0x10];
    IDirectSoundStream* dialogStream0;   // +0x10d160
    byte padding_23[0x40];
    float dialogVolume2;                 // +0x10d1a4
    byte padding_24[0x58];
    byte padding_25[0x3C];
    char dialogFilenameBuffer[256];      // +0x10d2a0
    byte padding_26[0x10];
    int frameCounter;                    // +0x10d2b0
    byte padding_27[0x1C];
    int frameCounter2;                   // +0x10d2d0
    byte padding_28[0x20];

    // === Music Volume Control (0x10d2f4 - 0x10d8BC) ===
    int musicTargetVolume0;              // +0x10d2f4
    int musicCurrentVolume0;             // +0x10d2f8
    byte padding_29[0x8];
    IDirectSoundStream* musicStream0;    // +0x10d304
    byte padding_30[0x14];
    float computedVolume0;               // +0x10d31c
    byte padding_31[0x5C];
    float volumeBlendFactor;             // +0x10d434
    byte padding_32[0x94];
    float masterVolumeScale;             // +0x10d4cc
    byte padding_33[0x4];
    int activeDialogCount;               // +0x10d4d4
    byte padding_34[0x4];
    float masterVolumeMult;              // +0x10d4dc
    ISoundDevice* soundDevice;           // +0x10d4e0 (vtable object)
    byte padding_35[0xE4];
    int musicTargetVolume1;              // +0x10d5d0
    int musicCurrentVolume1;             // +0x10d5d4
    byte padding_36[0x2D4];
    int musicTargetVolume2;              // +0x10d8ac
    int musicCurrentVolume2;             // +0x10d8b0
    byte padding_37[0x8];
    IDirectSoundStream* musicStream2;    // +0x10d8bc

    // More fields likely follow...
};
```

## Summary and Conclusions

### What Are These DAT Variables?

**Confirmed Pattern**: The DAT_XXXX variables in SND_Frame are **NOT separate global variables**. Instead:

1. **Global Offset Table (GOT) Entries** - Each DAT_XXXX is a pointer stored in the GOT section
2. **Relocatable Base Addresses** - All point to different offsets within a single large `SoundManager` global structure
3. **Compiler Optimization** - The compiler generates multiple GOT entries to optimize access patterns
4. **Runtime Relocation** - At load time, these pointers are fixed up to point to the actual allocated memory

### The Truth

There is likely **one large global structure** (possibly named something like `g_SoundManager` or just in BSS) that contains:
- 3 main music streaming channels (OGG Vorbis)
- 30+ animation-triggered sound effect slots
- Dialog queueing and playback system
- 47 DirectSound buffers for sound effects
- Volume fading, mixing, and 3D positioning
- Frame-based update and synchronization

The DAT variables are just **different GOT entries that resolve to different parts of this structure**.

### Why This Pattern?

1. **Xbox Binary** - Original game was for Xbox, which uses PIC (Position Independent Code)
2. **Dynamic Linking** - Allows the binary to be loaded at any address
3. **Multiple Compilation Units** - Different .c files may reference the same structure differently
4. **Optimization** - Compiler can optimize register usage when accessing nearby fields

## Additional Notes

- The function is extremely complex (500+ lines decompiled)
- Handles: music streaming, dialog playback, animation sounds, DirectSound buffer management
- Uses OGG Vorbis decoding (`ov_read`, `ov_clear`, `ov_raw_seek`)
- Three parallel music streams with independent volume control
- Dialog system with random selection and filename resolution
- Frame-based volume fading with multiple fade rates
- Physical memory allocation via `XPhysicalAlloc` (Xbox API) for stream buffers
- All buffers are 64-byte aligned (typical for DMA/cache alignment)
