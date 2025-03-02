# Balrdur's Gate: Dark Alliance Vita

This is a wrapper/port of <b>Balrdur's Gate: Dark Alliance</b> for the *PS Vita*.

The port works by loading the official Android ARMv7 executables in memory, resolving its imports with native functions and patching it in order to properly run.
By doing so, it's basically as if we emulate a minimalist Android environment in which we run natively the executable as is.

# Changelog

WIP early and broken version. Current issues: 
- Slow video and audio playback during intro / transitions
- The UI is missing from the main menu and the entire game after that is full blackscreen. However, by blindingly touch the buttons (using a working PS2 version as reference) you can navigate and start a new game. Still will show just a black screen, but proves that the game is able to load up to the part where you can control the character.
- Many issues more that will arise once some rendering is working