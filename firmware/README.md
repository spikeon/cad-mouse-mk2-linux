## LED configuration

The LED ring color and brightness can be configured without reflashing. Changes are stored in flash and persist across power cycles.

### Runtime configuration (USB serial)

Connect a serial terminal at 115200 baud and send commands:

```
led show               — print current brightness and color
led brightness <0-255> — set brightness
led color <RRGGBB>     — set idle color (hex, e.g. led color FF8800)
led reset              — restore firmware defaults
```

### On-device configuration (button hold)

With the device connected, hold both buttons for 6 seconds. The device will go through its normal 3-second calibration, then enter color config mode.

**Step 1 — Brightness:** The ring breathes at the current color. Twist the knob to adjust brightness. Right button to confirm, left button to cancel.

**Step 2 — Color:** The ring breathes through the selected color. Twist the knob to cycle through the color wheel (red → yellow → green → cyan → blue → magenta → red). Right button to save, left button to cancel.

The color wheel follows standard RGB hue angles: red at 0°, yellow at 60°, green at 120°, cyan at 180°, blue at 240°, magenta at 300°.

---

You can customize several variables to tune gains, smoothing, and deadzones for all six axes.
Most of these settings are defined in [`Config.h`](include/Config.h) and are the main place to adjust the overall feel of the device.

```cpp
// Gains and sign fixes
const float GAIN_T[3] = {28.0, 28.0, 24.0};
const float GAIN_R[3] = {18.0, 18.0, 20.0};
const int SIGN_AXIS[6] = {-1, +1, -1, +1, +1, +1};

// Dead zones
const float DEAD_T = 16.0;
const float DEAD_R = 20.0;

// Smoothing
const float SMOOTH_TAU_S = 0.08;
```

⚠️ Refer to the video at [6:23](https://youtu.be/62xlzGs8LXA?si=ld2shDCaTxOLIGB8&t=383) for a demo of driver support. Related settings can be found commented in[`platformio.ini`](../platformio.ini).

⚠️ As mentioned in the video, the motion processing still needs work and may eventually be replaced entirely. This is beyond me for now, so contributions and improvements are welcome.

There is bleed between the axis due to the way each of them is calculated independently. This implementation also assumes the readings from the sensors are linear, which is not true.

If you want to experiment with different motion processing approaches, you can modify the [`MotionController`](src/controllers/MotionController.cpp). The current implementation is as follows:

**Sensor layout:**
- `mag1` = bottom
- `mag2` = top left
- `mag3` = top right

```
Tx = (mag1x + mag2x + mag3x) / 3
Ty = (mag1y + mag2y + mag3y) / 3
Tz = (mag1z + mag2z + mag3z) / 3

Rx = sqrt(3) * (mag2z + mag3z - 2 * mag1z) / 3
Ry = mag3z - mag2z
Rz = sum_i (posXi * magYi - posYi * magXi)
```
- `Tx`, `Ty`, `Tz`: average of all three sensors
- `Ry`: left-right z difference across the top edge
- `Rx`: top pair versus bottom, scaled for the triangle geometry
- `Rz`: twist estimate from the x/y sensor positions
