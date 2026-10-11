I am starting a new embedded development project: a standalone physical remote for the Beyond Power VOLTRA.

I am not a software developer and I want you to perform essentially 100% of the development work. I can connect hardware, flash/test builds when necessary, interact with the physical VOLTRA, and return logs/results to you, but you should own the architecture, code, build environment, debugging, tests, documentation, protocol investigation, and Git workflow.

Please treat everything below as the master requirements and standing engineering rules for this project.

# PROJECT GOAL

Build a standalone rotary touchscreen remote that connects directly to one or more Beyond Power VOLTRAs over Bluetooth Low Energy.

The initial prototype hardware is a 2.1-inch round ESP32-S3 rotary touchscreen device with approximately:

* ESP32-S3
* 480x480 round capacitive touchscreen
* physical rotary encoder surrounding the display
* encoder press action
* Wi-Fi/BLE
* likely 16 MB flash / 8 MB PSRAM

It appears closely related to the Elecrow CrowPanel 2.1-inch HMI ESP32 Rotary Display, but DO NOT assume its pinout, display controller, touch controller or hardware revision until we inspect the physical unit.

IMPORTANT: the remote hardware may change over the life of this project.

I may later:

* use a different commercially available rotary touchscreen
* use a different round-screen resolution
* use a different ESP32 board
* use a different MCU/BLE platform
* create custom production hardware
* sell these remotes
* need to replace discontinued controller hardware

Therefore, the application must not be architecturally tied to this particular 2.1-inch prototype.

Board-specific code must be isolated behind hardware interfaces.

The finished remote should NOT require:

* Home Assistant
* a phone
* a Raspberry Pi
* a PC
* Beyond Power Cortex
* an Internet connection

Normal operation should be:

Remote hardware
↓ BLE
VOLTRA

Wi-Fi may eventually be useful for firmware updates, diagnostics or optional functionality, but it must not be required for basic VOLTRA control.

# EXISTING PROJECTS / RESEARCH SOURCES

These are research/reference sources.

They should NOT become directories inside my working repository and should not become runtime dependencies unless there is a compelling technical reason.

If local clones are useful for investigation, clone them outside my working project.

## 1. Ryan Markoff-Eaton VOLTRA DIY Remote

https://github.com/RyanMarkoff-eaton/voltra-diy-remote

This is an important starter project because it targets essentially the same use case and similar 2.1-inch ESP32-S3 rotary hardware.

Use it as a starting firmware/hardware reference.

It already contains work involving:

* BLE scanning
* VOLTRA connection
* handshake
* reconnect
* reading settings
* encoder interaction
* display/touch support
* base weight adjustment
* eccentric
* chains
* inverse chains
* load/unload
* drop-set concepts

However, do not blindly preserve its architecture or hard-coded protocol approach.

Preserve a known-working baseline before major refactoring.

## 2. Unofficial VOLTRA Node SDK

https://github.com/HJewkes/voltra-node-sdk

Treat this as a major protocol reference.

It has a more mature abstraction of the VOLTRA including:

* connection
* protocol framing
* commands
* parameters
* training modes
* current state
* real-time telemetry
* reps
* velocity
* position
* force
* events
* multiple devices
* mock BLE/device support

Ryan's project appears to have ported some protocol information from an earlier version of this SDK.

Review its CURRENT implementation, tests, changelog and relevant history rather than assuming Ryan's snapshot is complete.

## 3. Beyond Power Home Assistant integration

https://github.com/dylanmaniatakes/Beyond-Power-HomeAssistant

This independently communicates directly with VOLTRA over BLE.

Use it as an independent protocol cross-check, especially for:

* service/characteristic UUIDs
* connection/handshake
* 0x55 frame behavior
* CRC
* parameter IDs
* notifications
* state parsing
* training state
* load readiness
* battery/state data
* mode support
* reps/sets/telemetry

Its state-aware approach is particularly useful.

## 4. Beyond Power VOLTRA Android project / protocol research

https://github.com/dylanmaniatakes/Beyond-Power-Voltra-Android

Treat this as another major research source.

It contains protocol notes, parameter information and analysis derived partly from the official Beyond+ application.

This source is particularly important for:

* firmware/module information
* capability discovery
* beta functionality
* less-common protocol parameters
* Twin/coworker state
* master/slave concepts
* Twin telemetry
* Twin compatibility behavior
* safety-related device state

Inspect relevant protocol notes, parameter databases, commits, tests and static-analysis artifacts.

Do NOT assume every discovered parameter is safe, writable, current or appropriate for our remote.

## 5. Beyond Power Cortex

