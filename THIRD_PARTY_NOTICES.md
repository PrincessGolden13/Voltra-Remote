# Third-party dependencies and research

LVGL 9.3.0 is a build dependency under the MIT license. Its complete source and
LICENSE.txt remain in the downloaded dependency, outside tracked product source.
Preserve the applicable LVGL notice with distributed binaries.

Research only: RyanMarkoff-eaton/voltra-diy-remote and HJewkes/voltra-node-sdk
publish MIT notices. Their source/binaries are not copied into this product's
tracked source. Protocol constants are documented with provenance; this project
uses its own small bitwise framing implementation and its own synthetic mocks.

The Home Assistant and Android projects are external research sources. Their
application source, parameter database, captures and official-app artifacts are
not vendored or redistributed. Review actual licenses before any future copying.

The physical targets now use Arduino-ESP32 2.0.17 (LGPL-2.1), its pinned ESP-IDF
4.4 SDK (Apache-2.0 and component-specific notices), NimBLE-Arduino 2.3.6
(Apache-2.0, including its bundled NimBLE notices), and LVGL 9.3.0 (MIT).
PlatformIO espressif32 6.12.0 resolves their exact versions. These libraries are
build dependencies, not copied application repositories. Arduino GFX belongs only
to the external starter reference and is not linked by this project.

PanelInit.h adapts register data from Andrew Malitchuk's prompt-knob-firmware,
commit b738e065b7dec7a13501cdd81c8d3b511850bce4, under Apache-2.0. The file describes
the changes and docs/GUITION_K5.md records the source. Its license is retained in
third_party/licenses along with the runtime library license texts/notices.

Firmware releases include these notices and an archive of application/library
objects and ELF files for relinking. The complete application source, dependency
pins and build instructions are public. Dependency sources are available from
[Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/tree/2.0.17),
[ESP-IDF 4.4.7](https://github.com/espressif/esp-idf/tree/v4.4.7),
[NimBLE-Arduino 2.3.6](https://github.com/h2zero/NimBLE-Arduino/tree/2.3.6) and
[LVGL 9.3.0](https://github.com/lvgl/lvgl/tree/v9.3.0). Their component notices
continue to apply; the project's MIT license does not replace them.

Native build tools (CMake, Ninja, Zig/Clang) and PlatformIO are development tools.
