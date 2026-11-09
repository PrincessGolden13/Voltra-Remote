# Contributing

Fork this repository, create a branch and open a pull request. Bug reports should
include the remote board/revision, application version, VOLTRA firmware, steps to
reproduce and sanitized logs. Remove Wi-Fi credentials, device addresses and
bootstrap identities before uploading anything.

Read [AGENTS.md](AGENTS.md), [architecture](docs/ARCHITECTURE.md),
[safety](docs/SAFETY.md) and [current application flow](docs/APP_FLOW.md).
Keep hardware drivers, generic BLE, the VOLTRA profile, machine-confirmed state
and the UI separate. Unknown readiness/capability must never enable loading.
Never run automated physical resistance tests.

Build the offline simulator for UI/controller work. Build both physical targets
after shared changes. New automated test code is currently deferred by the owner;
record missing regression cases in docs/TESTING.md and run existing validators.
Do not describe a compile, simulator result or source audit as physical validation.

The source is MIT except identified third-party portions/dependencies, which
retain their own licenses. Contributions are submitted under the project's
applicable license. Keep attribution intact.

Only maintainers publish firmware releases. A merged commit is not automatically
installed on remotes. See [release instructions](docs/FIRMWARE_UPDATES.md).