Getting started:
https://cortex.beyond-power.com/docs/getting-started

Documentation:
https://cortex.beyond-power.com/docs/

Cortex is Beyond Power's official local API/CLI.

We DO NOT intend to run Cortex on the remote or make Cortex a runtime dependency.

Use Cortex as an official behavioral reference for:

* supported training modes
* official terminology
* expected command sequencing
* load/unload behavior
* device readiness
* cable-zero/safety behavior
* settings
* mode transitions
* safety expectations

If unofficial reverse-engineering conflicts with documented Beyond Power safety behavior, use the conservative interpretation.

# REPOSITORY PHILOSOPHY

I DO NOT want copies of other people's projects inside my application's repository.

They are research sources, not product source code.

My repository should contain only:

* this project's source code
* tests
* mocks
* simulator
* development tools
* fixtures/captures
* documentation
* build configuration

# CORE ARCHITECTURE

The VOLTRA side should conceptually be:

UI
↓
VOLTRA Controller
↓
Requested / Confirmed State
↓
VOLTRA Protocol
↓
VOLTRA GATT Profile
↓
BLE Transport
↓
VOLTRA

The remote-hardware side should conceptually be:

UI / Application
↓
Hardware Interfaces
↓
Board Implementation
↓
Physical Remote Hardware

There are several important abstraction boundaries:

1. Different VOLTRA firmware/capabilities must not require rewriting the UI/application.
2. Different physical remote hardware must not require rewriting the application.
3. Different BLE stacks must not require rewriting the VOLTRA protocol/controller.
4. VOLTRA-specific GATT details must not become generic BLE transport logic.

# BLE VS VOLTRA PROTOCOL RESPONSIBILITIES

Do NOT create different BLE folders for Bluetooth 4, Bluetooth 5, etc.

The underlying BLE stack/chipset should handle Bluetooth specification differences.

Generic BLE transport should expose operations such as:

* scan
* connect
* disconnect
* discover services
* discover characteristics
* subscribe
* write
* connection callbacks

VOLTRA-specific information belongs in the VOLTRA protocol/profile layer, including:

* service UUIDs
* characteristic UUIDs
* handshake behavior
* GATT profile selection
* command framing
* sequence numbers
* CRC
* parameter IDs
* response parsing

If a future remote platform does not use NimBLE, create another implementation of BleTransport rather than changing the application.

If a future VOLTRA firmware genuinely introduces an incompatible GATT profile, support multiple VOLTRA profiles behind a common interface rather than spreading firmware checks through generic BLE code.

# PROJECT STRUCTURE

Use a structure broadly like this:

