#ifndef __SOUND_MGMT_H__
#define __SOUND_MGMT_H__

#include <stdint.h>

// Forward declarations
typedef struct OggStream OggStream;
typedef struct File File;

/**
 * Individual sound stream slot within a channel
 * Size: 0x28 bytes (40 bytes)
 * Each channel can have up to 8 active streams
 */
// typedef struct {
//     char path[16];           // 0x00 - Path to audio file (usually shortened, like "music/battle.ogg")
//     int startSample;         // 0x10 - Start sample position (param_4 from SND_StartStream)
//     int calculatedValue;     // 0x14 - Rounded sample count: (endSample / 0x48) * 0x48
//     int startSample2;        // 0x18 - Copy of startSample (for seeking?)
//     int endSample;           // 0x1c - End sample position (param_5 from SND_StartStream)
//     uint32_t flags;          // 0x20 - Stream flags:
//                              //        bit 0: playing/active
//                              //        bit 1: fade command
//                              //        bit 2: unknown
//                              //        bit 3: paused (set/clear by continue stream)
//     OggStream* oggStreamPtr; // 0x24 - Pointer to OggStream object (allocated via operator_new)
// } SoundStreamSlot;           // Total size: 0x28

/**
 * Sound channel structure - manages multiple concurrent audio streams
 * Size: 0x2dc bytes (732 bytes)
 * The game has 3 channels total:
 *   - Channel 0: Background music
 *   - Channel 1: Sound effects
 *   - Channel 2: Dialog/voice
 */
typedef struct {
    char stateFlag;              // 0x00 - Channel state flag (0=ready, 1=busy?)
    char pad_0x01[0x0f];         // 0x01
    int64_t timeAccumulator;     // 0x10 - Time or sample accumulator
    int countdown;               // 0x14 - Countdown value (decremented each frame)
    int64_t field_0x18;          // 0x18 - Unknown int64
    uint32_t field_0x20;         // 0x20 - Status or flags
    File* filePtr;               // 0x24 - File pointer (for streaming?)
    int field_0x28;              // 0x28 - Unknown int
    uint32_t field_0x2c;         // 0x2c - Unknown uint
    char unknown_0x30[0x08];     // 0x30 - Padding or unknown
    int bufferSize;              // 0x38 - Buffer size (commonly set to 0x3fff)
    char unknown_0x3c[0x0c];     // 0x3c - Unknown fields
    void* objectWithVtable;      // 0x48 - Pointer to object with virtual methods
    char unknown_0x4c[0x14c];    // 0x4c - Unknown data (possibly DirectSound buffers, etc.)

    // Stream array starts at offset 0x198
    SoundStreamSlot streams[8];  // 0x198 - Array of 8 stream slots (0x140 bytes total)
    int streamCount;             // 0x2d8 - Number of currently active streams (0-8)
} SoundChannel;                  // Total size: 0x2dc

/**
 * Global sound management system
 * Contains 3 sound channels at offsets:
 *   Channel 0: base + 0x000  (streams at base + 0x198, count at base + 0x2d8)
 *   Channel 1: base + 0x2dc  (streams at base + 0x474, count at base + 0x5b4)
 *   Channel 2: base + 0x5b8  (streams at base + 0x750, count at base + 0x890)
 */
typedef struct {
    SoundChannel channels[3];    // 3 channels * 0x2dc = 0x894 bytes
} SoundSystem;

#endif // __SOUND_MGMT_H__
