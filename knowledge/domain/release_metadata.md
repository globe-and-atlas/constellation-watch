---
generated_by: "OpenAI Codex (GPT-6)"
timestamp: "2026-09-28T15:31:00-05:00"
---
# Release metadata

`watchface/package.json` is the source of truth for the Pebble app UUID, version, capabilities, target platform, and AppMessage keys. This project does not use `watchface/appinfo.json`. After changing message keys or their types, run `pebble clean` before `pebble build` so generated headers cannot remain stale.

`execution/verify_release.py` uses the package UUID to resolve the corresponding public RePebble listing and can compare the downloaded PBW digest with a local artifact. HTTP success alone proves reachability, not that Store text, screenshots, or package bytes changed.
