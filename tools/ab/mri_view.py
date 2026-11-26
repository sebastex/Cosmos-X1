"""MRI viewer: turns the SCAN lines of a mind test (feat/mri) into one HTML page - per heard word, an activity
movie as a heatmap (rows = the 256 regions, 64 per field: Input, Memory, Reasoning, Output; columns = ticks),
BOLD-style (activity convolved with a slow blood-flow response, the canonical double-gamma, 1 tick = 0.1 s), plus
how alike the words' patterns are per field (representational similarity) and the MIND markers.
    python tools/ab/mri_view.py RESULT_TXT [RESULT_TXT_CONTROL] OUT.html
"""
import json
import math
import sys

FIELDS = ['Input', 'Memory', 'Reasoning', 'Output']


def rows(path, key):
    return [json.loads(l.split(' ', 1)[1]) for l in open(path, errors='replace') if l.startswith(key + ' ')]


def hrf(dt=0.1, n=200):
    g = lambda t, k: t ** (k - 1) * math.exp(-t) / math.gamma(k) if t > 0 else 0.0
    h = [g(i * dt, 6) - g(i * dt, 16) / 6 for i in range(n)]
    s = sum(h)
    return [x / s for x in h]


def bold(series, h):
    out = []
    for t in range(len(series)):
        out.append(sum(series[t - k] * h[k] for k in range(min(len(h), t + 1))))
    return out


def corr(a, b):
    ma, mb = sum(a) / len(a), sum(b) / len(b)
    va = sum((x - ma) ** 2 for x in a)
    vb = sum((x - mb) ** 2 for x in b)
    return sum((x - ma) * (y - mb) for x, y in zip(a, b)) / math.sqrt(va * vb) if va > 0 and vb > 0 else 0.0


def scans(path):
    by = {}
    for r in rows(path, 'SCAN'):
        by.setdefault(r['word'], []).append(r)
    return by


def page(path, control, out):
    h = hrf()
    parts = []
    css = ('body{font-family:system-ui,sans-serif;background:#0f1115;color:#e6e6e6;margin:16px}h2{margin-top:28px}'
           '.grid{display:flex;flex-wrap:wrap;gap:18px}.card{background:#181b22;border-radius:8px;padding:10px}'
           'canvas{image-rendering:pixelated;border:1px solid #333}td,th{padding:3px 8px;text-align:right}'
           'th{color:#9aa}small{color:#9aa}')
    data = {}
    for label, p in [('trained', path)] + ([('untrained', control)] if control else []):
        sc = scans(p)
        words = {}
        for w, rs in sc.items():
            mat = [r['r'] for r in rs]  # ticks x 256
            T = len(mat)
            regions = len(mat[0]) if mat else 0
            b = [bold([mat[t][k] for t in range(T)], h) for k in range(regions)]
            words[w] = {'raw': [[mat[t][k] for t in range(T)] for k in range(regions)], 'bold': b,
                        'input': [r['input'] for r in rs]}
        rsa = {}
        ws = list(words)
        for fi, fname in enumerate(FIELDS):
            m = []
            for a in ws:
                ra = [sum(words[a]['raw'][k]) for k in range(fi * 64, fi * 64 + 64)]
                m.append([round(corr(ra, [sum(words[b]['raw'][k]) for k in range(fi * 64, fi * 64 + 64)]), 2) for b in ws])
            rsa[fname] = m
        mind = rows(p, 'MIND')
        data[label] = {'words': words, 'rsa': rsa, 'order': ws, 'mind': mind[0] if mind else {}}
    html = ['<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">'
            '<title>Cosmos MRI</title><style>' + css + '</style></head><body>',
            '<h1>Cosmos MRI</h1><p><small>Rows: 256 regions (4x4x4 voxel blocks), 64 per field from top: Input, Memory, '
            'Reasoning, Output. Columns: ticks; the white bar marks while the word is heard. Left: raw active voxels; '
            'right: BOLD-style (convolved with the canonical double-gamma blood-flow response).</small></p>']
    for label, d in data.items():
        html.append(f'<h2>{label} brain</h2><div class="grid">')
        for w in d['order']:
            html.append(f'<div class="card"><b>{w}</b><br><canvas id="{label}_{w}_raw"></canvas> <canvas id="{label}_{w}_bold"></canvas></div>')
        html.append('</div><h3>How alike the words look, per field (correlation of region patterns)</h3><div class="grid">')
        for fname, m in d['rsa'].items():
            tbl = '<table><tr><th></th>' + ''.join(f'<th>{w}</th>' for w in d['order']) + '</tr>'
            for w, row in zip(d['order'], m):
                tbl += f'<tr><th>{w}</th>' + ''.join(f'<td>{x:.2f}</td>' for x in row) + '</tr>'
            html.append(f'<div class="card"><b>{fname}</b>{tbl}</table></div>')
        html.append('</div>')
        if d['mind']:
            html.append('<h3>Consciousness markers</h3><pre>' + json.dumps(d['mind'], indent=1) + '</pre>')
    js = '''<script>const D=%s;
function draw(id,m,inp,scale){const c=document.getElementById(id);if(!c)return;const R=m.length,T=m[0].length,s=3;
c.width=T*s;c.height=R*s+6;const x=c.getContext('2d');let mx=1e-9;for(const r of m)for(const v of r)mx=Math.max(mx,v);
for(let k=0;k<R;k++)for(let t=0;t<T;t++){const v=m[k][t]/mx;x.fillStyle=`hsl(${240-200*v},90%%,${8+55*v}%%)`;x.fillRect(t*s,k*s+6,s,s);}
x.fillStyle='#fff';for(let t=0;t<T;t++)if(inp[t])x.fillRect(t*s,0,s,4);x.fillStyle='#555';for(let f=1;f<4;f++)x.fillRect(0,6+f*64*s,T*s,1);}
for(const [lab,d] of Object.entries(D))for(const [w,v] of Object.entries(d.words)){draw(lab+'_'+w+'_raw',v.raw,v.input);draw(lab+'_'+w+'_bold',v.bold,v.input);}
</script>''' % json.dumps({k: {'words': v['words']} for k, v in data.items()})
    html.append(js + '</body></html>')
    open(out, 'w', encoding='utf-8').write('\n'.join(html))
    print('wrote', out)


if __name__ == '__main__':
    a = sys.argv[1:]
    page(a[0], a[1] if len(a) > 2 else None, a[-1])
