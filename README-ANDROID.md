# PokéWilds Android / AYN Thor prototype

This port reuses the PokéWilds 0.8.11 game logic and assets, strips desktop-only LWJGL/Swing updater code, and runs the game through LibGDX's Android backend.

Features:
- Native Android LibGDX launcher.
- Existing PokéWilds Android touch overlay is enabled automatically by the original game code.
- Android gamepad backend for built-in/USB/Bluetooth controllers.
- AYN Thor second-display Presentation: game on the upper display, control deck + mapping menu on the lower display.
- Lower touch deck: D-pad, A/B, L/R, Start.
- Physical A/B/X/Y remapping and analog deadzone on the lower screen.
- Runtime second-display detection; no hardcoded Thor display ID.

Build input is PokéWilds v0.8.11. The GitHub Actions workflow downloads the official pokewilds-windows64.zip, extracts pokewilds.jar, creates an Android-safe core JAR, extracts the game assets into app/src/main/assets, and builds a debug APK.

The first APK is a prototype and still needs real-device testing for save-path edge cases, controller quirks, and lower-display behavior across Thor firmware revisions.