voltra-remote/
│
├── src/
│   ├── ui/
│   │   ├── screens/
│   │   ├── components/
│   │   └── UiController.*
│   │
│   ├── controller/
│   │   └── VoltraController.*
│   │
│   ├── state/
│   │   ├── VoltraState.*
│   │   ├── RequestedState.*
│   │   ├── VoltraCapabilities.*
│   │   ├── TwinState.*
│   │   ├── UiState.*
│   │   └── HardwareCapabilities.*
│   │
│   ├── protocol/
│   │   ├── VoltraGattProfile.*
│   │   ├── FrameBuilder.*
│   │   ├── FrameParser.*
│   │   ├── Commands.*
│   │   ├── Parameters.*
│   │   └── Crc.*
│   │
│   ├── ble/
│   │   ├── BleTransport.*
│   │   ├── BleTypes.*
│   │   └── NimBleTransport.*
│   │
│   ├── hardware/
│   │   ├── interfaces/
│   │   │   ├── HardwarePlatform.*
│   │   │   ├── Display.*
│   │   │   ├── TouchInput.*
│   │   │   ├── RotaryInput.*
│   │   │   ├── Haptics.*
│   │   │   ├── Backlight.*
│   │   │   └── PowerStatus.*
│   │   │
│   │   └── boards/
│   │       ├── prototype_21/
│   │       │   ├── Board.*
│   │       │   ├── Display.*
│   │       │   ├── Touch.*
│   │       │   ├── Rotary.*
│   │       │   ├── BoardConfig.*
│   │       │   └── HardwareCapabilities.*
│   │       │
│   │       └── future_board/
│   │           └── ...
│   │
│   └── main.cpp
│
├── simulator/
│   ├── MockVoltra.*
│   ├── MockTransport.*
│   ├── DesktopHardwarePlatform.*
│   ├── DesktopDisplay.*
│   ├── SimulatorProfiles.*
│   └── main.cpp
│
├── tests/
│   ├── protocol/
│   │   ├── frame_builder/
│   │   ├── frame_parser/
│   │   ├── crc/
│   │   ├── commands/
│   │   ├── parameters/
│   │   └── gatt_profile/
│   │
│   ├── state/
│   │   ├── requested_state/
│   │   ├── confirmed_state/
│   │   ├── capabilities/
│   │   └── twin_state/
│   │
│   ├── controller/
│   │   ├── settings/
│   │   ├── modes/
│   │   ├── load_guards/
│   │   ├── unload/
│   │   ├── timeouts/
│   │   ├── reconnection/
│   │   └── twin_aware/
│   │
│   ├── ble/
│   │   ├── connection/
│   │   ├── handshake/
│   │   ├── notifications/
│   │   └── reconnect/
│   │
│   ├── telemetry/
│   │   ├── reps/
│   │   ├── sets/
│   │   ├── force/
│   │   ├── velocity/
│   │   ├── position/
│   │   └── twin/
│   │
│   ├── hardware/
│   │   ├── interfaces/
│   │   ├── board_profiles/
│   │   ├── display_metrics/
│   │   └── input_mapping/
│   │
│   ├── safety/
│   │   ├── load_safety/
│   │   ├── state_safety/
│   │   ├── twin_safety/
│   │   └── regression/
│   │
│   ├── ui/
│   │   ├── interactions/
│   │   ├── responsive_layout/
│   │   └── confirmed_vs_pending/
│   │
│   ├── integration/
│   │   ├── mock_voltra/
│   │   └── simulated_sessions/
│   │
│   ├── mocks/
│   │   ├── MockTransport.*
│   │   ├── MockVoltra.*
│   │   ├── MockHardwarePlatform.*
│   │   └── MockClock.*
│   │
│   └── fixtures/
│       ├── captured_packets/
│       ├── device_profiles/
│       ├── hardware_profiles/
│       ├── telemetry/
│       ├── twin/
│       └── malformed_packets/
│
├── tools/
│   ├── packet_inspector/
│   └── capture_tools/
│
├── docs/
│   ├── SAFETY.md
│   ├── VOLTRA_COMPATIBILITY.md
│   ├── HARDWARE_COMPATIBILITY.md
│   ├── PROTOCOL_NOTES.md
│   ├── TWIN_MODE.md
│   └── TESTING.md
│
├── AGENTS.md
└── platformio.ini

The exact nesting may be simplified if the build/test framework strongly prefers another convention, but preserve the conceptual separation.

Do not create abstraction purely for abstraction's sake.

Do not duplicate simulator/test mocks unnecessarily.

# REMOTE HARDWARE ABSTRACTION

The current prototype is only the FIRST supported remote platform.

Treat it as one board implementation, not as the definition of the product.

Application code should interact with logical devices such as:

* Display
* TouchInput
* RotaryInput
* HardwarePlatform

rather than directly using board GPIOs or vendor-specific drivers.

Conceptually:

rotary.getDelta()
rotary.wasPressed()

rather than:

digitalRead(GPIO...)

Clockwise rotation must produce the same logical positive event regardless of physical encoder implementation.

Board-specific differences such as reversed encoder direction must be corrected inside the board implementation.

Likewise:

* touch coordinates should be normalized
* display initialization is board-specific
* backlight is board-specific
* haptics are board-specific
* battery/power telemetry is board-specific

# REQUIRED VS OPTIONAL REMOTE HARDWARE

Treat these as REQUIRED for the core product unless requirements change later:

* suitable BLE support
* round display
* touchscreen
* physical rotary encoder
* rotary encoder press action

Potentially OPTIONAL:

* Wi-Fi
* haptics
* battery
* battery gauge
* speaker/buzzer
* ambient light sensor
* additional physical buttons

Optional hardware must not be required for core operation.

If a target lacks a required capability, it should fail target validation or be explicitly marked unsupported.

# HARDWARE CAPABILITIES

Create HardwareCapabilities separately from VoltraCapabilities.

Conceptually it may expose:

displayWidth
displayHeight
displayShape
touchAvailable
rotaryAvailable
rotaryPressAvailable
hapticsAvailable
batteryAvailable
wifiAvailable
bluetoothAvailable
speakerAvailable
ambientLightSensorAvailable

Do not over-engineer unused optional capabilities, but establish a clean model for board variation.

# DISPLAY RESOLUTION / RESPONSIVE UI

Do NOT hard-code the application to 480x480.

Future round displays might be:

* 360x360
* 466x466
* 480x480
* another similar resolution

Avoid scattering assumptions such as:

centerX = 240
centerY = 240
radius = 230

through UI code.

Expose dimensions through the display/hardware abstraction.

