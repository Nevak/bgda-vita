# Baldur's Gate: Dark Alliance Vita

<p align="center"><img src="./screenshots/2026-01-04-010811.jpg"></p>
<p align="center"><img src="./screenshots/2025-12-06-171920.jpg"></p>

This is a wrapper/port of <b>Baldur's Gate: Dark Alliance</b> for the *PS Vita*.

The port works by loading the official Android ARMv7 executables in memory, resolving its imports with native functions and patching it in order to properly run.
By doing so, it's basically as if we emulate a minimalist Android environment in which we run natively the executable as is.

# Changelog
### v1.06
- Substantially improved video playback performance by passing YUV data directly to GXM, avoiding CPU color space conversion.
- Added playstation button icons and PS Vita diagram in control settings screen. Notes:
  - This works by automatically patching the original game files when the game starts. The game creates automatic backups of files before applying patches (e.g., `config.lmp.backup`). Backups are only created once and won't be overwritten. The patching code was written with the help of AI tools.
  - This was achieved in part thanks to the cool [BGDA Explorer tool for PS2 by Bryce Barbara](https://github.com/bigianb/bgda-explorer). The tool was adapted to work with the Xbox texture format and modified to support importing new textures. I will share the modified fork of BGDA Explorer in the future.
- Eliminated periodic framerate stutter every 3-4 seconds. Moved Vorbis audio decoding to a background thread on CPU core 2. Uses a [custom fork of libvorbis](https://github.com/Nevak/vorbis-vita/tree/rulas) with a lock-free ring buffer that pre-fills decoded PCM data so `ov_read()` returns instantly via memcpy. Included as a submodule at `lib/libvorbis` and built automatically by the build script. Note: The code for this custom fork was written with the help of AI tools, by creating tests to ensure the original libvorbis and the threaded version behave exactly the same for this game's use case.


### v1.05
- Fixed glitched textures when playing for a prolonged period of time. Caused by leaked texture slots in previous version of VitaGL. See the [commit](https://github.com/Rinnegatamante/vitaGL/commit/8950bfa970b7e8a47454a5cdf89e28be638b22fe) that fixed it.

### v1.04
- Fixed crashes/freezes right before the loading screen when entering a new level. This happened after playing for a while and changing levels a few times due to a memory leak from the loading screen thread.
- Fixed crash during the ending cutscene after defeating the final boss.
- Enabled 2x antialias as the impact in performance seems negligible.

### v1.03
- Support for L2/R2 button emulation using rear touch panel
- Disabled logs in VitaGL options

### v1.02
- Fixed Italian language not being set correctly

### v1.01
- Fixes for typo in live area game title

### v1.0
- Initial release.

## Known Issues
- ~~Low framerate during video playing, including main menu due to CPU video decoding.~~ Greatly improved in v1.06.
- ~~Slight framerate spikes every few seconds in some levels due to vorbis audio decoding in main thread.~~ Fixed in v1.06 with threaded Vorbis decoding.
- Missing real-time shadows from characters and enemies. Temporarily disabled for performance.
- Achievements not working yet.
- Some levels with lots of vegetation and water have performance issues. Ex: the Rotting Bog.
- 2 player coop with 2 controllers is implemented, but **only works if both controllers are connected before launching the game**.

## Notes
- If performance drops in some levels or the game slows down, locking to 30FPS can help. This is done in the game settings menu by enabling Battery Saving.
- The game will set the language to your Vita's system language. It has been mostly tested in Spanish and English, but other versions such as Italian, French or German might have issues with audio/texts or even crashes. If you find any problems, feel free to open an Issue.
- For 2-player coop, if you are using a handheld Vita (not Vita TV) you need to install this plugin: https://github.com/TheOfficialFloW/MiniVitaTV
  
## Setup Instructions (For End Users)
In order to properly install the game, you'll have to follow these steps precisely:

- Install [kubridge](https://github.com/TheOfficialFloW/kubridge/releases/) and [FdFix](https://github.com/TheOfficialFloW/FdFix/releases/) by copying `kubridge.skprx` and `fd_fix.skprx` to your taiHEN plugins folder (usually `ux0:tai`) and adding two entries to your `config.txt` under `*KERNEL`:
  
```
  *KERNEL
  ux0:tai/kubridge.skprx
  ux0:tai/fd_fix.skprx
```

**Note** Don't install fd_fix.skprx if you're using rePatch plugin

- **Optional**: Install [PSVshell](https://github.com/Electry/PSVshell/releases) to overclock your device to 500Mhz.
- Install `libshacccg.suprx`, if you don't have it already, by following [this guide](https://samilops2.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx).
- Obtain your copy of *Baldur's Gate: Dark Alliance* legally for Android in form of an `.apk` file. [You can get all the required files directly from your phone](https://stackoverflow.com/questions/11012976/how-do-i-get-the-apk-of-an-installed-app-without-root-access) or by using an apk extractor you can find in the play store.
- Extract these files from the APK to your vita:
  - Copy the contents of **lib** folder to **ux0:/data/bgda/lib**
  - Copy the contents of **assets** folder to **ux0:/data/bgda/assets**
- Open the file **ux0:/data/bgda/assets/world.xvu** in a text editor and do the following:
  - Right under the line that says ```#define lightColor4_extra C(20)``` add this new line:
    - ```#define uvScale C(24)```
  - Near the end of the file, immediately under the line that says ```oT0 = inTexCoord;``` add these two lines:
    - ```oT0.x *= uvScale.x;```
    - ```oT0.y *=  uvScale.y;```
- To verify that you have all the files check the total number of files at the bottom part of the screenshots:
  - Inside ux0:/data/bgda/assets:
    <p align="center"><img src="./screenshots/assets.png"></p>
  - Inside ux0:/data/bgda/assets/res:
    <p align="center"><img src="./screenshots/res.png"></p>

## Build Instructions (For Developers)

This project is built using **VitaSDK (SoftFP)** via Docker.

## Requirements
- Docker/podman
- Git

## Build Instructions

### 1. Create a working directory

Create a directory on your host machine that will be mounted into the container:

```bash
mkdir vita-dev
```

> **Windows example:** `E:\vita-dev`

---

### 2. Pull the VitaSDK Docker image

```bash
docker pull vitasdk/vitasdk-softfp:latest
```

---

### 3. Start the Docker container (first time only)

```bash
docker run -it \
  --name vita-dev-sdk \
  --platform linux/amd64 \
  --volume "E:\vita-dev:/vita-dev" \
  vitasdk/vitasdk-softfp:latest \
  /bin/bash
```

> Adjust the host path (`E:\vita-dev`) to match your setup.

---

### 4. Re-enter the container (subsequent sessions)

After the container has been created, use:

```bash
docker exec -it vita-dev-sdk /bin/bash
```

Do **not** use `docker run` again.

---

### 5. Clone the repository

Inside the container:

```bash
cd /vita-dev
```

If cloning for the first time:

```bash
git clone --recursive https://github.com/Nevak/bgda-vita.git
```

If already cloned:

```bash
git submodule update --init --recursive
```

> This project relies on submodules pinned to specific commits.

---

### 6. Build and run

From the project root:

```bash
extras/scripts/build_and_run.sh
```

---

## Notes
- All build steps must be performed **inside the Docker container**
- The project uses a **SoftFP VitaSDK toolchain**
- Ensure submodules are initialized recursively before building

## Credits
- [TheFlow](https://github.com/TheOfficialFlow) for the original .so loader.
- [Rinnegatamante](https://github.com/Rinnegatamante) for VitaGL, his Android ports, answering all my questions and making changes needed for the game to VitaGL.
- [gl33ntwine](https://github.com/v-atamanenko) for the awesome Android subsystem reimplementation [FalsoNDK](https://github.com/v-atamanenko/FalsoNDK) and [FalsoJNI](https://github.com/v-atamanenko/FalsoJNI).
- [withLogic] for the help testing.

## Special
- Dedicado a mi Abuelo Manolo ❤️

