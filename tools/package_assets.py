"""Prepare Windows RC resources from the single versioned original asset pack."""
from pathlib import Path
import hashlib,json
from asset_pack import read_pack
root=Path(__file__).resolve().parents[1]
source=root/'assets/default.pack';out=root/'build';out.mkdir(exist_ok=True)
assets=read_pack(source);manifest=json.loads(assets['asset_manifest.json'])
if manifest.get('skin')!='original-default':raise ValueError('Expected original default HUD')
for item in manifest['assets']:
    data=assets[item['name']]
    if len(data)!=item['bytes'] or hashlib.sha256(data).hexdigest()!=item['sha256']:raise ValueError('Resource hash mismatch: '+item['name'])
for name in ('a91.dds','crazy.dds','chance_active.dds','countdown_1.dds','BEAT.ogg','READY.ogg','START.ogg','app.ico'):
    if name not in assets:raise ValueError('Missing resource: '+name)
(out/'assets.pack').write_bytes(source.read_bytes())
(out/'studio.ico').write_bytes(assets['app.ico'])
(out/'asset_manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
(out/'studio.rc').write_text('101 ICON "studio.ico"\n201 RCDATA "assets.pack"\n',encoding='ascii')
print(f'Original default HUD + sound: {len(assets)} resources in {source.stat().st_size:,} bytes')
