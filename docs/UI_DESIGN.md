# UI visual reference implementation

The five user-provided mockups are the visual reference for Devices, Weight,
Magic Modes, Eccentric Overload and Rest Pause. The artwork is implemented as
native LVGL widgets and vector lines, so values, limits, arcs, labels and hit
targets remain interactive and scale to 360, 466 and 480 px round displays.

- Near-black face, subtle circular edge and shaded panels.
- Cyan device tiles with interlocking link glyphs, names and Online/Offline badges.
  Four tiles per touch page; remembered/offline behavior remains controller-driven.
- Forty-one outer markers over 240 degrees. Cyan markers show the selected
  value; EO uses pink. Zero, midpoint and maximum labels follow the live range.
- Large central round weight/modifier control with a cyan or pink border and
  restrained glow. Only confirmed loaded weight is bright; unloaded weight is dim.
- Small circular Magic, EO and Dropset controls retain the established tap/hold
  gestures and actual values. Magic editor order is IC, MTN, C as in the mockup.
- RP uses three central dials. All values and arcs are active simultaneously.
  The focused dial has a cyan outline/glow, drives the outer scale and names the
  parameter below. Tap changes rotary focus; it never disables another field.
- Apply retains the existing apply-and-return behavior.
- Remote battery appears at the bottom. An optional hardware battery reading
  avoids confusing remote battery with the connected VOLTRA's battery.

The desktop's 75% battery is explicit synthetic data and is labeled SIM. The
physical casing/photographic texture is not a screen asset. No rasterized
numbers, fabricated limits or device settings are taken from the artwork.

Native interaction capture remains unavailable on this Windows host; layout
verification uses the simulator's actual offscreen LVGL framebuffer renders.
