"""Bundle a tested generic build with its exact sources, dependencies and notices.

Writes outside the repository; never reads device configuration or user secrets.
"""
import argparse, hashlib, io, json, re, subprocess, tarfile
from pathlib import Path
import esphome

ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def allowed(member):
    parts=Path(member.name).parts
    if any(p in ('.git','__pycache__','.DS_Store','build','node_modules') or p.startswith('.esphome') for p in parts):return None
    if member.name.endswith(('.pyc','.log')):return None
    if member.issym() or member.islnk():
        if Path(member.linkname).is_absolute() or '..' in Path(member.linkname).parts:raise ValueError('External source symlink')
    member.uid=member.gid=0;member.uname=member.gname='';return member

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--version',required=True);parser.add_argument('--build',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    if not re.fullmatch(r'(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)',args.version):parser.error('Numeric version required')
    if subprocess.check_output(['git','status','--porcelain'],cwd=ROOT,text=True):parser.error('Commit the exact public source before packaging')
    out=args.output.resolve()
    if out==ROOT or ROOT in out.parents:parser.error('Release output must be outside the source repository')
    out.mkdir(parents=True,exist_ok=False)
    build=args.build.resolve();description=json.loads((build/'build/project_description.json').read_text());idf=Path(description['idf_path']);esp=Path(esphome.__file__).resolve().parent
    if description['target']!='esp32':parser.error('Wrong hardware target')
    maincpp=(build/'src/main.cpp').read_text()
    defines=(build/'src/esphome/core/defines.h').read_text()
    if f'"{args.version}"' not in defines or 'firmware_channel("testing")' not in maincpp:parser.error('Build must match version and test channel')
    for forbidden in ['home-oxolon.de','DE_RO_AK3','192.168.21.5','mqtt2fritz']:
        if forbidden in maincpp:parser.error('Private defaults in generated firmware')
    commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    for name in ['firmware.factory.bin','firmware.ota.bin']:
        (out/name).write_bytes((build/'build'/name).read_bytes())
    licenses=out/'LICENSES';licenses.mkdir()
    roots={'esp-idf':idf,'esphome':esp,'managed':build/'managed_components'}
    for name,base in roots.items():
        for p in base.rglob('*'):
            if p.is_file() and p.name.lower().startswith(('license','copying','notice')) and '.git' not in p.parts:
                dst=licenses/name/p.relative_to(base);dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(p.read_bytes())
    for p in [ROOT/'LICENSE',Path(esphome.__file__).parent.parent/'esphome-2026.10.1.dist-info/licenses/LICENSE',idf/'docs/en/COPYRIGHT.rst']:
        if not p.exists():raise ValueError('Missing required license source: '+p.name)
        (licenses/('ESPHome-LICENSE' if 'dist-info' in str(p) else 'ESP-IDF-COPYRIGHT.rst' if p.suffix=='.rst' else 'FRITZ2MQTT-LICENSE')).write_bytes(p.read_bytes())
    metadata={'version':args.version,'channel':'testing','hardware_profile':'esp32-4mb-v1','source_commit':commit,'esphome_version':'2026.10.1','idf_version':'5.5.6','components':description['build_components'],'build_command':f'esphome -s firmware_version {args.version} compile firmware/esp32-test.yaml','factory_sha256':sha(out/'firmware.factory.bin'),'ota_sha256':sha(out/'firmware.ota.bin')}
    (out/'build.json').write_text(json.dumps(metadata,indent=2)+'\n');(out/'dependencies.lock').write_bytes((build/'dependencies.lock').read_bytes())
    (out/'BUILD.md').write_text('''# Corresponding build sources

This package is an OTA test prerelease, not a stable release.

`corresponding-sources.tar.gz` contains the exact committed FRITZ2MQTT source,
the ESPHome source used for generation, generated C++/configuration, managed
components and ESP-IDF SDK with their original notices. Root licenses are also
provided separately under LICENSES. Unmodified SDK components retain their own
licenses; the project GPL does not replace those notices.

To rebuild: use Python 3.12, install esphome==2026.10.1, check out source_commit
from build.json and execute build_command. Compiler/SDK tools are obtained by
ESPHome. dependencies.lock records the resolved managed versions.

Factory file: initial USB installation at address 0. OTA file: application only.
Do not put personal settings into the build; configure them on the device.
''')
    archive=subprocess.check_output(['git','archive','--format=tar','--prefix=fritz2mqtt/',commit],cwd=ROOT)
    with tarfile.open(out/'corresponding-sources.tar.gz','w:gz',compresslevel=1) as tar:
        with tarfile.open(fileobj=io.BytesIO(archive)) as own:
            for member in own:
                data=own.extractfile(member) if member.isfile() else None;tar.addfile(member,data)
        for name,base in roots.items():tar.add(base,arcname=name,filter=allowed)
        tar.add(build/'src',arcname='generated/src',filter=allowed)
        for name in ['CMakeLists.txt','partitions.csv','sdkconfig.fritz2mqtt','dependencies.lock']:
            tar.add(build/name,arcname='generated/'+name,filter=allowed)
        tar.add(out/'BUILD.md',arcname='BUILD.md')
    with tarfile.open(out/'corresponding-sources.tar.gz') as tar:
        for member in tar:
            if member.name.startswith('/') or '..' in Path(member.name).parts:raise ValueError('Unsafe archive member')
    with tarfile.open(out/'licenses.tar.gz','w:gz') as tar:tar.add(licenses,arcname='LICENSES')
    md5=hashlib.md5((out/'firmware.ota.bin').read_bytes()).hexdigest()
    manifest={'name':'FRITZ2MQTT · OTA-Testkanal','version':args.version,'builds':[{'chipFamily':'ESP32','ota':{'md5':md5,'path':f'https://m-kaufmann.github.io/fritz2mqtt/firmware/esp32-4mb-v1/{args.version}/firmware.ota.bin','release_url':f'https://github.com/m-kaufmann/fritz2mqtt/releases/tag/v{args.version}-test.1','summary':'OTA-Testversion; keine allgemeine stabile Freigabe'}}]}
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n')
    files=[p for p in out.iterdir() if p.is_file()];(out/'SHA256SUMS.txt').write_text(''.join(f'{sha(p)}  {p.name}\n' for p in sorted(files)))
    print(json.dumps({'output':str(out),'source_commit':commit,'source_archive_mb':round((out/'corresponding-sources.tar.gz').stat().st_size/1048576,1),'license_files':sum(p.is_file() for p in licenses.rglob('*'))}))
if __name__=='__main__':main()
