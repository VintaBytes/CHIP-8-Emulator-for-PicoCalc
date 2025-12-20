
## CHIP-8 Emulator for PicoCalc (RP2040)

I’ve been working on a **native CHIP-8 emulator for the Clockwork PicoCalc**, written in C and running directly on the **PicoCalc hardware (RP2040)** 

<center><img src="https://github.com/VintaBytes/CHIP-8-Emulator-for-PicoCalc/blob/main/screenshots/1.jpeg"> </center>




The project started as an exploration of the PicoCalc SDK and gradually evolved into a fully playable emulator capable of running classic CHIP-8 programs such as **PONG**, **Tetris**, and similar early titles.

This work is **built on top of Blair’s PicoCalc SDK**, which provided the foundation for graphics, input, storage, and overall system integration.



## What the emulator currently does

At its current stage, the emulator:

* Implements the **full standard CHIP-8 instruction set**
* Runs on a real **RP2040-based PicoCalc**
* Uses the PicoCalc LCD with:

  * Centered 64×32 CHIP-8 display
  * Scaled pixels for good readability
  * Incremental (diff-based) rendering for decent performance
* Supports:

  * Delay and sound timers at 60 Hz
  * Keyboard input mapped to the CHIP-8 hex keypad
  * A ROM selection menu that scans `.CH8` files from the `/roms` directory
* Allows:

  * Selecting a ROM from a menu at startup
  * Exiting a running game with `ESC` and returning to the ROM menu

Overall, classic CHIP-8 games are **fully playable** and responsive.



## Current state of the project

* The emulator is **functional and stable**
* Performance is good enough for real gameplay on the PicoCalc
* Some visual details (minor flicker in certain ROMs, menu polish) are still being refined
* The codebase is intentionally kept simple and readable, with future expansion in mind

This is very much a **working, usable emulator**, not just a proof of concept — but there is still room to grow.



## Planned future work

Once development resumes, the next steps include:

* **Super-CHIP (SCHIP) support**

  * 128×64 resolution
  * Extended instructions
* Better timing control and fine-tuning of CPU speed
* Small UI improvements in the ROM menu
* Optional emulator “quirks” configuration for broader ROM compatibility
* Code cleanup and documentation



## Development pause & GitHub release

I’m about to enter a **vacation period**, so development will slow down significantly during that time.

Once I’m back, I plan to:

* Clean up the code
* Add documentation
* Publish the **full source code on GitHub**

At that point, others will be able to build, study, and extend the emulator easily.

---

## Summary

* ✔ Native CHIP-8 emulator for PicoCalc (RP2040)
* ✔ Built using Blair’s PicoCalc SDK
* ✔ Runs on unmodified, stock hardware
* ✔ Playable classic games
* 🚧 More features planned (Super-CHIP, refinements)

If you own a PicoCalc and enjoy retro systems, this project is a fun way to push the device a bit further.

More to come soon 
