# DualityDS

**A Nintendo DS emulator designed around how it feels to use.**

Your games live on a Wii-style cover carousel instead of a file list — browse with a controller, pick a game, and play in a clean dual-screen window. DualityDS takes the powerful [melonDS](https://github.com/melonDS-emu/melonDS) emulation core and wraps it in a simplified, console-style interface built for casual players who just want to sit down and play.

No config files. No menus to learn. Plug in a controller and go.

---

## Download

Grab the latest release from the [Releases page](https://github.com/wesleywtchang-jpg/DualityDS/releases).

1. Download `DualityDS-v0.1.1-win64.zip`
2. Unzip it anywhere
3. Open the `DualityDS` folder and run `DualityDS.exe`
4. On first launch the library is empty — click **Add Folder** to point it at your games

DualityDS is self-contained for Windows 64-bit; Qt6 and the runtime are bundled, so there's nothing to install.

> You bring your own NDS game files. DualityDS does not include any games or copyrighted ROMs.

## Powered by melonDS

DualityDS runs on the melonDS core, one of the most accurate DS emulators available, so the full emulation feature set is here:

- Local and online multiplayer (LAN and Netplay)
- Cheat codes and save states
- Screen layout and placement editing
- Dual-screen rendering and full system configuration

## What DualityDS adds

A simplified, console-style interface layered on top, aimed at casual players:

- **Wii-style cover carousel** with online box art — your library, not a folder
- **Full controller navigation** — browse, select, and play without the keyboard
- **Right-stick velocity cursor** and **R2 touchscreen mapping** for the bottom screen
- **Single-window dual-screen layout** and a clean console-style theme

## Building from source

See [BUILD.md](BUILD.md) for build instructions (Qt6, C++).

## License

DualityDS is free software licensed under **GPLv3**, inherited from melonDS. All credit for the underlying DS emulation goes to the melonDS team (Arisotura and contributors). The full melonDS project, documentation, and credits live at [melonDS-emu/melonDS](https://github.com/melonDS-emu/melonDS).

See [LICENSE](LICENSE) for the full license text.
