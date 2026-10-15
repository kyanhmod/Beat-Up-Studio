"""Optional pack authoring from normalized default DDS filenames and OGG effects."""
import argparse,hashlib,io,json
from pathlib import Path
from asset_pack import write_pack

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--textures',type=Path,required=True)
    ap.add_argument('--sounds',type=Path,required=True)
    ap.add_argument('--chance-active',type=Path)
    ap.add_argument('--output',type=Path,default=Path('assets/default.pack'))
    args=ap.parse_args()
    from PIL import Image
    files={p.name:p for p in args.textures.glob('*.dds') if p.name not in {'bg_stage.dds','track_wide_l.dds','track_wide_r.dds'}}
    files['chance_active.dds']=args.chance_active or args.textures/'chance_active.dds'
    for name in ('BEAT','SPACE','GREAT','MISS','READY','START'):files[name+'.ogg']=args.sounds/(name+'.ogg')
    assets={name:p.read_bytes() for name,p in files.items()}
    icon=Image.open(io.BytesIO(assets['a91.dds'])).convert('RGBA');buffer=io.BytesIO()
    icon.save(buffer,format='ICO',sizes=[(16,16),(24,24),(32,32),(48,48),(64,64)])
    assets['app.ico']=buffer.getvalue()
    manifest={'skin':'original-default','assets':[{'name':name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()} for name,data in sorted(assets.items())]}
    assets['asset_manifest.json']=json.dumps(manifest,ensure_ascii=False,indent=2).encode('utf-8')
    write_pack(args.output,assets)
    print(f'Wrote {args.output}: {len(assets)} resources, {args.output.stat().st_size:,} bytes')
if __name__=='__main__':main()
