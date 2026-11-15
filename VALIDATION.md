# Validation (v3.1)

Release ROM `ArchersArcade-v3.1.gbc`: 262,144 bytes, SHA-256 `904ede2a717404f4dd7e57cffa94eb907f2fbaf4f6cf4902b80fbccd65a27e0e`.

v3.1 adds coin rewards (a heart every 50 coins, +10 at full health) and eight hood colours unlocked by the lifetime coin bank. Checked in the emulator: the 50th coin restores a heart, the 100th at full health adds 10, a game over banks the run's coins and announces a newly unlocked hood, the start-screen picker cycles only unlocked hoods and the choice is saved; Prism cycles colours.

## Build
- Official GB Studio 4.3.2 CLI build (GBDK 4.5.0), GBC-only.
- Header: title `ARCHERSARCADE`, CGB flag 0xC0, cartridge type 0x1E (MBC5 + RAM + battery), ROM 256 KB, RAM 32 KB, same as earlier cartridges. Header and global checksums valid.
- ROM: about 20 KB still free (banks 13 to 15).
- RAM: 996 bytes free in WRAM high. The C stack has 740 bytes between the heap and its top; the deepest use measured in the emulator was 231 bytes (during Gomoku's strongest AI), 211 bytes in the tower and menus. The tower record (best floor, coin bank, checkpoint, hood) is appended to the save.
- Save: v1 and v2 saves load unchanged (accepted sizes 130, 180 and 186 bytes); the tower record is appended and starts at zero.

## Desktop tests
- `sh tests/run.sh`: all four suites pass: 2048 rules/save (20,096 checks), Gomoku core (110,271), AI (1,815; strongest vs beginner 20/0), rivals/saves (8,731, including legacy-save migration).
- Tower rooms: every one of the 39 templates passes the reachability checker for each entrance it accepts (`python3 tools/tower_rooms.py`).
- Tower room builder (`tests/test_tower_gen.c` + `tools/check_tower_rooms.py`): 50,000+ generated floors across eight seeds (about half of them new procedural layouts), 0 unclimbable rooms, 0 unsafe entrances.

## Emulator checks (PyBoy 2.7, CGB mode, exact release ROM)
- Title, four-card main menu, tower start panel, pause panel (Resume, Restart run, Main menu), game-over and new-record panels, retry from checkpoint, quit to menu; 2048 and Gomoku still start and play from their new menu positions.
- Physics: full jump 30 px, tapped jump 11 px, run speed about 1.3 px per frame, spring launch 72 px, walls and ceilings block correctly.
- Mechanics: arrows stick in wood and can be stood on; arrows break on stone; stomping a slime pops it (+1 coin) and bounces; side contact costs a heart with knockback; spikes cost a heart and respawn you at the entrance; crumbling blocks fall after standing on them and come back; moving platforms carry the hero; cannons fire; icicles fall when you pass under them; the campfire refills hearts and saves the checkpoint; checkpoint floors can be picked on the start screen.
- Camera pans between floors take 30 to 35 frames, including pans into a new zone (colours cross-fade over three-frame steps). The main loop drops at most one frame on a normal floor change and none during normal play.
- Climbed to floor 200 with no failed transitions; 42,000 frames of random input (mashing, deaths, retries, pauses) with no hang.
- Audio: tower music and effects present.

## Not verified here
Real-hardware feel of the controls, audio balance on the Chromatic speaker and power-cycle save retention need a check on the device after flashing.
