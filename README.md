# CAD Mouse MK2 — Linux Fork

This is a fork of [sb-ocr/cad-mouse-mk2](https://github.com/sb-ocr/cad-mouse-mk2) with changes to make the device work on Linux with OnShape.

## What's different

**Firmware**
- Sleep timeout disabled

**`linux/` directory** — full Linux integration, not in the original repo:
- One-command installer (`install.sh`)
- `spacenavd` + `spacenav-ws` WebSocket bridge for OnShape
- Tampermonkey userscript so OnShape connects to the local bridge
- Button mapper (`spnav-buttons`): scroll up/down while held, both buttons = Shift+7
- udev rules with delayed restarts to avoid race conditions on plug-in
- `spacemouse-status` health check script
- Patches to `spacenav-ws` that disable the button-snap-to-front-view behaviour and add automatic reconnection to `spacenavd` (no OnShape tab refresh needed after replug)

## Linux setup

See [linux/README.md](linux/README.md).

## Original project

Hardware design, build instructions, BOM, and enclosure files are from the original project:

**[sb-ocr/cad-mouse-mk2](https://github.com/sb-ocr/cad-mouse-mk2)**

Build instructions → [Instructables](https://www.instructables.com/CAD-Mouse-MK2-a-6DoF-Space-Mouse-Using-Magnets)

[![CC BY-NC-SA 4.0][cc-by-nc-sa-shield]][cc-by-nc-sa]

[cc-by-nc-sa]: http://creativecommons.org/licenses/by-nc-sa/4.0/
[cc-by-nc-sa-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg
