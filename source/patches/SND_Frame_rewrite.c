/**
 * SND_Frame - Main audio system frame update
 *
 * This function is called every frame to:
 * - Update DirectSound buffers
 * - Handle music volume fading during dialogs
 * - Process OGG Vorbis stream decoding
 * - Manage animation sound effects
 * - Handle dialog queuing and playback
 * - Update volume levels for all channels
 *
 * Original address: 0x0010c5a4
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// STRUCTURE DEFINITIONS
// ============================================================================

typedef struct Point3 {
    float x, y, z;
} Point3;

typedef struct AnimationState AnimationState;

typedef struct File File;

// OGG Vorbis stream structure (partial)
typedef struct {
    // OGG Vorbis file state
    void* oggVorbisFile;  // OggVorbis_File structure
    // ... other fields
} OggStream;

// Stream slot for pending sounds (0x20 bytes each)
typedef struct {
    void* soundDataPtr;       // +0x00
    int soundId;              // +0x04
    int unknown1[2];          // +0x08
    int unknown2[2];          // +0x10
    int pendingBytes;         // +0x18 - bytes to play
    int errorOrState;         // +0x1C
} StreamSlot;

// Music/audio stream channel (0x2DC bytes each)
typedef struct {
    char name[20];                  // +0x000 - Stream name/path
    int frameCounter;               // +0x014
    int bytesToRead;                // +0x018 - Bytes pending to decode
    int errorState;                 // +0x01C
    uint8_t statusFlags;            // +0x020
    uint8_t pad1[3];
    File* oggFile;                  // +0x024 - JBE::File*
    int accumulatedBytesRead;       // +0x028
    int targetBufferSize;           // +0x02C
    uint8_t pad2[0x10];
    void* audioBuffer;              // +0x040 - PCM output buffer
    int allocatedSize;              // +0x044
    void* streamInterface;          // +0x048 - IDirectSoundStream*
    uint8_t pad3[0x14C];

    // Stream request queue (5 entries)
    uint64_t requests[5 * 5];       // +0x198 - 5 requests × 5 fields each
    uint32_t streamFlags;           // +0x1B8
    // bit 1: fade requested
    // bit 2: stream active
    // bit 3: paused

    uint8_t pad4[0x11C];
    int requestCount;               // +0x2D8 - Number of queued requests
} MusicStream;

// Animation sound effect slot (0x4C bytes)
typedef struct {
    void* soundData;            // +0x00
    int soundType;              // +0x04
    int minDelayFrames;         // +0x08
    int maxDelayFrames;         // +0x0C
    int currentDelay;           // +0x10 - countdown
    int volumePosX;             // +0x14
    int volumePosY;             // +0x18
    int remainingLoops;         // +0x1C
    float animationTime;        // +0x20
    uint8_t pad1[2];
    short animIndex;            // +0x26
    AnimationState* animState;  // +0x28
    Point3 position3D;          // +0x2C
    float volumeMultiplier;     // +0x34
    int field_38;               // +0x38
    int isActive;               // +0x3C
    uint8_t useCustomStream;    // +0x3A
    uint8_t pauseFlag;          // +0x3B
    uint8_t isPlaying;          // +0x40
    uint8_t pad2[7];
} AnimSound;

// Dialog queue entry (0x2A8 bytes)
typedef struct {
    uint8_t inUse;              // +0x00
    uint8_t pad1[0xB];
    int delayCountdown;         // +0x0C - frame delay
    int minDelay;               // +0x10
    int field_14;               // +0x14
    char dialogName[41];        // +0x18 - dialog identifier
    uint8_t pad2[0x27F];
} DialogEntry;

// Main sound manager global structure
typedef struct {
    // === DirectSound Core ===
    int** stackGuard;                       // +0x10c5d0
    uint8_t pad_core[0xC];
    void* primaryBuffer;                    // +0x10c5e0 - IDirectSoundBuffer*
    uint8_t pad_buf[0x10];
    uint32_t playbackPosition;              // +0x10c5f4
    uint32_t lastPosition;                  // +0x10c5f8
    uint8_t pad_pos[0x24];
    float musicVolumeFade;                  // +0x10c620 - current fade multiplier
    uint8_t pad_fade[0x7C];

    // === Stream Slots ===
    StreamSlot streamSlots[32];             // +0x10c6a0 (0x400 bytes)
    uint8_t pad_slots1[0x14];
    char pauseFlag;                         // +0x10c6b8
    uint8_t pad_slots2[0x30];
    int maxSoundInstances;                  // +0x10c6ec
    uint8_t pad_slots3[0x2C];
    uint8_t busyFlags;                      // +0x10c71c (CD/texture loading)
    uint8_t pad_busy[0x2C];

    // === Stream State ===
    char streamReadyFlag;                   // +0x10c74c
    uint8_t pad_ready[0x18];
    uint32_t currentStreamIndex;            // +0x10c768
    uint8_t pad_idx[0xC];
    MusicStream streamData[3];              // +0x10c778
    uint8_t pad_stream[0x14];
    uint64_t streamSnapshot[5];             // +0x10c8e8
    uint32_t streamBufferSizes[3];          // +0x10c910
    uint8_t pad_bufsize[0x24];

    // === Main Music Streams ===
    MusicStream musicStreams[3];            // +0x10c938

    // === OGG Decode State ===
    int currentDecodeStream;                // +0x10ccc8
    int decodeBufferSize;                   // +0x10ccd0
    uint8_t pad_dec1[4];
    int decodeOffset;                       // +0x10ccd8
    uint8_t pad_dec2[0xC];
    char decodeActive;                      // +0x10cce8
    uint8_t pad_dec3[0x7C];
    int stream2Index;                       // +0x10cd68
    uint8_t pad_dec4[0x18];
    int decode2Offset;                      // +0x10cd84
    int bytesDecoded;                       // +0x10cd8c
    uint8_t pad_dec5[0x8];
    char decode2Active;                     // +0x10cd98
    uint8_t pad_dec6[0x24];

    // === Animation Sounds ===
    AnimSound animSounds[30];               // +0x10cdc0
    float currentBlendWeight;               // +0x10cdc8
    int currentAnimIndex;                   // +0x10cdcc
    void* animStreamSlotPtr;                // +0x10cdd0
    uint8_t pad_anim[0x65C];
    int pauseState;                         // +0x10ce30 - 1 = paused, 0 = playing
    uint8_t pad_anim2[0xA0];

    // === Dialog System ===
    int dialogQueueCount;                   // +0x10cf30
    uint8_t pad_dlg1[0x18];
    DialogEntry dialogQueue[16];            // +0x10cf4c
    uint32_t* randomSeedPtr;                // +0x10cfc8
    uint8_t pad_dlg2[0x110];
    float dialogVolumeScale;                // +0x10d0dc
    float dialogVolumeMult;                 // +0x10d0e8
    uint8_t pad_dlg3[0x60];
    float dialogVolume1;                    // +0x10d14c
    uint8_t pad_dlg4[0x10];
    void* dialogStream0;                    // +0x10d160 - IDirectSoundStream*
    uint8_t pad_dlg5[0x40];
    float dialogVolume2;                    // +0x10d1a4
    uint8_t pad_dlg6[0x58];
    uint8_t pad_dlg7[0x3C];
    char dialogFilenameBuffer[256];         // +0x10d2a0
    uint8_t pad_dlg8[0x10];
    int frameCounter;                       // +0x10d2b0
    uint8_t pad_frame1[0x1C];
    int frameCounter2;                      // +0x10d2d0
    uint8_t pad_frame2[0x20];

    // === Music Volume Control ===
    int musicTargetVolume[3];               // +0x10d2f4, +0x10d5d0, +0x10d8ac
    int musicCurrentVolume[3];              // +0x10d2f8, +0x10d5d4, +0x10d8b0
    uint8_t pad_mvol1[0x8];
    void* musicStreamIface0;                // +0x10d304 - IDirectSoundStream*
    uint8_t pad_mvol2[0x14];
    float computedVolume[3];                // +0x10d31c, +0x10d37c, +0x10d400
    uint8_t pad_mvol3[0x5C];
    float volumeBlendFactor;                // +0x10d434
    uint8_t pad_mvol4[0x94];
    float masterVolumeScale;                // +0x10d4cc
    uint8_t pad_mvol5[0x4];
    int activeDialogCount;                  // +0x10d4d4
    uint8_t pad_mvol6[0x4];
    float masterVolumeMult;                 // +0x10d4dc
    void* soundDevice;                      // +0x10d4e0 - ISoundDevice* (vtable)

    uint8_t pad_end[0x2D4];
    void* musicStreamIface2;                // +0x10d8bc - IDirectSoundStream*
    uint8_t pad_final[0x2FC];
    int stackCheckValue;                    // +0x10d5bc
} SoundManager;

// ============================================================================
// EXTERNAL FUNCTIONS
// ============================================================================

// DirectSound
extern int IDirectSoundBuffer_GetCurrentPosition(void* buffer, uint32_t* playCursor, uint32_t* writeCursor);
extern void DirectSoundDoWork(void);
extern void IDirectSoundStream_SetVolume(void* stream, int volume);

// Dialog system
extern int DLG_IsDialogRunning(void);
extern void* SND_GetDialogDir(void);
extern void SND_GetDialogFilename(int index);
extern void SND_StartStream(int channel, const char* path, int flags, int startSample, int endSample);

// OGG Vorbis
extern long ov_read(void* vf, void* buffer, int length, int bigendianp, int word, int sgned, int* bitstream);
extern int ov_clear(void* vf);
extern int ov_raw_seek(void* vf, int* pos, int unused1, int unused2);

// Game systems
extern int cdIsBusy(void);
extern int texIsBusyLoading(void);
extern void pauseProcess(void);
extern void animAdvanceAnimation(AnimationState* anim, Point3 position);
extern int animGetVolumeScale(int posX, int posY);

// Sound system
extern void SND_UpdateMusicVolume(void);
extern void SND_PlaySoundNew(void* soundData, int soundId, float volume1, float volume2, float volume3);

// Utilities
extern void FUN_0010b930(int code, void* param);
extern int FUN_001da280(float value);  // Likely log10 or dB conversion
extern void FUN_0027c2c0(int val1, int val2);
extern void FUN_0027c8b4(uint32_t random, int count);

// ============================================================================
// GLOBALS (these would be properly declared in headers)
// ============================================================================

extern SoundManager* g_SoundManager;

// Float constants
extern float DIALOG_FADE_TARGET;      // 0.5 typically
extern float DIALOG_FADE_RATE;
extern float MUSIC_FADE_RATE;
extern float RANDOM_SOUND_FACTOR;
extern float FRAME_TIME_DELTA;
extern float MIN_VOLUME_THRESHOLD;
extern float VOLUME_SCALE_DIVISOR;
extern float VOLUME_SCALE_MULTIPLIER;
extern float VOLUME_TO_DB_MIN;
extern float VOLUME_TO_DB_DIV;
extern float VOLUME_TO_DB_MUL;
extern float STREAM_FRAME_TIME_MULT;
extern float GLOBAL_VOLUME_SCALE;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Update music volume fade based on dialog state
 */
