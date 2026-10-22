#!/usr/bin/env python3
"""Decode the supplied white-background artwork into the generated asset catalog."""
import base64
import hashlib
import json
import subprocess
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = base64.b64decode((root / 'Artwork/AppIcon.webp.base64').read_bytes())
assert hashlib.sha256(source).hexdigest() == "5b4f7d98df45f538ebc297132f18fe4db2fbc53af14aa019752b8e828ed49d41"
work = root / 'build'
work.mkdir(exist_ok=True)
webp = work / 'AppIcon.webp'
webp.write_bytes(source)
catalog = root / 'Generated/Assets.xcassets'
icon = catalog / 'AppIcon.appiconset'
icon.mkdir(parents=True, exist_ok=True)
subprocess.run(['sips', '-s', 'format', 'png', str(webp), '--out', str(icon / 'AppIcon.png')], check=True)
(catalog / 'Contents.json').write_text(json.dumps({'info': {'author': 'xcode', 'version': 1}}))
(icon / 'Contents.json').write_text(json.dumps({
    'images': [{'filename': 'AppIcon.png', 'idiom': 'universal', 'platform': 'ios', 'size': '1024x1024'}],
    'info': {'author': 'xcode', 'version': 1}
}))
print('User-supplied opaque white-background icon prepared (1024 x 1024).')
