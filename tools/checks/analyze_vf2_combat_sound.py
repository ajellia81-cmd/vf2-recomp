#!/usr/bin/env python3
"""Summarize candidate associations from scalar VF2 CSVs; never alter haptics.

A time window is a hypothesis, not evidence of causation. Try several windows
and compare recordings before assigning meaning to any effect ID.
"""
import argparse
import bisect
import csv
import json
import statistics

FIELDS = 'sequence kind time_us frame pc_hex player before after maximum mode round live channel note_hex slot'.split()
NUMBERS = 'sequence time_us frame player before after maximum mode round live channel slot'.split()


def load(lines):
    rows = []
    reader = csv.DictReader(line for line in lines if not line.startswith('#'))
    if reader.fieldnames != FIELDS:
        raise ValueError('Expected VF2 sound/contact CSV v1')
    for raw in reader:
        row = dict(raw)
        for key in NUMBERS:
            row[key] = int(raw[key])
        row['pc'] = int(raw['pc_hex'], 16)
        row['note'] = int(raw['note_hex'], 16) if raw['note_hex'] else None
        if row['kind'] not in ('contact_store', 'voice_start_request'):
            raise ValueError('Unknown observation kind')
        if row['time_us'] < 0:
            raise ValueError('Negative emulated timestamp')
        if row['kind'] == 'voice_start_request' and (
            row['channel'] != 14 or row['note'] is None or
            not 0 <= row['note'] <= 255 or row['note'] in (0x22, 0x28, 0x29)
        ):
            raise ValueError('Unexpected sound candidate')
        rows.append(row)
    return rows


def analyze(rows, window_us):
    if window_us <= 0:
        raise ValueError('Time window must be positive')
    contacts = [r for r in rows if r['kind'] == 'contact_store']
    # CPU scheduling can interleave observations: search emulated time rather
    # than assuming insertion order equals cross-CPU timestamp order.
    sounds = sorted((r for r in rows if r['kind'] == 'voice_start_request'),
                    key=lambda r: (r['time_us'], r['sequence']))
    times = [r['time_us'] for r in sounds]
    choices, uses = [], [0] * len(sounds)
    for hit in contacts:
        lo = bisect.bisect_left(times, hit['time_us'] - window_us)
        hi = bisect.bisect_right(times, hit['time_us'] + window_us)
        choices.append((lo, hi))
        for i in range(lo, hi):
            uses[i] += 1
    result = dict(window_us=window_us, contact_stores=len(contacts),
                  voice_requests=len(sounds), unmatched_contacts=0,
                  ambiguous_contacts=0, unique_candidates=0,
                  candidate_normal_samples=0, candidate_post_ko_contacts=0)
    groups = {}
    for hit, (lo, hi) in zip(contacts, choices):
        if hi == lo:
            result['unmatched_contacts'] += 1
            continue
        # Do not select the nearest sound from multiple candidates or reuse a
        # sound for simultaneous hits. Multiple slots may also be ambiguous.
        if hi-lo != 1 or uses[lo] != 1:
            result['ambiguous_contacts'] += 1
            continue
        result['unique_candidates'] += 1
        sound = sounds[lo]
        post_ko = hit['before'] == hit['after'] == 0 and hit['mode'] == 9
        if post_ko:
            result['candidate_post_ko_contacts'] += 1
        # Exclude lethal/clipped hits, refills, unknown maximum and non-live
        # states. KO remaining health is not the move's full damage.
        normal = (hit['mode'], hit['round'], hit['live']) == (9, 9, 2) and (
            0 < hit['after'] < hit['before'] <= hit['maximum'] <= 1000)
        if not normal and not post_ko:
            continue
        note = f"0x{sound['note']:02x}"
        group = groups.setdefault(note, dict(damage=[], fraction=[], delay_us=[], post_ko=0))
        if post_ko:
            group['post_ko'] += 1
        if normal:
            result['candidate_normal_samples'] += 1
            damage = hit['before'] - hit['after']
            group['damage'].append(damage)
            group['fraction'].append(damage / hit['maximum'])
            group['delay_us'].append(sound['time_us'] - hit['time_us'])
    result['notes'] = {}
    for note, group in sorted(groups.items()):
        d = group['damage']
        result['notes'][note] = dict(
            candidate_samples=len(d), post_ko_candidates=group['post_ko'],
            damage_min=min(d) if d else None, damage_max=max(d) if d else None,
            damage_median=statistics.median(d) if d else None,
            fraction_median=statistics.median(group['fraction']) if d else None,
            sound_delay_us_median=statistics.median(group['delay_us']) if d else None)
    result['interpretation'] = (
        'Provisional time-window associations only. Swishes share this channel; '
        'unique proximity does not prove a hit sound. Repeated recordings and '
        'latency checks are required. No haptic profiles are generated.')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv')
    parser.add_argument('--window-ms', type=float, required=True,
                        help='Provisional symmetric association window; verify latency using recordings')
    args = parser.parse_args()
    try:
        with open(args.csv, encoding='utf-8') as file:
            result = analyze(load(file), int(args.window_ms * 1000))
    except (ValueError, OSError, OverflowError) as exc:
        parser.error(str(exc))
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
