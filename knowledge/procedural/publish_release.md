---
generated_by: "OpenAI Codex (GPT-6)"
timestamp: "2026-09-28T15:36:00-05:00"
---
# Publish and verify a Pebble release

1. Confirm `watchface/package.json` has the intended version and UUID. Clean/build from `watchface/` with `pebble clean && pebble build`.
2. Run `python3 execution/publish_release.py --dry-run` and inspect its version, description, release notes, and four named Store screenshots.
3. Publish the existing app with `python3 execution/publish_release.py`. A successful upload reports the resolved app ID and screenshots uploaded; it does not prove public visibility.
4. Open the existing public listing and copy its versioned `/api/assets/pbw/<app-id>/<version>/<filename>.pbw` URL. Run `/Users/danielbally/.local/share/uv/tools/pebble-tool/bin/python execution/verify_release.py --public-pbw <url>` to compare public PBW bytes and package metadata with the local build.
5. Publish the GitHub release with the same `watchface/build/watchface.pbw`; compare its asset SHA-256 to the Store verifier output.
6. Inspect the rendered public description and screenshots separately. The publisher portal may be signed into an account that does not manage an existing listing; `pebble publish` can update a release without changing the app description. If so, recover/link the listing owner before declaring Store copy accurate.

For Constellation 0.1.2, the public PBW matched local SHA-256 `089dd91fd107e26f57c3a3a336d197dca8be93f9c8d97e9366a34700c7b489be`; listing description edit remains pending publisher-account linkage.


## Historical release-note correction — 2026-09-28
Authenticated dashboard at `https://developer.repebble.com/dashboard` is the linked Daniel Bally account and lists Constellation. Release-history `Edit` controls save corrected notes and those edits persist after dashboard reload. The public changelog still displayed old 0.1.0/0.1.1 text after reload and cache-busted navigation; keep this separate from the current corrected listing description and do not claim propagation until verified publicly.


## Publisher-account correction — 2026-09-28
The prior account-mismatch note is superseded: the linked dashboard session identified Daniel Bally and listed Constellation and Satellite Overpass. The corrected current description is saved and publicly verified. Historical changelog edits remain a separate public-propagation gap.
