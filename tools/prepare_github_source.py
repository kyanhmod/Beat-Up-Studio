"""Create a clean upload folder and ZIP, using an explicit source-file allowlist."""
from pathlib import Path
import shutil,zipfile
root=Path(__file__).resolve().parents[1];out=root/'github-ready';out.mkdir(exist_ok=True)
stage=out/'BeatUpStudio';stage.mkdir(exist_ok=True)
files=[root/n for n in ('build.bat','README.md','THIRD_PARTY_NOTICES.md','.gitignore','.gitattributes','requirements-assets.txt','assets/default.pack')]
for folder,suffixes in [('src',{'.h','.cpp','.c'}),('tests',{'.cpp','.bat'}),('tools',{'.py','.bat'}),('docs',{'.md','.json'})]:
    files.extend(p for p in (root/folder).glob('*') if p.is_file() and p.suffix in suffixes)
for p in files:
    target=stage/p.relative_to(root);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
archive=out/'BeatUpStudio-source.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted(files):z.write(stage/p.relative_to(root),Path('BeatUpStudio')/p.relative_to(root))
print(f'Prepared {len(files)} files: {archive} ({archive.stat().st_size:,} bytes)')
