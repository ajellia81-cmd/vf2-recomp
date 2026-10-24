# VF2 sound/contact diagnostic analysis

## Scope

This note records conclusions from the user's local `haptics-sound-diagnostics.csv` capture so that the analysis remains available on the remote `ios-port` branch. The raw CSV is intentionally not committed.

## Observations

- The capture contains 49 `contact_store` events at guest PC `0x00018A7C`.
- 48 of 49 contact events have one or more `voice_start_request` events in the same emulated frame.
- Three contact events preserve zero health (`0 -> 0`) and still have same-frame voice requests:
  - frame 2556: note `0x01`
  - frame 2649: note `0x01`
  - frame 7010: note `0x25`
- Note `0x24` occurred only with the four largest observed hits (40-45 damage).
- Other note values overlap substantially across damage sizes. In particular, `0x25` appears with 0-30 damage and `0x01` with 0-19 damage.

## Decision

Use the verified contact-store event as the primary haptic trigger. Treat voice-start requests as optional flavor/context signals rather than a strict damage-to-sound strength table. This preserves KO and post-KO contacts that can have no health delta.

## Next device check

Verify on device that one normal hit, the KO hit, and a direct post-KO hit each produce exactly one haptic response. Only collect another diagnostic CSV if a response is absent or duplicated.