static void UpdateMusicVolumeFade(SoundManager* snd) {
    float* fadePtr = &snd->musicVolumeFade;
    float currentFade = *fadePtr;

    int dialogRunning = DLG_IsDialogRunning();

    if (!dialogRunning) {
        // Fade music back up to full volume
        if (currentFade < 1.0f) {
            currentFade += MUSIC_FADE_RATE;
            if (currentFade >= 1.0f) {
                currentFade = 1.0f;
            }
            *fadePtr = currentFade;
        }
    } else {
        // Fade music down to dialog fade target (typically 0.5)
        if (currentFade != DIALOG_FADE_TARGET) {
            if (currentFade < DIALOG_FADE_TARGET) {
                currentFade += DIALOG_FADE_RATE;
                if (currentFade > DIALOG_FADE_TARGET) {
                    currentFade = DIALOG_FADE_TARGET;
                }
            }
            *fadePtr = currentFade;
        }
    }

    // If fade changed, update music volume
    if (*fadePtr != currentFade) {
        SND_UpdateMusicVolume();
    }
}

/**
 * Process pending stream slots and play sounds
 */
static void ProcessStreamSlots(SoundManager* snd) {
    StreamSlot* slots = snd->streamSlots;

    for (int i = 0; i < 32; i++) {
        StreamSlot* slot = &slots[i];

        // Skip if paused and stream is still active
        if (snd->pauseFlag == 1 && slot->errorOrState != 0) {
            continue;
        }

        // Check if this slot has pending audio
        int pendingBytes = slot->pendingBytes;
        if (pendingBytes > 0 && pendingBytes <= snd->maxSoundInstances) {
            // Play the sound
            SND_PlaySoundNew(
                slot->soundDataPtr,
                slot->soundId,
                snd->musicVolumeFade,
                snd->musicVolumeFade,
                1.0f
            );

            // Clear pending bytes
            slot->pendingBytes = 0;
        }
    }
}

