"""Copy original DDS bytes to portable aliases; no image resampling or recoloring."""
from pathlib import Path
import hashlib
import json
import shutil

root=Path(__file__).resolve().parents[1]
original=root/'assets/textures/hud/beatup'
target=root/'assets/hud_clean'
decoded={}
for path in original.glob('*.dds'):
    try: name=path.stem.encode('latin1').decode('cp949')
    except (UnicodeError,LookupError): name=path.stem
    decoded[name]=path
mapping={
    'gauge_up_blue':'up_1',
    'gauge_flame_orange':'게이지바탕불꽃',
    'gauge_flame_blue':'게이지바탕불꽃_1',
    'gauge_letter_orange':'글자_불꽃',
    'gauge_letter_blue':'글자_불꽃_1',
}
for letter in 'beatup': mapping['gauge_letter_'+letter]='글자_'+letter
for bank,color in [(2,'yellow'),(3,'purple')]:
    for number in range(10): mapping[f'combo_{color}_{number}']=f'콤보_{bank}_{number}'
manifest=[]
for alias,name in mapping.items():
    source=decoded[name]
    destination=target/(alias+'.dds')
    shutil.copyfile(source,destination)
    manifest.append({'alias':alias+'.dds','source':str(source.relative_to(root)),
                     'sha256':hashlib.sha256(source.read_bytes()).hexdigest()})
(target/'combo_assets.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
print(f'Prepared {len(manifest)} original combo textures')
