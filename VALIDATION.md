# Validation (v2.1)

Release ROM `ArchersTabletop-v2.1.gbc`: 262,144 bytes, SHA-256 `8719f0607f004e61d28aa1d70e90362328fe691663a6c45179d7e818fe48323a`.

## Difficulty ladder (v2.1, `tests/difficulty_sim.c`, 400 games per rival, colors alternating)

Player win rate. "Newcomer" builds lines and often misses threats; "decent" blocks fours, usually blocks open threes and spots open fours; "strong" is the SOREN engine itself.

| Rival | Newcomer | Decent | Strong |
|---|---|---|---|
| PIP 600 | 62% | 96% | 94% |
| RUSH 780 | 48% | 95% | 93% |
| MARA 900 | 40% | 91% | 89% |
| JUNE 1020 | 40% | 90% | 73% |
| VEX 1120 | 33% | 84% | 73% |
| ORIN 1240 | 36% | 80% | 69% |
| NOVA 1380 | 31% | 68% | 63% |
| ROOK 1500 | 23% | 62% | 57% |
| IRIS 1640 | 19% | 57% | 56% |
| SOREN 1800 | 11% | 40% | 50% |

Before v2.1 every rival measured the same (newcomer 11-17%, decent 40-58%, strong 45-59% against all ten). SOREN's settings are unchanged. On the release ROM the longest AI turn measured 145 frames (about 2.4 s).

## Build
- Official GB Studio 4.3.2 CLI build (GBDK 4.5.0), GBC-only.
- Header identical in configuration to the v1 cartridge: CGB flag 0xC0, cartridge type 0x1E (MBC5 + RAM + battery), ROM 256 KB, RAM 32 KB. Header and global checksums valid.
- Memory: ROM0 609 bytes free; WRAM 881 bytes free (unused GB Studio projectile, trigger and script-text buffers were shrunk via header overrides).
- Save layout unchanged from v1, so existing ratings, rival history and the 2048 board load as-is.

## Desktop tests (`sh tests/run.sh`)
All four suites pass: 2048 rules/save (20,096 checks), Gomoku core (110,271), AI (1,815; strongest vs beginner 20/0), rivals/saves (8,731, including legacy-save migration).

## Emulator checks (PyBoy 2.7, CGB mode, exact release ROM)
- Title, main carousel (all three pages, both directions), Gomoku mode carousel, rival select (10 rivals), Settings (toggles, erase confirmation), ranked roulette landing on the matched rival.
- 2048: slides, merges with sparkles and score popups, undo, pause menu (resume/new game/menu), "YOU MADE 2048!" popup with confetti, game-over popup with undo, 2,500-move random soak with no hang.
- Gomoku: two-player five-in-a-row with the win-line sparkle chain, result card, Select to hide it; ranked game with Elo change; five practice games against rivals PIP, MARA, VEX, NOVA and IRIS played to completion with correct winners.
- Steady-state main loop runs at one iteration per frame; fades take 8 frames each way.
- Audio rendered to WAV: music is present on every screen; title lead melody measured at E5-G5-C6-B5-G5-E5 over a C bass, matching the score.

## Not verified here
Real-hardware appearance, audio balance on the Chromatic speaker, and power-cycle save retention need a check on the device after flashing.