/**
 * Process OGG Vorbis streaming for a music channel
 */
static int ProcessOggStream(SoundManager* snd, int streamIndex, MusicStream* stream) {
    int totalBytesRead = 0;

    // Check if stream is ready and has data to decode
    if (stream->name[0] != '\0' && stream->bytesToRead > 0) {
        OggStream* oggStream = (OggStream*)stream->oggFile;
        if (!oggStream) return 0;

        int bytesToDecode = stream->bytesToRead;
        uint16_t* outputBuffer = (uint16_t*)(stream->audioBuffer);

        // Read OGG data in chunks until buffer is filled
        while (bytesToDecode > 0) {
            int bitstream;
            long bytesRead = ov_read(
                &oggStream->oggVorbisFile,
                outputBuffer,
                bytesToDecode,
                0,      // little endian
                2,      // 16-bit samples
                1,      // signed
                &bitstream
            );

            if (bytesRead <= 0) break;  // EOF or error

            totalBytesRead += bytesRead;
            outputBuffer = (uint16_t*)((uint8_t*)outputBuffer + bytesRead);
            bytesToDecode -= bytesRead;
        }

        // Update stream state
        stream->accumulatedBytesRead += totalBytesRead;
        stream->bytesToRead = 0;

        // Store decode info
        snd->currentDecodeStream = streamIndex;
        snd->decodeBufferSize = totalBytesRead;
        snd->decodeActive = 1;
    }

    return totalBytesRead;
}

