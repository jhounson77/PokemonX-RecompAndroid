from pathlib import Path
exec(Path(__file__).with_name('fetch_dependencies.py').read_text().split('with ThreadPoolExecutor')[0])
fetch(('dist/compatibility_list','azahar-emu/compatibility-list','d282515cd6a66b694ac0657eef250c24b7410495'))
repo='azahar-emu/dynarmic';sha=links['externals/dynarmic']
t=json.load(urllib.request.urlopen('https://api.github.com/repos/'+repo+'/git/trees/'+sha+'?recursive=1'))
nested={x['path']:x['sha'] for x in t['tree'] if x['mode']=='160000'}
c=configparser.ConfigParser();c.read(root/'externals/dynarmic/.gitmodules')
for sec in c.sections():
 p=c[sec]['path'];repo=c[sec]['url'].removeprefix('https://github.com/').removesuffix('.git')
 if p in nested and p.split('/')[-1] in {'mcl','robin-map','zydis','zycore','biscuit'}:print(fetch(('externals/dynarmic/'+p,repo,nested[p])),flush=True)

print(fetch(('externals/libadrenotools/lib/linkernsbypass','bylaws/liblinkernsbypass','aa3975893d83ef1bc84c321ec60c65fbf1287887')),flush=True)
