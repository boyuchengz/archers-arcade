# Validation (v2 "Game Night")

Release ROM `ArchersTabletop-v2.gbc`: 262,144 bytes, SHA-256 `940e4aaff8927a123a9a0b10f161bb042ba3aeb902c876e8b591e0b75fbd0516`.

## Build
- Official GB Studio 4.3.2 CLI build (GBDK 4.5.0), GBC-only.
- Header identical in configuration to the v1 cartridge: CGB flag 0xC0, cartridge type 0x1E (MBC5 + RAM + battery), ROM 256 KB, RAM 32 KB. Header and global checksums valid.
- Memory: ROM0 609 bytes free; WRAM 881 bytes free (unused GB Studio projectile, trigger and script-text buffers were shrunk via header overrides).
- Save layout unchanged from v1, so existing ratings, rival history and the 2048 board load as-is.

## Desktop tests (`sh tests/run.sh`)
All four suites pass: 2048 rules/save (20,096 checks), Gomoku core (110,271), AI (1,876; strongest vs beginner 20/0), rivals/saves (8,731, including legacy-save migration).

## Emulator checks (PyBoy 2.7, CGB mode, exact release ROM)
- Title, main carousel (all three pages, both directions), Gomoku mode carousel, rival select (10 rivals), Settings (toggles, erase confirmation), ranked roulette landing on the matched rival.
- 2048: slides, merges with sparkles and score popups, undo, pause menu (resume/new game/menu), "YOU MADE 2048!" popup with confetti, game-over popup with undo, 2,500-move random soak with no hang.
- Gomoku: two-player five-in-a-row with the win-line sparkle chain, result card, Select to hide it; ranked game with Elo change; five practice games against rivals PIP, MARA, VEX, NOVA and IRIS played to completion with correct winners.
- Steady-state main loop runs at one iteration per frame; fades take 8 frames each way.
- Audio rendered to WAV: music is present on every screen; title lead melody measured at E5-G5-C6-B5-G5-E5 over a C bass, matching the score.

## Not verified here
Real-hardware appearance, audio balance on the Chromatic speaker, and power-cycle save retention need a check on the device after flashing.
