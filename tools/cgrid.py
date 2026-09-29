import subprocess, re, os, sys
from concurrent.futures import ThreadPoolExecutor
EXE=r"C:\dev\cosmos-x1\build\cosmos_x1_dev.exe"
base={}
C={"plastic_budget":1.5,"learning_rate":0.02,"presynaptic_bound":1,"covariance":1}
combos={
 "C+lr16":{**C,"long_range_links":16,"long_range":0.0082},
 "C+lr32":{**C,"long_range_links":32,"long_range":0.0041},
 "mid+lr32":{"long_range_links":32,"long_range":0.0041},
 "C+lr64":{**C,"long_range_links":64,"long_range":0.00205},
 "C+lr32b2.5":{**C,"long_range_links":32,"long_range":0.0041,"plastic_budget":2.5},
 "C+lr64b3":{**C,"long_range_links":64,"long_range":0.00205,"plastic_budget":3},
 "WH":{"long_range_links":64,"long_range":0.00205,"presynaptic_bound":1,"covariance":1},
 "WH+b.8":{"long_range_links":64,"long_range":0.00205,"presynaptic_bound":1,"covariance":1,"plastic_budget":0.8,"learning_rate":0.012},
 "C+lr64b2":{**C,"long_range_links":64,"long_range":0.00205,"plastic_budget":2},
 "C+rate.04":{**C,"learning_rate":0.04},
 "C+budget2.5":{**C,"plastic_budget":2.5},
 "C+b2.5r.04":{**C,"plastic_budget":2.5,"learning_rate":0.04},
 "C+fat.03":{**C,"fatigue_gain3":0.03},
 "C+norelax":{**C,"agc_relax_field":0},
 "C+mode60":{**C,"mode_tau":60},
 "C+supp.4":{**C,"encoding_suppression":0.4},
 "middle-only":{},
 "strong+pre1+cov1":{"plastic_budget":1.5,"learning_rate":0.02,"presynaptic_bound":1,"covariance":1},
 "strong+pre1":{"plastic_budget":1.5,"learning_rate":0.02,"presynaptic_bound":1},
 "pre1+cov1":{"presynaptic_bound":1,"covariance":1},
 "pre1+pred1":{"presynaptic_bound":1,"predictive":1},
 "all3":{"presynaptic_bound":1,"covariance":1,"predictive":1},
 "pre1":{"presynaptic_bound":1},
}
if len(sys.argv)>1: combos={k:v for k,v in combos.items() if k in sys.argv[1:]}
seeds=[int(x) for x in os.environ['SL'].split()] if 'SL' in os.environ else range(int(os.environ.get("S0",22)),int(os.environ.get("S1",30))); preset=os.environ.get("PRESET","small")
def run(job):
    name,sd=job; args=[EXE,"--test","completion","--preset",preset,"--seed",str(sd)]
    for k,v in {**base,**combos[name]}.items(): args+=["--set",f"{k}={v}"]
    out=subprocess.run(args,capture_output=True,text=True,env=dict(os.environ,OMP_NUM_THREADS="4")).stdout
    m=re.search(r"learned margin ([+-][0-9.]+) \(identifies all: (\w+)\).*gain ([+-][0-9.]+).*: (PASS|FAIL)",out)
    return name,sd,m.groups() if m else ("?","?","?","?")
jobs=[(n,s) for n in combos for s in seeds]
res={}
with ThreadPoolExecutor(3) as p:
    for n,s,g in p.map(run,jobs): res.setdefault(n,[]).append((s,g))
for n,r in res.items():
    passes=sum(g[3]=="PASS" for _,g in r)
    print(f"{n:11s} {passes}/{len(r)} pass | "+" ".join(f"s{s}:{g[2]}{'' if g[1]=='yes' else '!'}" for s,g in r),flush=True)