/**
 * Convert linear volume (0-1) to decibels (-10000 to 0)
 */
static int VolumeToDecibels(float linearVolume) {
    // Clamp to minimum threshold
    if (linearVolume < MIN_VOLUME_THRESHOLD) {
        linearVolume = MIN_VOLUME_THRESHOLD;
    }

    // Convert using logarithm (FUN_001da280 is likely log10 or similar)
    float dbValue = (float)FUN_001da280(linearVolume);

    // Scale to DirectSound range
    int dsVolume = (int)((dbValue / VOLUME_TO_DB_DIV) * VOLUME_TO_DB_MUL);

    // Clamp to valid range
    if (dsVolume < -10000) {
        dsVolume = -10000;
    }

    return dsVolume;
}

/**
 * Update volume levels for music streams
 */
static void UpdateMusicStreamVolumes(SoundManager* snd) {
    // Smoothly interpolate current volumes toward target volumes
    for (int i = 0; i < 3; i++) {
        int target = snd->musicTargetVolume[i];
        int current = snd->musicCurrentVolume[i];

        // Move current toward target by 512 units per frame
        if (current < target) {
            current += 0x200;
            if (current > target) current = target;
        } else if (current > target) {
            current -= 0x200;
            if (current < target) current = target;
        }

        snd->musicCurrentVolume[i] = current;

        // Compute actual volume as float
        snd->computedVolume[i] = (float)current * STREAM_FRAME_TIME_MULT;
    }

    // Apply volumes to DirectSound streams
    void* streamIfaces[3] = {
        snd->musicStreamIface0,
        snd->musicStreamIface0,  // TODO: get correct interface for stream 1
        snd->musicStreamIface2
    };

    float volumeScales[3] = {
        snd->masterVolumeScale * snd->masterVolumeMult,
        snd->volumeBlendFactor * snd->dialogVolume1,
        snd->computedVolume[2] * snd->dialogVolume2
    };

    for (int i = 0; i < 3; i++) {
        if (streamIfaces[i]) {
            int dbVolume = VolumeToDecibels(volumeScales[i]);
            IDirectSoundStream_SetVolume(streamIfaces[i], dbVolume);
        }
    }
}

/**
 * Process animation sound effects
 */
