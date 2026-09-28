"""Publish Constellation to the authenticated RePebble Appstore."""
import argparse
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()

    package = json.loads((ROOT / 'watchface/package.json').read_text())
    store = ROOT / 'store'

    screenshots = [
        str(store / 'emery_orbit.png'),
        str(store / 'emery_skyplot.png'),
        str(store / 'emery_geodesy.png'),
        str(store / 'emery_ground_track.png'),
    ]

    command = [
        'pebble', 'publish', '--non-interactive', '--is-published', '--no-gif-all-platforms',
        '--name', 'Constellation',
        '--version', package['version'],
        '--description', (store / 'description.txt').read_text().strip(),
        '--release-notes', (store / 'release-notes.txt').read_text().strip(),
        '--source', 'https://github.com/globe-and-atlas/constellation-watch',
        '--category', 'tools-utilities',
        '--icon-small', str(store / 'icon-small.png'),
        '--icon-large', str(store / 'icon-large.png'),
        '--replace-screenshots',
        '--screenshots', *screenshots,
    ]

    print(f'Constellation {package["version"]} ready to publish.')
    if args.dry_run:
        print('Dry run command:')
        print(' '.join(command))
        return

    subprocess.run(command, cwd=ROOT / 'watchface', check=True)


if __name__ == '__main__':
    main()
