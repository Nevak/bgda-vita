# SND_StartStream Analysis

## Function Signature
```c
void SND_StartStream(int channelId, char *path, int streamFlags, int startSample, int endSample)
```

## Parameters
- **channelId**: Sound channel (0=music, 1=sfx, 2=dialog)
- **path**: File path to OGG audio file
- **streamFlags**: Stream control flags
- **startSample**: Starting sample position
- **endSample**: Ending sample position (-1 to use file's natural end)

## Structure Layout

### SoundChannel (0x2dc bytes per channel)
```
Offset  | Size | Field              | Description
--------|------|--------------------|-----------------------------------------
0x000   | 0x1  | stateFlag          | Channel state
...     | ...  | ...                | Various channel state fields
0x198   | 0x140| streams[8]         | Array of 8 stream slots (0x28 each)
0x2d8   | 0x4  | streamCount        | Number of active streams (0-8)
```

### SoundStreamSlot (0x28 bytes per slot)
```
Offset  | Size | Field              | Description
--------|------|--------------------|-----------------------------------------
+0x00   | 0x10 | path[16]           | File path string (copied from param)
+0x10   | 0x4  | startSample        | Start sample position
+0x14   | 0x4  | calculatedValue    | (endSample/0x48)*0x48 - rounded value
+0x18   | 0x4  | startSample2       | Duplicate of startSample (for seeking?)
+0x1c   | 0x4  | endSample          | End sample position
+0x20   | 0x4  | flags              | Stream flags (play/pause/fade)
+0x24   | 0x4  | oggStreamPtr       | Pointer to OggStream object
```

### Absolute Offsets (when accessing via channelBase + streamIndex * 0x28)
Since stream array starts at +0x198 in the channel:
- **0x198**: path[0]
- **0x1a8**: startSample (+0x10)
- **0x1ac**: calculatedValue (+0x14)
- **0x1b0**: startSample2 (+0x18)
- **0x1b4**: endSample (+0x1c)
- **0x1b8**: flags (+0x20)
- **0x1bc**: oggStreamPtr (+0x24)

## Key Logic

1. **Check capacity**: Verify streamCount < 8 before adding
2. **Lookup file**: Call `cdDirectoryLookup()` to verify file exists
3. **Allocate OggStream**: `operator_new(0x2f8)` allocates OggStream object
4. **Initialize slot**:
   - Calculate slot address: `channelBase + streamCount * 0x28 + 0x198`
   - Copy path (16 bytes max)
   - Store sample positions
   - Store calculated value: `(endSample / 0x48) * 0x48`
   - Store flags
   - Store OggStream pointer
5. **Increment counter**: `streamCount++`

## OggStream Constructor Call
```c
OggStream::OggStream(oggStreamObj, realPath, &startSampleValue, &endSample, channelId)
```

## Notes
- The `0x48` divisor/multiplier in the calculated value (72 decimal) is likely related to audio frame alignment
- Stream slots are filled sequentially based on streamCount
- Maximum 8 concurrent streams per channel (24 total across all channels)
- Each channel is independently managed at offsets: 0x0, 0x2dc, 0x5b8
