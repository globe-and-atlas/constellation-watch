# Errors

Record deterministic errors, root causes, and fixes here.

- 2026-09-28: `cat watchface/appinfo.json` failed because this project stores Pebble metadata in `watchface/package.json`; use the package manifest as the authoritative version/message-key source. Graduated to `knowledge/domain/release_metadata.md` (to be added after this release work is verified).
