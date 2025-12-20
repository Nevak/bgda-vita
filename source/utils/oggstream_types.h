#include <stdio.h>
typedef struct OggStream OggStream, *POggStream;

typedef struct File File, *PFile;
typedef struct SoundChannel SoundChannel, *PSoundChannel;

// __attribute__((packed)) struct File { /* PlaceHolder Structure */
//     uint32_t field0_0x0;
//     uint32_t field1_0x1;
//     uint32_t field2_0x2;
//     uint32_t field3_0x3;
//     int baseOffset; /* Created by retype action */
//     uint32_t field5_0x8;
//     uint32_t field6_0x9;
//     uint32_t field7_0xa;
//     uint32_t field8_0xb;
//     int length;
//     int startOffset;
//     int ramFSptr;
//     uint position;
// };

__attribute__((packed)) struct File { /* PlaceHolder Structure */
    uint32_t field0_0x0;
    int baseOffset; /* Created by retype action */
    uint32_t field5_0x8;
    int length;
    int startOffset;
    int ramFSptr;
    uint position;
};

__attribute__((packed)) struct OggStream { /* PlaceHolder Structure */
    struct File file;
    uint32_t field1_0x1c;
    uint32_t *vorbisFile; /* Created by retype action */
    uint8_t pad[716];
    uint8_t isOggS; /* Created by retype action */
};

typedef struct __attribute__((packed)) StreamSlot { /* Individual sound stream slot within a channel */
    char path[16];
    int startSample;
    int roundedSampleCount; /* Rounded sample count: (endSample / 0x48) * 0x48 */
    int startSample2;
    int endSample;
    int statusOrFlags; /* Stream flags:                              //        bit 0: playing/active                              //        bit 1: fade command                              //        bit 2: unknown                              //        bit 3: paused (set/clear by continue stream)     */
    OggStream *oggStream;
}StreamSlot;

struct __attribute__((packed)) SoundChannel {
    uint8_t pad[0x10];
    int timeAccumulator; /* Created by retype action */
    int countdown; /* Created by retype action */
    uint64_t pad2;
    int pad3;
    struct OggStream *oggStreamPtr;
    int pad4;
    uint pad5;
    uint8_t pad6[8];
    int bufferSize; /* Created by retype action */
    uint8_t pad7[348];
    struct StreamSlot streamSlots[8];
    int streamCount; /* Created by retype action */
};

typedef struct SoundSystem {
    SoundChannel channels[3];
}SoundSystem;


_Static_assert(sizeof(struct File) == 0x1C, "File must be packed to 0x1C");
_Static_assert(sizeof(struct StreamSlot) == 0x28, "StreamSlot must be packed to 0x28");
_Static_assert(sizeof(struct SoundChannel) == 0x2dc, "SoundChannel must be packed to 0x2dc");

_Static_assert(offsetof(SoundChannel, streamSlots) == 0x198,
               "`streamSlots` is not at the ritght offset - fix the struct or add packed!");
_Static_assert(offsetof(SoundChannel, streamCount) == 0x2D8,
               "`streamCount` is not at the ritght offset - fix the struct or add packed!");
_Static_assert(offsetof(StreamSlot, oggStream) == 0x24,
               "`oggStream` is not at the ritght offset - fix the struct or add packed!");

_Static_assert(offsetof(OggStream, isOggS) == 0x2f0, "`isOggS` is not at the ritght offset - fix the struct or add packed!");