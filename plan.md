# DualityDS — Plan

## Done
- [x] v0.1.1 public release — portable zip (DualityDS-v0.1.1-win64.zip), 88 MB, full audio
- [x] Clean-machine verified: launch, audio, saves
- [x] Source committed + tagged (v0.1.1 → bbb4ad97), GPL-3 compliant
- [x] README rewritten — casual voice, melonDS credit, carousel + gameplay screenshots
- [x] Repo public, About sidebar updated

## In progress — v0.1.2 installer
- [ ] Build DualityDS-Setup-v0.1.2.exe via dist/ Inno Setup script
- [ ] Verify installer bundles the FULL v0.1.1 file set (117 DLLs, multimedia plugin, ffmpeg) — NOT the old lean build
- [ ] Config: Start-menu shortcut, desktop icon, uninstaller; no .nds association
- [ ] Clean-machine test: install → shortcut launches → audio plays → desktop icon → UNINSTALL fully removes
- [ ] Publish v0.1.2 release with BOTH installer + portable zip (new tag, fresh SHA256)

## Backlog / later
- [ ] Confirm cheats + multiplayer actually work in-build before heavily advertising them
- [ ] Consider CI to auto-build Windows binaries on tag

## Release rules (don't break these)
- New tag + version bump every release; never reuse a tag
- Fresh SHA256 per build; binary must build from tagged source (GPL-3)
- Keep prior working release intact until the new one is verified on a clean machine
- I publish/delete releases via GitHub web UI myself; Claude Code stops at gates for my OK
