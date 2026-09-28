"""Verify that the authenticated RePebble account recognizes this UUID and PBW."""
from __future__ import annotations

import argparse
import hashlib
import io
import json
import zipfile
from pathlib import Path

import requests
from pebble_tool.account import get_account
from pebble_tool.commands.publish import DEFAULT_APPSTORE_API_BASE, PublishCommand

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--public-pbw', help='PBW URL copied from the public listing page')
    args = parser.parse_args()
    package = json.loads((ROOT / 'watchface/package.json').read_text())
    account = get_account(auth_provider='firebase')
    if not account.is_logged_in:
        raise SystemExit('Pebble login required')
    context = PublishCommand._get_me_context(DEFAULT_APPSTORE_API_BASE, account.get_access_token())
    lookup = (context.get('app_lookup') or {}).get('by_app_uuid') or {}
    app_id = PublishCommand._lookup_app_id_case_insensitive(lookup, package['pebble']['uuid'])
    result = {'app_id': app_id, 'version': package['version']}
    if app_id:
        response = requests.get(f'https://apps.repebble.com/constellation_{app_id}', timeout=30)
        response.raise_for_status()
        result['public_http_status'] = response.status_code
    if args.public_pbw:
        prefix = f'{DEFAULT_APPSTORE_API_BASE}/api/assets/pbw/{app_id}/'
        if not app_id or not args.public_pbw.startswith(prefix):
            raise SystemExit('PBW URL does not belong to the account-matched app UUID')
        response = requests.get(args.public_pbw, timeout=30)
        response.raise_for_status()
        with zipfile.ZipFile(io.BytesIO(response.content)) as archive:
            metadata = json.loads(archive.read('appinfo.json'))
        if metadata['versionLabel'] != package['version'] or metadata['uuid'] != package['pebble']['uuid']:
            raise SystemExit('Public PBW metadata does not match the source package')
        local = (ROOT / 'watchface/build/watchface.pbw').read_bytes()
        if response.content != local:
            raise SystemExit('Published PBW differs from the local production build')
        result['published_pbw_sha256'] = hashlib.sha256(response.content).hexdigest()
        result['published_pbw_matches_local'] = True
    out = ROOT / '.tmp' / 'store-verification.json'
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