static void ProcessAnimationSounds(SoundManager* snd, void* streamSlot) {
    AnimSound* animSounds = snd->animSounds;

    for (int i = 0; i < 30; i++) {
        AnimSound* sound = &animSounds[i];

        // Skip inactive sounds
        if (!sound->isActive) continue;

        // Check if sound has timed out (remainingLoops field is actually at offset 0x3C = isActive)
        // Note: The offset 0x3C in Ghidra corresponds to isActive, not remainingLoops
        if (sound->isActive == 0) {
            // Check if animation has valid sound events
            AnimationState* animState = sound->animState;  // offset 0x28

            // animState->soundEventArray at offset 0x38
            void** soundEventArray = (void**)((char*)animState + 0x38);

            if (*soundEventArray == NULL) {
                // No sound event array - deactivate this sound
                sound->soundType = 0;  // Marks sound as inactive (offset 0x04)
                continue;
            }

            // Check if the specific animation index has a valid event
            // soundEventArray[animIndex * 0x1C]
            int* eventData = (int*)(*soundEventArray);
            int eventValue = eventData[sound->animIndex * 0x1C / 4];

            if (eventValue == 0x7FFFFFFF) {
                // Invalid/end marker - deactivate this sound
                sound->soundType = 0;
                continue;
            }
        } else {
            // isActive != 0: Check if animation time needs reset
            AnimationState* animState = sound->animState;

            // Get target animation time from animState offset 0x40
            int* targetTimePtr = (int*)((char*)animState + 0x40);
            float targetTime = (float)*targetTimePtr;

            float currentTime = sound->animationTime;

            // If current time has reached or exceeded target, reset to 0
            if (currentTime >= targetTime) {
                sound->animationTime = 0.0f;
            }
        }

        // Check global pause state and individual sound pause
        bool globalPaused = (snd->pauseState == 1);  // Global pause at offset 0x10ce30
        bool shouldPlay = !globalPaused || !sound->pauseFlag;

        if (shouldPlay) {
            // Update position and volume from animation state or custom stream
            Point3 position = sound->position3D;
            float volumeMult = sound->volumeMultiplier;

            // Get data from animation state
            AnimationState* animState = sound->animState;
            if (animState) {
                // animState has a pointer at offset 0x0C to another structure
                void* dataPtr = *(void**)((char*)animState + 0x0C);
                if (dataPtr) {
                    // Read position (Point3 at offset 0x14) and volume mult (float at 0x1C)
                    position = *(Point3*)((char*)dataPtr + 0x14);
                    volumeMult = *(float*)((char*)dataPtr + 0x1C);

                    // Update sound fields
                    sound->position3D = position;
                    sound->volumeMultiplier = volumeMult;
                }
            }

            // Override with custom stream data if enabled
            if (sound->useCustomStream) {
                // streamSlot contains custom position data (8 bytes for position, then float for volume)
                position = *(Point3*)streamSlot;
                volumeMult = *((float*)streamSlot + 2);

                sound->position3D = position;
                sound->volumeMultiplier = volumeMult;
            }

            // Store current animation index and blend weight for global state
            snd->currentAnimIndex = i;
            snd->currentBlendWeight = sound->volumeMultiplier;

            // Advance the animation with updated position
            animAdvanceAnimation(sound->animState, sound->position3D);
        }
    }
}

/**
 * Process dialog queue and trigger dialog playback
 */
