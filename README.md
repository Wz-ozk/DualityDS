# DualityDS

**A Nintendo DS emulator designed around how it feels to use.**

Your games live on a Wii-style cover carousel instead of a file list — browse with a controller, pick a game, and play in a clean dual-screen window. DualityDS takes the powerful [melonDS](https://github.com/melonDS-emu/melonDS) emulation core and wraps it in a simplified, console-style interface built for casual players who just want to sit down and play.

No config files. No menus to learn. Plug in a controller and go.

<img width="1034" height="757" alt="Screenshot 2026-06-29 230422" src="https://github.com/user-attachments/assets/3df97ecd-86e8-4e17-8901-f07c95f04f22" />

*DualityDS library view — games not included; bring your own ROM files.*

---

## Download

Grab the latest release from the [Releases page](https://github.com/Wz-ozk/DualityDS/releases).

1. Download `DualityDS-v0.1.1-win64.zip`
2. Unzip it anywhere
3. Open the `DualityDS` folder and run `DualityDS.exe`
4. On first launch the library is empty — click **Add Game…** and select your NDS game files

DualityDS is self-contained for Windows 64-bit; Qt6 and the runtime are bundled, so there's nothing to install.

> Please bring your own NDS game files. DualityDS does not include any games or copyrighted ROMs.

## Powered by melonDS

DualityDS runs on the melonDS core, one of the most accurate DS emulators available, so the full emulation feature set is here:

- Local and online multiplayer (LAN and Netplay)
- Cheat codes and save states
- Screen layout and placement editing
- Rendering and full system configuration

## What DualityDS adds

A simplified, console-style interface layered on top, aimed at casual players:

- **Wii-style cover carousel** with online box art — your library, not a folder
- **Full controller navigation** — browse, select, and play without the keyboard
- **Right-stick velocity cursor** and **R2 touchscreen mapping** for the bottom screen
- **Single-window dual-screen layout** and a clean console-style theme


<img width="842" height="803" alt="Screenshot 2026-06-29 231332" src="https://github.com/user-attachments/assets/faa21d4f-c53e-434d-b939-bca0fd28e3dd" />

*Classic stacked layout — both DS screens, just like the handheld.*

<img width="2036" height="929" alt="Screenshot 2026-06-29 231520" src="https://github.com/user-attachments/assets/40c73e13-19ed-4217-9c54-7e3510d1dad1" />

*Dual-screen window mode — spread both screens however you like.*


## Building from source

See [BUILD.md](BUILD.md) for build instructions (Qt6, C++).

## License

DualityDS is free software licensed under **GPLv3**, inherited from melonDS. All credit for the underlying DS emulation goes to the melonDS team (Arisotura and contributors). The full melonDS project, documentation, and credits live at [melonDS-emu/melonDS](https://github.com/melonDS-emu/melonDS).

See [LICENSE](LICENSE) for the full license text.
