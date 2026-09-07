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

PlatformIO, Espressif/Arduino, NimBLE and Arduino GFX remain dependencies of the
external starter build, not dependencies of the desktop simulator. Native build
tools (CMake, Ninja, Zig/Clang) are development tools. Review all transitive
runtime notices before distributing a compiled product.
