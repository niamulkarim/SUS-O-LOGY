# SUS-O-LOGY

**Stay alive, or fear it.**

A social deduction game built from scratch in C — starting as a terminal simulation and growing into a full graphical desktop game with raylib. This description is as much a devlog of that process as it is a game pitch.

## What it is

Players are secretly assigned one of three roles — **Villager**, **Ghost**, or **Wizard** — plus a fourth, **Zombie**, that only the human player can become. Villagers vote each day to find and eliminate the Ghost. The Ghost secretly turns people into Zombies each night. The Wizard secretly protects someone from the Ghost's attack. And if the Ghost gets you, you don't just spectate — you come back as a Zombie allied with the Ghost, with your own vote and your own night action, fighting to survive until the village catches up to you.

## The journey

This didn't start as a GUI game — it started as a plain terminal program (`core.c` / `game.c` / `game.h`) to nail the actual rules before touching any graphics. That early version surfaced real bugs worth fixing properly:

- A silent **stack buffer overflow risk** from unvalidated player-count input
- A genuine **game-logic deadlock**: with only the Ghost and one Villager left alive, day votes could lock into a permanent 1-vs-1 tie forever — fixed by treating that parity as an automatic Ghost win
- A full **redesign of the Wizard** from a "revive the dead" ability into a "protect before the kill lands" mechanic, and a redefinition of what "Zombie" even means (alive-but-mute vs. properly dead)

Once the rules were solid, the project moved to **raylib** for real graphics — learned from zero, one small program at a time: a blank window, then movement and clickable buttons, then screen-to-screen navigation, before ever touching the real game. Along the way:

- Set up a full **C build toolchain from scratch** (w64devkit, compiling raylib itself from source, PowerShell quirks like the `.\` execution prefix)
- Hit and fixed the classic **Windows path escaping bug** (`\` vs `/` in string literals) more than once
- Discovered raylib **doesn't support JPG by default** — PNG-only unless you rebuild the library with that flag enabled
- Grew the game through versioned milestones (v2 → v3 → v4), each one a real feature push: a full 9-screen state machine, per-player name entry, a select-then-confirm voting flow, a 3-way art-branching Role Reveal screen, an intro animation sequence, and the standout addition — a **human-exclusive Zombie survival mechanic** that changes how the endgame plays out specifically for the player, not the bots

## Problems solved along the way

- **Portability bug**: backgrounds worked locally but showed up blank for a friend testing the build — traced to hardcoded absolute file paths (`C:/Game/VERSION_4/...`) instead of relative ones. Fixed, then verified by testing from a fresh folder before shipping again.
- **Distribution**: confirmed via direct DLL inspection that the compiled `.exe` has zero MinGW-specific runtime dependencies — it runs on a bare Windows 10/11 machine with nothing extra installed.
- **Email delivery**: Gmail silently blocks zipped `.exe` files — worked around via cloud links instead.
- **Startup delay**: traced to 150 intro-animation frames being loaded synchronously and fully before the window even appears — solved short-term with a "please wait" loading screen, with a real fix (progressive/threaded loading, fewer/smaller frames) scoped for later.
- **Repo cleanup**: inherited a messy first GitHub push (duplicate/stale files from manual web uploads) and properly reset it via `git rm` + a clean commit, plus a `.gitignore` to keep build artifacts (`.exe`, `.o`, the raylib library) out of version control going forward.
- **Scoped and deliberately declined**: real LAN/WiFi multiplayer was seriously evaluated (architecture, effort, what it would take to hide secret roles across a network) and consciously shelved as a future-maybe rather than rushed — a good example of scoping a feature honestly instead of overcommitting.

## Tech stack

- **Language**: C
- **Graphics**: raylib
- **Build**: GCC via w64devkit, `mingw32-make`
- **Platform**: Windows (built and tested on Windows 10/11)

## Building from source

```text
mingw32-make
.\test_version_4.exe
