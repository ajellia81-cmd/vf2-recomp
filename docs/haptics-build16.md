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