Use LVGL or another appropriate layout system so the UI can adapt to sensible supported round-screen resolutions.

We do not need arbitrary desktop/TV resolution support.

Across all supported physical targets preserve:

* legibility
* safe margins
* usable touch targets
* consistent rotary interaction
* STOP accessibility

# PLATFORMIO / BUILD TARGETS

Different physical boards should use separate PlatformIO environments or equivalent target configurations.

Conceptually:

[env:prototype_21]

[env:future_rotary_board]

[env:custom_remote_v1]

Each target may define its own:

* MCU/board definition
* GPIO mapping
* display driver
* touch driver
* rotary encoder implementation
* memory settings
* PSRAM configuration
* flash settings
* compile flags

Each target should compile the same core:

* UI
* VOLTRA controller
* state
* protocol
* safety logic

Document supported hardware in:

docs/HARDWARE_COMPATIBILITY.md

# STATE MODEL

There must be strict separation between UiState, RequestedState and VoltraState.

## UiState

State belonging only to the remote UI.

Examples:

* current screen
* selected control
* menu state
* encoder focus
* brightness
* theme

## RequestedState

What the user has asked the VOLTRA to do, but which may not yet be machine-confirmed.

Example:

VoltraState.weight = 100

User selects 120.

Immediately:

RequestedState.weight = 120
VoltraState.weight = 100

After BLE transmission succeeds but before machine confirmation:

RequestedState.weight = 120
VoltraState.weight = 100
CommandStatus = PENDING

Only after VOLTRA confirms it:

RequestedState.weight = 120
VoltraState.weight = 120
CommandStatus = CONFIRMED

If rejected or timed out:

RequestedState.weight = 120
VoltraState.weight = 100
CommandStatus = FAILED or TIMEOUT

A successful BLE write is NOT proof that VOLTRA applied the requested setting.

The VOLTRA's reported state is the source of truth.

# VOLTRA FIRMWARE / CAPABILITY COMPATIBILITY

VOLTRA functionality changes with firmware.

Examples include:

* new training modes
* new parameters/selectors
* new telemetry
* newer maximum-resistance capabilities
* Mountain / MTN and other beta functionality
* different internal module firmware versions

Do NOT simply assume:

firmware >= X = everything supported

Prefer capability-based support.

Create VoltraCapabilities.

Conceptually it may expose:

maxWeightLbs
weightTraining
resistanceBand
damper
isokinetic
isometric
customCurves
rowing
skiing
mountain
dtr
chains
inverseChains
percentageProfiles
autoLoad
autoUnload
twinAware

Capabilities should come from device-reported information wherever possible.

Firmware/component versions should still be recorded for:

* diagnostics
* compatibility analysis
* fallback decisions
* bug reports

Use:

CAPABILITY DETECTION FIRST
FIRMWARE-VERSION INFERENCE SECOND

Unknown capability = unavailable.

Never enable a potentially dangerous function because we merely think the firmware probably supports it.

Do NOT create protocol_v1 / protocol_v2 directories unless investigation shows a genuine incompatible wire-protocol split.

Prefer one protocol implementation with capability/profile-aware behavior.

# TWIN MODE REQUIREMENT

Twin-aware operation is a core project requirement.

However:

THE REMOTE DOES NOT NEED TO CREATE OR MANAGE TWIN PAIRING.

Twin pairing/unpairing remains outside this project's normal scope.

The remote must NOT:

* pair two VOLTRAs into Twin Mode
* unpair Twin VOLTRAs
* alter the master/slave relationship
* silently enable or disable Twin Mode

Instead, the remote must detect and correctly support a VOLTRA that is ALREADY operating in Twin Mode.

# TWIN STATE

Create a TwinState model as part of the machine-confirmed VOLTRA session state.

Only include fields once protocol evidence supports them.

Likely concepts include:

active
role                  // MASTER / SLAVE / UNKNOWN
peerConnected
peerReady
peerLocked
peerBattery
peerDeviceId
statusFresh

Potentially later:

combinedMaxResistance
combinedForce
peerForce
peerCablePosition
peerVelocity

Do not invent unsupported state.

TwinState must be derived from VOLTRA-reported data, not UI assumptions.

# TWIN CONTROL SEMANTICS

The working hypothesis is:

When a Twin pair is active, the remote should communicate with the appropriate connected VOLTRA—likely the Twin master—using the same user-facing combined resistance value shown by the official VOLTRA/Beyond+ experience.

For example:

User selects 300 lb total
↓
remote sends a 300 lb Twin weight request to the appropriate VOLTRA
↓
the Twin pair internally distributes/co-ordinates approximately 150 lb per unit

