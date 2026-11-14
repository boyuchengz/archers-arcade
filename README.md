# Archer's Tabletop (v2 "Game Night")

A Game Boy Color cartridge with Gomoku and 2048, built for the ModRetro Chromatic. Author: Archer.

## What's new in v2

- **Readable type everywhere**: a new bold 2-pixel-stem font with drop shadows and lowercase, no cryptic abbreviations.
- **Animated title screen** with a ribbon logo, drifting star pattern, bobbing stones and sparkles.
- **Swipeable card menus** (smooth hardware-scrolled carousel) with illustrations for every mode.
- **Ten redrawn rivals** with personalities and four expressions each (neutral, happy, shocked, sad). They react to your play, type their lines out letter by letter with their own voice pitch.
- **Ranked matchmaking roulette**, rank titles (Rookie to Master), and an Elo change readout after each game.
- **Gomoku table**: wooden board, glossy stones that drop in with a clack, pulsing cursor, last-move marker, and a sparkle chain plus confetti over the winning five.
- **2048**: colorful keycap tiles (2 through 16384), rolling score counter, "+points" popups, merge sparkles, screen shake on big merges, new-best and 2048 celebrations, pause menu.
- **Original soundtrack**: title theme, a 2048 groove, a calm pentatonic Gomoku theme, plus win, lose, 2048 and new-best jingles. Sound effects for every action.
- Smooth fades between every screen.

Saves from v1 load unchanged (same save layout). Music now defaults to on; turn it off in Settings.

## v2.1: a real difficulty ladder

In earlier builds all ten rivals played at the same strength. Now each rival has its own skill profile in `tt_ai.c` (`skills` table): how often it misses its own five, overlooks your four or makes a careless attacking move, how deep it reads four-chains (VCF), whether it sees yours, and how far it looks ahead. PIP is genuinely beatable; SOREN plays exactly as strong as before. Practice always gives you black (the first move).

`tests/difficulty_sim.c` plays simulated players against every rival to check the ladder:

    cc -O2 -w -Iplugins/tabletop/engine/include -Iplugins/tabletop/engine/src/states tests/difficulty_sim.c -o /tmp/sim && /tmp/sim 400

## Controls

Menus: Left/Right pick, A open, B back.

Gomoku: D-pad moves the cursor, A places a stone, B or Start opens the game menu (Resume, Undo in Practice, Resign). After a game, A plays again, B returns to the menu, Select hides the result card so you can look at the board.

2048: D-pad slides, B undoes the last move, Start pauses (Resume, New game, Main menu), Select returns to the main menu.

## Build and edit

Open `project.gbsproj` in GB Studio 4.3.2, or build headlessly:

    node <gb-studio>/out/cli/gb-studio-cli.js make:rom project.gbsproj ArchersTabletop.gbc

All gameplay and presentation is C in `plugins/tabletop/engine` (the TABLETOP scene type). Art, music and dialogue are generated into headers by Python scripts in `tools` (they need Pillow):

    python3 tools/make_art.py        # font, UI kit, sprites, logo, illustrations, 2048 cards, board
    python3 tools/make_portraits.py  # rivals (writes tools/preview/portraits.png for review)
    python3 tools/make_music.py      # soundtrack and jingles
    python3 tools/make_openings.py   # rival opening book
    python3 tools/make_dialogue.py   # rival dialogue

`tools/preview` holds rendered previews of the generated art. Run `sh tests/run.sh` (needs a C compiler) for the desktop rules, AI, rival and save checks.

The project-local `vm.h`, `actor.h`, `projectiles.h`, `trigger.h` and `ui.h` derive from GB Studio 4.3.2 (MIT, see LICENSE). They shrink engine buffers this dedicated scene never uses, which frees RAM for the new presentation layer.
