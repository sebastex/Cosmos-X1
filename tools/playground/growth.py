"""Collect every playground lane's growth log (newest pg-log-<variant>-<seed> artifact) into one json.

    python tools/playground/growth.py > growth.json
"""
import json
import os
import subprocess
import sys
import tempfile
import zipfile

REPO = 'sebastex/Cosmos-X1'


def main():
    arts = []
    for line in subprocess.check_output(['gh', 'api', '--paginate', f'repos/{REPO}/actions/artifacts?per_page=100',
                                         '--jq', '[.artifacts[] | select(.name | startswith("pg-log-")) | select(.expired | not)]'],
                                        text=True).splitlines():  # one json list per page
        if line.strip():
            arts += json.loads(line)
    newest = {}
    for a in arts:
        if a['name'] not in newest or a['created_at'] > newest[a['name']]['created_at']:
            newest[a['name']] = a
    lanes = []
    for name, a in sorted(newest.items()):
        with tempfile.TemporaryDirectory() as d:
            z = os.path.join(d, 'a.zip')
            with open(z, 'wb') as f:
                f.write(subprocess.check_output(['gh', 'api', f"repos/{REPO}/actions/artifacts/{a['id']}/zip"]))
            with zipfile.ZipFile(z) as zf:
                if 'growth.json' in zf.namelist():
                    lanes.append(json.loads(zf.read('growth.json')))
    print(json.dumps(dict(lanes=lanes)))


if __name__ == '__main__':
    sys.exit(main())
