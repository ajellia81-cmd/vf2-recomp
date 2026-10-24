# Personal iOS build 15: sound-estimated lethal and post-KO impacts

The earlier build14 source at a4193eed already contained a post-KO effect matcher
introduced by 8a195a61. The Library build14 IPA predates that source change.
The build14 documentation and package metadata incorrectly described every
build14 variant as diagnostic-only. Build15 has a new bundle version and accurate
metadata so the two versions cannot be confused.

## Behavior

Ordinary nonlethal impacts retain the existing measured-damage response.
The verified contact writer remains mandatory; sound alone never produces a hit.
Lethal (positive-to-zero) and post-KO zero-to-zero contacts wait for a causal
channel-14 effect request in the following 8 emulated milliseconds. Their bounded
pending records are consumed after complete original frames, potentially the next
frame. Actual remaining-HP loss and the KO event are carried with delayed lethal
contacts, avoiding an early clipped pulse followed by a second sound pulse.
A delayed KO keeps its one extra tail; post-KO contacts do not repeat that tail.

The provisional effect estimates remain 01/03:15, 25:22, 24:45. They represent
approximate categories from the supplied diagnostic observations, not recovered
move damage. Different moves sharing an effect can still feel the same.
For zero-health matched contacts, measured clipped damage is a lower bound.
Sound-estimated responses use sqrt(clamp(4*magnitude/maximum,0,1)), instead of
the ordinary measured curve's factor 8, so medium/strong categories do not both
saturate at 22 damage with maximum 176. Ordinary measured-hit behavior is unchanged.

Unknown/multiple requests and overlapping contacts do not get a guessed ID:
lethal contacts retain measured remaining-HP damage, zero-damage post-KO contacts
retain the saturated fallback. Phone/controller gain, selected side and no-JIT
policy are unchanged. Pause/menu/background/reset discard pending associations.
Menu clears join the emulation worker before mutating the matcher.

## Validation

The Actions workflow now executes the previously omitted impact-sound regression
test with ASan/UBSan, alongside existing tracker, VF2 adapter and event tests.
Tests cover both victims, health ceilings 176/200/220/240, strengths 0/25/75/100,
causal timing and frame-boundary delivery, one KO tail, increasing light/medium/heavy
amplitude and duration, ordinary-hit parity, cancellation, ambiguous/unknown
requests, overflow and silence from sound alone. Device compilation checks the
actual production integration. CI result must be recorded after completion;
no physical gameplay, perceived strength or ROM import test is implied.

Match-mode replay contacts may still vibrate, as documented previously.
No ROMs, audio samples or program data are included.
