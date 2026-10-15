"""Use the original Korean-named countdown DDS files without resampling."""
from pathlib import Path
import shutil
import json
import hashlib
root = Path(__file__).resolve().parents[1]
manifest = []
for source in (root / 'assets/textures/hud/beatup').glob('*.dds'):
    try:
        name = source.stem.encode('latin1').decode('cp949')
    except UnicodeError:
        continue
    if name in {'카운트_1', '카운트_2', '카운트_3'}:
        target = root / 'assets/hud_clean' / ('countdown_' + name[-1] + '.dds')
        shutil.copyfile(source, target)
        manifest.append(dict(alias=target.name, source=str(source.relative_to(root)),
                             sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
if len(manifest) != 3:
    raise SystemExit('Missing original countdown texture')
(root / 'assets/hud_clean/intro_assets.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding='utf-8')
print('Prepared three original countdown textures')
