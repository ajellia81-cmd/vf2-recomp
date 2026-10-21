#!/usr/bin/env python3
"""Inspect XcodeGen output, without compiling, signing or accessing any ROM."""
import json
import pathlib
import subprocess

root = pathlib.Path(__file__).resolve().parents[1]
project = root / 'VF2Recomp.xcodeproj' / 'project.pbxproj'
data = json.loads(subprocess.check_output(['plutil', '-convert', 'json', '-o', '-', str(project)]))
objects = data['objects']
app = next(v for v in objects.values() if v.get('isa') == 'PBXNativeTarget' and v['name'] == 'VF2Recomp')
phases = [objects[key] for key in app['buildPhases']]
def files(kind):
    result = []
    for phase in phases:
        if phase['isa'] == kind:
            for key in phase.get('files', []):
                ref = objects[objects[key]['fileRef']]
                result.append(pathlib.PurePosixPath(ref.get('path', ref.get('name', ''))).name)
    return result
sources = files('PBXSourcesBuildPhase')
for required in ['main.cpp', 'i960_native_stub.cpp', 'gen_none.cpp', 'd3d11_map.cpp',
                 'VF2Platform.mm', 'VF2ROMImport.mm', 'VF2InputBridge.mm', 'gpu3d_metal.mm']:
    assert sources.count(required) == 1, (required, sources.count(required))
for forbidden in ['i960_native.cpp', 'sljitLir.c', 'gpu3d_sdlgpu.cpp']:
    assert forbidden not in sources, forbidden
assert 'assets' in files('PBXResourcesBuildPhase'), 'Missing bundled menu/fonts/backgrounds'
for name in ['Debug.xcconfig', 'Release.xcconfig']:
    assert any(v.get('path', '').endswith(name) for v in objects.values()), name
assert 'VF2_DISABLE_JIT=1' in (root / 'Config/Debug.xcconfig').read_text()
assert 'Debug.xcconfig' in (root / 'Config/Release.xcconfig').read_text()
print('Generated source selection, resource copy and interpreter configuration: OK')