This is the current preferred hypothesis.

DO NOT implement automatic divide-by-two logic.

DO NOT independently send duplicate weight commands to both VOLTRAs.

DO NOT assume the remote must maintain BLE connections to both Twin units.

Instead, investigate and verify the official Twin behavior from:

* official-app artifacts
* Dylan's Android/static-analysis material
* available protocol captures
* safe physical testing

Only change this model if evidence shows the working hypothesis is wrong.

Mark it as INFERRED until verified.

# TWIN EFFECTIVE WEIGHT LIMIT

Twin Mode changes the user-facing effective resistance range.

Do not simply hard-code:

if twin:
maxWeight *= 2

Instead determine the effective maximum using, in priority order:

1. Explicit machine-reported Twin capability/combined maximum, if available.
2. Verified machine-reported per-unit maximum combined with verified Twin behavior.
3. Known firmware compatibility rule.
4. Otherwise UNKNOWN/conservative limit.

For example, a 200 lb single-unit maximum may correspond to a 400 lb Twin combined maximum, but this should be represented as an effective Twin capability rather than scattered arithmetic throughout the UI.

If newer VOLTRA firmware changes per-device capacity, Twin limits must adapt accordingly when verified.

The UI should present user-facing combined resistance in Twin Mode.

# TWIN MASTER/SLAVE BEHAVIOR

On connection, investigate and determine:

* whether this VOLTRA is in Twin Mode
* whether it is master or slave (note: dont use the words master and slave anywhere in code or documentation - use primary and secondary or something else instead)
* whether the peer is connected
* whether peer health/readiness is reported
* whether commands should only be sent to the master
* whether connecting to a slave is supported
* whether commands to a slave are forwarded
* whether the pair appears as one logical controllable unit
* whether Twin pairing persists across reboots
* how combined target resistance is represented
* how Twin force/position/velocity telemetry is reported
* whether the app connects to one or both machines after pairing

Do not guess these behaviors where protocol evidence can determine them.



# GIT / PROTOCOL RESEARCH

Inspect relevant:

* source
* changelogs
* commits
* issues
* pull requests
* tests
* protocol notes
* parameter databases

from the reference projects.

Focus on compatibility-relevant discoveries such as:

* parameters/features
* training modes
* packet structures
* UUIDs/GATT profiles
* handshake behavior
* CRC/framing
* telemetry
* firmware-specific behavior
* capability detection
* firmware/module reporting
* beta features
* safety/readiness state
* Twin/coworker state and telemetry

Determine:

1. Whether different VOLTRA firmware versions genuinely require different protocol implementations.
2. What capability information can be read directly.
3. Which compatibility rules require firmware fallback.
4. Whether unknown commands/features may behave unpredictably.
5. Whether Mountain/MTN or other beta features are already represented in captured data.
6. Whether one reverse-engineered implementation contains important knowledge absent from another.
7. Whether safety behavior changes by firmware.
8. How already-paired Twin VOLTRAs expose information

Document findings in:

docs/VOLTRA_COMPATIBILITY.md
docs/PROTOCOL_NOTES.md
docs/TWIN_MODE.md

Clearly label important findings as:

CONFIRMED
INFERRED
UNVERIFIED

# SIMULATOR / DESKTOP DEVELOPMENT

I strongly want a desktop simulator.

Normal UI/controller development should NOT require:

edit
→ compile
→ flash remote
→ connect physical VOLTRA
→ navigate
→ inspect
→ repeat

The majority of development should be possible on Windows.

Investigate LVGL as the UI framework because of embedded and desktop simulation support, but choose based on technical fit rather than blindly migrating.

The simulator should emulate:

mouse click = touchscreen
mouse wheel = rotary encoder
left/right keys = encoder rotation if useful
Enter/Space = rotary press

The simulator should run the same:

* UI
* controller
* state
* safety logic
* protocol logic where practical

used by the embedded firmware.

The desktop simulator should implement the same HardwarePlatform abstraction as physical hardware where practical.

# MOCK VOLTRA

Create MockTransport / MockVoltra so VoltraController can operate against:

REAL:

BleTransport
↓
physical VOLTRA

or

SIMULATED:

MockTransport
↓
MockVoltra

The controller should not need to know which it is using.

The simulator should eventually support:

* successful connection
* connection loss
* reconnect
* slow acknowledgement
* timeout
* rejection
* unsupported feature
* old firmware
* stable firmware
* beta firmware
* unknown/future firmware
* different maximum resistance
* wrong mode
* cable not zero
* not load-ready
* loaded/unloaded
* ambiguous load state
* battery
* reps
* sets
* force
* velocity
* position
* telemetry
* mode changes

