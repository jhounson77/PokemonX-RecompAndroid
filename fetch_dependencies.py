from pathlib import Path
from concurrent.futures import ThreadPoolExecutor,as_completed
import configparser,json,urllib.request,tarfile,io
root=Path(__file__).resolve().parent/'azahar'
tree=json.loads(root.with_name('azahar-tree.json').read_text())['tree']
links={x['path']:x['sha'] for x in tree if x['mode']=='160000'}
cfg=configparser.ConfigParser();cfg.read(root/'.gitmodules')
selected=[]
skip={'dist/compatibility_list','externals/catch2','externals/discord-rpc','externals/openal-soft','externals/sdl2/SDL','externals/cubeb','externals/libretro-common/libretro-common','externals/dllwalker','externals/libusb/libusb'}
for section in cfg.sections():
 path=cfg[section]['path'];repo=cfg[section]['url'].removeprefix('https://github.com/').removesuffix('.git')
 if path in links and path not in skip:selected.append((path,repo,links[path]))
def fetch(item):
 path,repo,sha=item;dest=root/path
 if (dest/'.source-revision').exists():return path+' cached'
 data=urllib.request.urlopen('https://codeload.github.com/'+repo+'/tar.gz/'+sha,timeout=120).read()
 with tarfile.open(fileobj=io.BytesIO(data),mode='r:gz') as tar:
  members=tar.getmembers();prefix=members[0].name.split('/')[0]+'/'
  for m in members:
   if not m.name.startswith(prefix):continue
   m.name=m.name[len(prefix):]
   if m.name:tar.extract(m,dest,filter='data')
 (dest/'.source-revision').write_text(repo+' '+sha+'\n');return path+' downloaded'
with ThreadPoolExecutor(max_workers=5) as pool:
 for f in as_completed([pool.submit(fetch,x) for x in selected]):
  try:print(f.result(),flush=True)
  except Exception as e:print('ERROR',repr(e),flush=True);raise
