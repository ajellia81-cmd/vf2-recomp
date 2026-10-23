import io
import unittest
from analyze_vf2_combat_sound import FIELDS, analyze, load


def hit(t, before=100, after=80, player=1, mode=9, round_=9, live=2):
    return dict(kind='contact_store', time_us=t, sequence=t, player=player,
                before=before, after=after, maximum=100, mode=mode, round=round_, live=live)


def sound(t, note=0x10):  # Synthetic ID: no claim about actual VF2 sound meaning.
    return dict(kind='voice_start_request', time_us=t, sequence=t, note=note)


class AnalysisTests(unittest.TestCase):
    def test_normal_and_post_ko_same_id(self):
        rows = [sound(1004), hit(1000), hit(2000, 0, 0, round_=17, live=0), sound(2005)]
        r = analyze(rows, 10)
        self.assertEqual(r['candidate_normal_samples'], 1)
        self.assertEqual(r['candidate_post_ko_contacts'], 1)
        self.assertEqual(r['notes']['0x10']['damage_median'], 20)
        self.assertEqual(r['notes']['0x10']['sound_delay_us_median'], 4)

    def test_ambiguous_and_unmatched_not_trained(self):
        r = analyze([hit(100), sound(99), sound(101), hit(200)], 10)
        self.assertEqual((r['ambiguous_contacts'], r['unmatched_contacts']), (1, 1))
        self.assertEqual(r['notes'], {})
        r = analyze([hit(100), hit(102, player=2), sound(101)], 10)
        self.assertEqual(r['ambiguous_contacts'], 2)

    def test_lethal_nonlive_and_invalid_max_not_trained(self):
        rows = [hit(100, 5, 0), sound(101), hit(200, round_=17), sound(201),
                hit(300, mode=8), sound(301), hit(400, live=0), sound(401)]
        h = hit(500); h['maximum'] = 0
        rows.extend([h, sound(501)])
        self.assertEqual(analyze(rows, 10)['notes'], {})

    def test_csv_and_weather_rejected(self):
        header = ','.join(FIELDS) + '\n'
        row = '0,voice_start_request,120,2,00600a96,0,-1,-1,0,9,9,2,14,10,31\n'
        rows = load(io.StringIO('# schema comment\n'+header+row))
        self.assertEqual(rows[0]['note'], 0x10)
        with self.assertRaises(ValueError):
            load(io.StringIO(header + row.replace(',14,10,31', ',14,22,31')))
        with self.assertRaises(ValueError):
            load(io.StringIO('wrong,schema\n'))
        with self.assertRaises(ValueError):
            analyze([], 0)


if __name__ == '__main__':
    unittest.main()