Also include Twin profiles such as:

* single VOLTRA
* healthy Twin master
* healthy Twin slave
* Twin peer disconnected
* Twin peer state stale
* Twin firmware/model incompatible
* Twin combined max different from single max
* Twin command timeout
* Twin peer lost during session

Simulator builds must be structurally incapable of accidentally issuing production BLE commands.

Prefer compile/build-target isolation rather than runtime flags alone.

# SAFETY REQUIREMENTS

This remote controls powered exercise resistance.

Treat safety logic as first-class architecture.

These rules are mandatory.

## 1. Never automatically engage resistance

Resistance must NEVER activate merely because of:

* boot
* reboot
* BLE connect
* BLE reconnect
* screen wake
* restoring UI state
* restoring RequestedState
* restoring preferences
* mode change
* crash recovery
* firmware update
* simulator transition
* Twin state transition
* Twin peer reconnection

Every load requires explicit CURRENT user action.

## 2. STOP/Unloading weight is critical

We want to ensure that when weight is loaded, it is very simple and intuitive at all times to unload the weight. Copy the design patterns from the voltra UI (i.e. text is highlighted/bright when loaded, and a single tap of the weight will unload it and turn the text a lighter color.

## 3. Never bypass VOLTRA readiness checks

Do not implement an equivalent of Cortex's force/bypass loading.

If VOLTRA indicates:

* cable not zero
* device not ready
* invalid training state
* unsafe state
* stale state
* unknown state

DO NOT LOAD weight

UNKNOWN = NOT SAFE.
There are some options on the voltra like skipping a safety check etc to load weight under certain conditions. I am not going to copy those in my voltra remote, so the user would need to pick those options on the voltra screen directly if they want that.

## 4. Respect physical-risk safety behavior

Cortex includes physical-risk acknowledgement concepts.

Respect Beyond Power's intended safety model.

Where Cortex-side acknowledgement differs from device-side BLE safety state, document that distinction.

Never invent a shortcut intended to bypass machine safety.

## 5. Auto Load / Auto Unload are READ ONLY

The remote must NEVER modify the user's Auto Load or Auto Unload configuration.

It may:

* read them
* display them
* account for them

It must not:

* enable
* disable
* alter sensitivity
* reset
* overwrite
* initialize them to defaults

This applies during:

* boot
* reconnect
* mode changes
* Twin detection
* any other initialization

## 6. Requested state is not confirmed state

Successful BLE transmission is NOT proof of machine action.

Only VOLTRA-reported state can become confirmed VoltraState.

## 7. Unknown feature = disabled

Never send an unverified command because:

* beta firmware mentions it
* a selector value looks plausible
* a neighboring parameter suggests it
* another app exposes it

Unknown/incompletely understood features remain unavailable.

## 8. No dangerous automated physical tests

Automated tests must never intentionally engage a physical VOLTRA motor.

Use mocks/fixtures/simulator ONLY.

Physical resistance testing requires explicit human initiation.

## 9. Guard load

Conceptually:

explicit user LOAD action
↓
BLE connection valid
↓
handshake valid
↓
fresh machine state
↓
supported mode
↓
device load-ready
↓
cable/safety state acceptable
↓
Twin state acceptable if applicable
↓
no conflicting pending/error state
↓
send LOAD
↓
wait for machine confirmation

If any required condition is NO or UNKNOWN:

DO NOT LOAD.

## 10. No secrets in Git

Do not commit:

* sensitive BLE identifiers where unnecessary
* API keys
* credentials
* account information
* machine-specific secrets

Use ignored local configuration where appropriate.

I also want to avoid hard coding generally. If there are certain values that must be hard coded, I want you to put them in variables and tell me about them so I can decide if that is ok.

# MULTIPLE VOLTRAS

Architect the transport/session system so multiple VOLTRA instances are possible.

Do not bake a global single-device assumption into:

* BLE
* protocol
* state
* controller/session management

However, distinguish:

MULTIPLE INDEPENDENT VOLTRAS

from:

TWIN MODE

Twin Mode is an existing relationship between VOLTRAs with its own master/slave behavior.

Do not fake Twin Mode by merely issuing simultaneous commands to two independent devices.
However, I want 1 voltra remote to be able to be used across multiple voltras.

# UI PHILOSOPHY

This is a physical training controller, not a tiny general-purpose computer.

The rotary encoder should be a first-class input.

Examples:

Home screen:
turn encoder = adjust primary value such as weight

Touch:
select setting/mode

Encoder:
adjust selected value

Encoder press:
select/confirm where appropriate

The UI should prioritize:

* large readable values
* fast adjustments
* minimal taps
* rotary-first interaction
* clear confirmed vs pending state
* clear connection/readiness state
* immediate STOP access
* clear Twin indication when applicable
* combined user-facing resistance in Twin Mode
* responsive support for multiple round-screen resolutions

# TESTING PHILOSOPHY

Testing is mandatory.

Focus particularly on:

* safety boundaries
* state correctness
* protocol correctness
* acknowledgement behavior
* failure behavior
* reconnect behavior
* capability handling
* Twin-awareness
* hardware portability
* regressions

Most tests should use:

* mocks
* fixtures
* captured packets
* simulated device profiles
* desktop simulator

Every meaningful bug involving safety, protocol, state, compatibility, Twin behavior, reconnect or hardware abstraction should receive a regression test whenever practical.

Safety generally fails closed:

UNKNOWN = NOT SAFE

except where specifically documented otherwise, particularly STOP/unload.

Do not create fake passing tests for unimplemented functionality.

Track them as pending requirements.

# REQUIRED TEST CASES

Implement tests progressively as the relevant functionality exists.
THat is, I dont want you to create test code right now - I am just including it while I think about it so you have the context for how we might incorporate it in the future.

1. The UI wont let me change the weight beyond the max weight of the voltra I am connected to. If I do keep turning the knob past it, it will reduce any overload modes (chains, eccentric, etc) first, but eventually it will stop at max weight.
2. If the bluetooth connection is lost or dropped, the UI handles it correctly - shows error messages, takes me back out to the home screen, etc.
3. If the weight is loaded, there is always an unload button/press/etc available on the screen
4. The UI doesnt show me options for settings my voltra doesnt support (e.g. MTN mode is not an option before voltra firmware version 2)


# DEVELOPMENT WORKFLOW

I want professional Git practices but I do not want to become the developer.

Please manage:

* repository setup
* branches when useful
* commits
* commit messages
* build configuration
* dependency management
* tests
* documentation
* troubleshooting

Do not make destructive GitHub changes without understanding them.

Keep commits logically grouped so regressions are easy to revert.

Before a major architectural refactor, preserve a known-working baseline.

Do not combine large refactoring with unrelated feature development in one commit.

# AGENTS.MD

Create AGENTS.md at the root as concise standing instructions.

Include at minimum:

* architecture boundaries
* UI/controller/protocol/BLE separation
* generic BLE transport vs VOLTRA GATT/profile responsibilities
* board-specific hardware isolation
* hardware interfaces vs board implementations
* support for future remote hardware
* no fixed 480x480 assumption
* VoltraState vs RequestedState
* VoltraState is machine-confirmed only
* VoltraCapabilities
* HardwareCapabilities
* TwinState is machine-confirmed state
* Twin-aware control is required but Twin pairing/unpairing is NOT part of normal remote scope
* capability-based compatibility
* unknown capability disabled
* no automatic loading
* explicit current user action required
* no force/bypass loading
* Auto Load/Auto Unload read-only.
* That the main goal of this remote is just to control the weight training options within the voltra - not adjust settings, cable length, DTR, do isokinetic testing, etc.
* Unload as a priority option to be preserved on the UI when possible
* stale/unknown state prevents loading
* stale/unknown required Twin peer state prevents loading
* automated tests must not intentionally engage resistance
* simulator isolated from real BLE
* multiple VOLTRA architecture without confusing it with Twin Mode
* no secrets committed
* safety/protocol/state/Twin/hardware bugs receive regression tests

# INITIAL DEVELOPMENT WORK

Begin by establishing a known-good baseline and development environment.

Initially:

1. Inspect all relevant current repositories and documentation.

2. Review relevant source/history/tests around:

   * protocol compatibility
   * firmware compatibility
   * BLE/GATT behavior
   * training modes
   * capability detection
   * safety/readiness
   * telemetry
   * beta features
   * Twin/coworker state

3. Set up my working repository.

4. Use voltra-diy-remote as starter/reference code where useful.

5. Keep reference repositories outside my source tree.

6. Install only the minimum necessary development environment.

Prefer PlatformIO because the starter already uses it.

7. Confirm the starter firmware can compile.

8. Run existing packet/protocol validation tests.

9. Preserve a known-working baseline before major refactoring.

10. Establish the architecture described above.

11. Establish HardwarePlatform abstraction before prototype-specific details spread through the application.

12. Treat the initial 2.1-inch board as the first implementation, not the product architecture.

13. Separate generic BLE transport from VoltraGattProfile.

14. Investigate and choose an appropriate shared UI architecture.

15. Investigate LVGL specifically.

16. Implement/scaffold the desktop simulator.

17. Implement/scaffold:

* BleTransport
* BleTypes
* NimBleTransport
* VoltraGattProfile
* MockTransport
* MockVoltra
* VoltraController
* VoltraState
* RequestedState
* TwinState
* UiState
* VoltraCapabilities
* HardwareCapabilities
* HardwarePlatform
* Display
* TouchInput
* RotaryInput
* first board implementation

18. Establish tests and begin implementing tests for already-understood functionality.

19. Create:

* docs/SAFETY.md
* docs/VOLTRA_COMPATIBILITY.md
* docs/HARDWARE_COMPATIBILITY.md
* docs/PROTOCOL_NOTES.md
* docs/TWIN_MODE.md
* docs/TESTING.md
* AGENTS.md

20. Research existing Twin/coworker protocol evidence and document:

* detection method
* role detection
* peer-state information
* effective max-weight evidence
* combined-weight semantics
* command-routing evidence
* uncertainties requiring physical validation

21. Do not create fake tests to satisfy coverage.

22. Do not enable or physically test load commands during initial setup.

# DEVELOPMENT MILESTONES

## MILESTONE 0 — Foundation

* environment
* Git
* upstream audit
* known-working starter baseline
* architecture
* BLE/profile separation
* hardware abstraction
* tests
* simulator
* mock transport/device
* documentation
* Twin protocol research

## MILESTONE 1 — Physical remote validation

* board identification
* display
* touch
* rotary
* rotary press
* USB/serial
* BLE scan
* HardwareCapabilities

## MILESTONE 2 — Read-only VOLTRA connection

* scan
* connect
* handshake
* notifications
* device info
* firmware/component versions
* state
* settings
* capabilities
* Twin state detection

No resistance commands.

## MILESTONE 3 — Safe unloaded parameter adjustment

Verified parameters such as:

* weight
* chains
* inverse chains
* eccentric
* other supported settings

Requested and confirmed state remain distinct.

Include Twin-aware unloaded parameter behavior once verified.

## MILESTONE 4 — Guarded load/unload

Only after readiness/safety behavior is understood, tested and physically verified.

Include Twin-specific readiness validation.

## MILESTONE 5 — Live telemetry

Potentially:

* cable position
* force
* velocity
* movement phase
* reps
* sets
* Twin/coworker telemetry

## MILESTONE 6 — Additional modes

Only when verified:

* Resistance Band
* Damper
* Isokinetic
* Isometric
* Custom Curves
* Rowing
* Ski
* Mountain / MTN
* DTR
* other verified functionality

## MILESTONE 7 — Advanced UX

Potentially:

* presets
* quick weight adjustments
* drop sets
* exercise profiles
* richer telemetry
* training workflows

## MILESTONE 8 — Multi-device support

Potentially:

* selection between independent VOLTRAs
* remembered devices
* independent device sessions

Do not confuse this with Twin Mode.

## MILESTONE 9 — Additional remote hardware

* add second physical rotary hardware target
* compile same application for both
* validate responsive UI
* validate equivalent input semantics
* document hardware differences

# GENERAL WORKING STYLE

Do the development work yourself instead of giving me tutorials for tasks you can perform directly.

If you can inspect a file, inspect it.

If you can modify code, modify it.

If you can run a build, run it.

If you can run tests, run them.

If something fails, investigate before asking me to troubleshoot it.

Only ask me to intervene when something genuinely requires:

* physical hardware
* physical VOLTRA interaction
* an account permission
* a physical observation
* information inaccessible to you

When reporting findings, distinguish clearly:

CONFIRMED
INFERRED
UNVERIFIED

Do not present reverse-engineered assumptions as official Beyond Power behavior.

Do not present suspected board compatibility as confirmed hardware compatibility.

Do not present the current Twin command hypothesis as confirmed until protocol evidence or physical testing establishes it.

Record protocol evidence in:

docs/PROTOCOL_NOTES.md

Record Twin findings in:

docs/TWIN_MODE.md

Record board-specific findings in:

docs/HARDWARE_COMPATIBILITY.md

For anything capable of applying physical resistance, use the conservative/safe interpretation.

Do not optimize for a flashy demo at the expense of:

* safety
* architecture
* state correctness
* testability
* Twin correctness
* hardware portability
* firmware compatibility

Start by auditing the environment and reference sources, establishing the known-working baseline, creating the architecture/hardware abstractions/test framework/simulator, researching Twin-aware behavior, and reporting what is CONFIRMED versus INFERRED before making major firmware changes.
