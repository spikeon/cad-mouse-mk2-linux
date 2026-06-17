# CAD Mouse MK2 — Linux Setup

This directory contains everything needed to get the CAD Mouse MK2 working on Linux, including full 6DoF motion in **OnShape** and physical button mapping.

## How it works

Linux has no official 3Dconnexion driver. The solution is a chain of open-source tools:

```
CAD Mouse MK2
    │  USB HID (VID/PID spoofed as SpaceMouse Compact 256f:c635)
    ▼
spacenavd          — reads 6DoF motion, exposes /var/run/spnav.sock
    │
    ├─► spacenav-ws  — WebSocket bridge to OnShape (port 8181)
    │       ▲
    │   Tampermonkey userscript (fakes navigator.platform = 'Win32')
    │
    └─► (buttons ignored by spacenavd — handled separately)

hidraw device      — raw HID access to button reports
    │
    └─► spnav-buttons — maps buttons via ydotool (Wayland input injection)
```

## Prerequisites

| Package | Notes |
|---------|-------|
| `spacenavd` | Arch: `sudo pacman -S spacenavd` |
| `ydotool` | Arch: `sudo pacman -S ydotool` |
| `uv` | `curl -LsSf https://astral.sh/uv/install.sh \| sh` |
| `python3` | Usually pre-installed |
| Tampermonkey | Browser extension (Chrome/Firefox) |

## Firmware

Before running the installer, flash the firmware with VID/PID spoofing enabled. `spacenavd` only recognises known 3Dconnexion VID/PIDs — spoofing as a SpaceMouse Compact (`256f:c635`) is the simplest way to make it work without custom configuration.

Edit `platformio.ini` and uncomment/set these two lines:

```ini
board_build.arduino.earlephilhower.usb_vid = 0x256F
board_build.arduino.earlephilhower.usb_pid = 0xC635
```

To flash:
1. Hold **B**, tap **R** on the XIAO RP2040 to enter BOOTSEL mode (or hold B while plugging in)
2. Build and copy the firmware: `pio run && sudo mount /dev/sdX1 /mnt && sudo cp .pio/build/seeed_xiao_rp2040/firmware.uf2 /mnt/ && sudo umount /mnt`

## Installation

```bash
cd linux
chmod +x install.sh
./install.sh
```

The installer:
- Adds your user to the `input` group (needed for hidraw button access)
- Installs `spnav-buttons` to `~/.local/bin/`
- Installs and enables three systemd user services: `ydotoold`, `spacenav-ws`, `spnav-buttons`
- Writes `/etc/spnavrc` (sensitivity config)
- Installs udev rules so services restart automatically on plug/unplug
- Patches `spacenav-ws` to disable its built-in button-snap behaviour

After the installer finishes, install the Tampermonkey userscript:

1. Install [Tampermonkey](https://www.tampermonkey.net/) in your browser
2. Drag `linux/onshape-spacenav.user.js` onto the Tampermonkey dashboard
3. Open [OnShape](https://cad.onshape.com) and open any document — motion should work immediately

> **Note:** If you were just added to the `input` group, log out and back in (or reboot) before the buttons will work.

## Button mapping

| Action | Result |
|--------|--------|
| Left button (hold) | Scroll up |
| Right button (hold) | Scroll down |
| Both buttons together | Shift+7 (OnShape: fit to window) |

To change button behaviour, edit `~/.local/bin/spnav-buttons` and restart the service:

```bash
systemctl --user restart spnav-buttons
```

## Sensitivity tuning

Edit `/etc/spnavrc`:

```
sensitivity-translation = 0.0054   # lower = less sensitive
sensitivity-rotation = 0.048
dead-zone = 40                      # raise if the view drifts when mouse is untouched
```

Then apply:

```bash
sudo systemctl restart spacenavd
```

## Troubleshooting

**Motion not working in OnShape**
- Check the Tampermonkey userscript is active on `cad.onshape.com`
- Refresh the OnShape tab after any service restart
- Verify spacenavd sees the device: `sudo journalctl -u spacenavd -n 30`
- Check spacenav-ws is running: `systemctl --user status spacenav-ws`

**Buttons not working**
- Ensure you're in the `input` group: `groups | grep input` (reboot if you just added yourself)
- Check the spnav-buttons service: `systemctl --user status spnav-buttons`
- Verify the hidraw device exists: `ls /dev/input/by-id/ | grep CAD`

**Device not recognised after plugging in**
```bash
# Manually restart services
sudo systemctl restart spacenavd
systemctl --user restart spacenav-ws spnav-buttons
```

**Services hitting start-limit**
```bash
systemctl --user reset-failed spacenav-ws spnav-buttons
systemctl --user restart spacenav-ws spnav-buttons
```

**View snaps to front when buttons are pressed**

The `spacenav-ws` patch wasn't applied. Re-run:
```bash
python3 linux/patch-spacenav-ws.py
systemctl --user restart spacenav-ws
```

**spacenav-ws patch lost after update**

`uvx` caches packages by version. If `spacenav-ws@latest` updates, the patch needs to be re-applied. Re-run:
```bash
python3 linux/patch-spacenav-ws.py
systemctl --user restart spacenav-ws
```
