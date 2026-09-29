import subprocess, re, os, sys, json
from concurrent.futures import ThreadPoolExecutor
EXE=r"C:\dev\cosmos-x1\build\cosmos_x1_diag.exe"
FULL=json.loads(os.environ.get("BASE","{}"))
V=json.loads(os.environ["VARIANTS"])
tests=[(t,int(s)) for t,s in (x.split(":") for x in os.environ.get("TESTS","capacity:22 completion:22 completion:23 completion:33").split())]
def run(job):
    name,(test,sd)=job; args=[EXE,"--test",test,"--preset",os.environ.get("PRESET","small"),"--seed",str(sd)]
    for k,v in {**FULL,**V[name]}.items(): args+=["--set",f"{k}={v}"]
    out=subprocess.run(args,capture_output=True,text=True,env=dict(os.environ,OMP_NUM_THREADS="4")).stdout
    if test=="continual":
        f=re.search(r"forgetting of old memories: ([+-][0-9.]+).*: (PASS|FAIL)",out)
        n=re.search(r"new memories: .*identifies all: (\w+)\).*: (PASS|FAIL)",out)
        ok=f and f.group(2)=="PASS" and n and n.group(2)=="PASS"
        return name,f"cont{sd}:forget{f.group(1) if f else '?'}{'' if (n and n.group(1)=='yes') else '!'}{'' if ok else 'x'}"
    m=re.findall(r"identifies all: (\w+)\).*gain ([+-][0-9.]+).*: (PASS|FAIL)",out)
    g=m[-1] if m else ("?","?","?")
    return name,f"{test[:4]}{sd}:{g[1]}{'' if g[0]=='yes' else '!'}{'' if g[2]=='PASS' else 'x'}"
jobs=[(n,t) for n in V for t in tests]
res={}
with ThreadPoolExecutor(3) as p:
    for n,r in p.map(run,jobs): res.setdefault(n,[]).append(r)
for n,r in res.items(): print(f"{n:14s} "+"  ".join(r),flush=True)
