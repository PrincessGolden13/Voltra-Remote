"""Reject unvalidated board targets before any build/upload action."""
raise RuntimeError(
    "prototype_21 is not identified. Board validation and a reviewed board adapter "
    "are required; no flashable product firmware exists yet. See docs/HARDWARE_COMPATIBILITY.md."
)