static void ProcessDialogQueue(SoundManager* snd) {
    // Skip if dialogs are already active or system is busy
    if (snd->activeDialogCount > 0) return;
    if (snd->dialogQueueCount <= 0) return;

    DialogEntry* dialogQueue = snd->dialogQueue;

    for (int i = 0; i < snd->dialogQueueCount; i++) {
        DialogEntry* entry = &dialogQueue[i];

        // Decrement delay countdown
        int countdown = entry->delayCountdown;
        entry->delayCountdown = countdown - 1;

        if (countdown < 1) {
            // Generate new random delay
            uint32_t randSeed = *snd->randomSeedPtr;
            randSeed = randSeed * 0x19660d + 0x3c6ef35f;
            *snd->randomSeedPtr = randSeed;

            float randomFactor = (float)(randSeed >> 16) * RANDOM_SOUND_FACTOR;
            int newDelay = (int)(((float)entry->minDelay + randomFactor *
                                 (float)(entry->field_14 - entry->minDelay)) *
                                 FRAME_TIME_DELTA);
            entry->delayCountdown = newDelay;

            // Check if it's time to play this dialog
            if (newDelay < 1) {
                // Look up dialog file
                void* dialogDir = SND_GetDialogDir();
                if (dialogDir) {
                    // Search for matching dialog name
                    // ... dialog directory search logic
                    // Then call SND_StartStream() with dialog channel
                }
            }
        }
    }
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

/**
 * Main audio system frame update
 * Called every frame to update all audio subsystems
 */
void SND_Frame(void) {
    SoundManager* snd = g_SoundManager;

    // Stack guard check at function entry
    int stackGuardEntry = **snd->stackGuard;

    // ========================================================================
    // 1. UPDATE DIRECTSOUND BUFFER POSITION
    // ========================================================================

    uint32_t soundWriteCursor = 0;
    IDirectSoundBuffer_GetCurrentPosition(snd->primaryBuffer, &soundWriteCursor, NULL);

    // Handle buffer wraparound (16KB ring buffer)
    uint32_t lastPos = snd->lastPosition;
    if ((int)soundWriteCursor < (int)(lastPos & 0x3FFF)) {
        lastPos = lastPos + 0x4000;  // Wrapped around
    }
    snd->playbackPosition = (lastPos & 0xFFFC000) | soundWriteCursor;

    // Process DirectSound work
    DirectSoundDoWork();

    // ========================================================================
    // 2. UPDATE MUSIC VOLUME FADE
    // ========================================================================

    UpdateMusicVolumeFade(snd);

    // ========================================================================
    // 3. PROCESS STREAM SLOTS
    // ========================================================================

    ProcessStreamSlots(snd);

    // ========================================================================
    // 4. HANDLE STREAMING AND DECODING
    // ========================================================================

    pauseProcess();

    // Only process streaming if system is not busy
    bool systemBusy = (snd->busyFlags & 3) != 0 || cdIsBusy() || texIsBusyLoading();

    if (!systemBusy && snd->streamReadyFlag) {
        // Clear ready flag
        snd->streamReadyFlag = 0;

        // Get current stream index
        uint32_t streamIdx = snd->currentStreamIndex;
        MusicStream* stream = &snd->musicStreams[streamIdx];

        // Update stream counters
        int bytesRead = stream->bytesToRead;
        stream->frameCounter += bytesRead;
        stream->bytesToRead = bytesRead - bytesRead;  // Clear

        int totalBytesRead = stream->accumulatedBytesRead;
        if (totalBytesRead > 0) {
            // Process OGG decoding
            if (stream->name[0] == '\0') {
                // Check if stream should auto-continue
                if (stream->streamFlags & 0x2D8) {
                    uint32_t channelFlags = (uint32_t)(stream->streamFlags >> 24);
                    if (channelFlags != 0) {
                        FUN_0010b930(0x40, &streamIdx);
                    }
                }
            }

            // Decode OGG stream
            if (totalBytesRead > 0) {
                ProcessOggStream(snd, streamIdx, stream);
            }
        }

        // Handle OGG seeking if needed
        if (stream->oggFile && (stream->statusFlags & 1)) {
            OggStream* oggStream = (OggStream*)stream->oggFile;
            int seekPos = (stream->calculatedValue / 0x48) * 0x48;
            ov_raw_seek(&oggStream->oggVorbisFile, &seekPos, 0, 0);
        }
    }

    // ========================================================================
    // 5. PROCESS ANIMATION SOUNDS
    // ========================================================================

    FUN_0010b930(0x50, NULL);

    // Load stream slot pointer for animation sounds
    void* streamSlot = snd->animStreamSlotPtr;

    ProcessAnimationSounds(snd, streamSlot);

    // ========================================================================
    // 6. PROCESS DIALOG QUEUE
    // ========================================================================

    ProcessDialogQueue(snd);

    // ========================================================================
    // 7. UPDATE VOLUME LEVELS
    // ========================================================================

    // Increment frame counter
    snd->frameCounter++;
    snd->frameCounter2 = snd->frameCounter;

    // Update music stream volumes
    UpdateMusicStreamVolumes(snd);

    // ========================================================================
    // 8. SOUND DEVICE UPDATE
    // ========================================================================

    // Call sound device virtual method (vtable + 0xC)
    if (snd->soundDevice) {
        void** vtable = *(void***)snd->soundDevice;
        void (*updateFunc)(void*, uint32_t*) = (void(*)(void*, uint32_t*))vtable[3];  // offset 0xC / 4 = 3
        updateFunc(snd->soundDevice, &soundWriteCursor);
    }

    // ========================================================================
    // 9. STACK GUARD CHECK
    // ========================================================================

    if (snd->stackCheckValue != stackGuardEntry) {
        // Stack corruption detected!
        __builtin_trap();  // or call __stack_chk_fail()
    }
}
