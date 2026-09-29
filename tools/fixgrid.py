import subprocess, re, os, sys, json
from concurrent.futures import ThreadPoolExecutor
EXE=r"C:\dev\cosmos-x1\build\cosmos_x1_diag.exe"
FULL={"long_range_links":64,"long_range":0.00205,"presynaptic_bound":1,"covariance":1,"plastic_budget":2,"learning_rate":0.02}
V=json.loads(os.environ["VARIANTS"])
tests=[(t,int(s)) for t,s in (x.split(":") for x in os.environ.get("TESTS","capacity:22 completion:22 completion:23 completion:33").split())]
def run(job):
    name,(test,sd)=job; args=[EXE,"--test",test,"--preset",os.environ.get("PRESET","small"),"--seed",str(sd)]
    for k,v in {**FULL,**V[name]}.items(): args+=["--set",f"{k}={v}"]
    out=subprocess.run(args,capture_output=True,text=True,env=dict(os.environ,OMP_NUM_THREADS="4")).stdout
    m=re.findall(r"identifies all: (\w+)\).*gain ([+-][0-9.]+).*: (PASS|FAIL)",out)
    g=m[-1] if m else ("?","?","?")
    return name,f"{test[:4]}{sd}:{g[1]}{'' if g[0]=='yes' else '!'}{'' if g[2]=='PASS' else 'x'}"
jobs=[(n,t) for n in V for t in tests]
res={}
with ThreadPoolExecutor(3) as p:
    for n,r in p.map(run,jobs): res.setdefault(n,[]).append(r)
for n,r in res.items(): print(f"{n:14s} "+"  ".join(r),flush=True)
