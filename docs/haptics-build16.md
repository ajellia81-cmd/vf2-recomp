# Personal iOS build 16: strongest fixed post-KO impacts

User feedback: build15 post-KO strength ordering felt reversed. Requested behavior:
all additional contacts after KO use the strongest ordinary profile. Do not
reintroduce variable sound-derived post-KO strength without a new user request.

The matcher now permits a sound estimate only for the decisive positive-to-zero
KO event. For zero-to-zero post-KO contacts, it always returns no estimate, selecting
the tracker saturated fallback (weight 1), regardless of sound ID, absence, weather,
ambiguity or overflow. At configured strength 100, landed low/high is 0.52/0.90 for
65 ms, received is 0.90/0.65 for 100 ms. Gain settings scale amplitude normally.
The post-KO event does not receive a repeated decisive-KO duration bonus.
Ordinary measured hits, decisive-KO behavior, duplicate suppression and lifecycle
handling are unchanged. Interpreter-only, no ROM data and no public release.

Regression checks compare post-KO responses across notes 01/03/25/24, unknown,
weather and missing requests for both victims, gains 0/25/75/100 and health ceilings
176/200/220/240. They check equal maximum amplitude/duration, bounded output,
no repeated KO tail and ordinary-hit parity. Build results must be recorded after CI.

## Clean distribution package

By user request this final package removes Test vibration, Record sound diagnostics
and Save hit diagnostics, all diagnostic capture instances, recording calls and
CSV export paths. Production contact detection and lethal effect matching remain;
post-KO feedback is fixed maximum as requested. Packaging verifies that diagnostic
UI/export strings and recorder symbols are absent from the executable. The IPA is
personal/unsigned; no public Release is created. Rendering, controls, saved
vibration enable/strength/player settings and warmup/lifecycle are preserved.

## Completed Apple validation

Workflow https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/40061384317 succeeded.
All configured regression tests, iPhone compilation, simulator document-import
tests, package verification and upload passed. VF3 also passed simulator app
compilation. VF2 package checks include absent diagnostic UI/export strings and
recorder symbols. No physical-device gameplay or vibration test was performed.
IPA SHA-256 (runner): e14d832c8af7ba35c36d72bbf46bcecee63e4bf8a5ca3e71e58469d2fccf253e
Artifact ID: 12547760748
Wrapper SHA-256 (artifact API): 813ed96c4ba1412ef38a54d3989b4040a3fd8c6d1b4afbf56b5e6fb9a81747e9
Download: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/40061384317/artifacts/12547760748
These digests were reported remotely; no independent archive download was performed.
