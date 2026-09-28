# Errors

Record deterministic errors, root causes, and fixes here.

- 2026-09-28: `cat watchface/appinfo.json` failed because this project stores Pebble metadata in `watchface/package.json`; use the package manifest as the authoritative version/message-key source. Graduated to `knowledge/domain/release_metadata.md` (to be added after this release work is verified).
- 2026-09-28: A multi-file patch was rejected because it included `watchface/src/c/main.c` twice; no changes were applied. Split edits by file and verify the resulting diff.
- 2026-09-28: Initial validation exposed stale generated Pebble message-key headers because the build was incremental; use `pebble clean` before validating package message-key changes. Also, the planned JS test directory did not yet exist; create `watchface/test/` before writing test files.
- 2026-09-28: First Node test invocation used a root-relative test path from `watchface/`, and the corrected invocation then imported PebbleKit JS without stubbing `Pebble`. The test now needs to install the platform event stub before requiring app code; run repository-level pytest from project root.
- 2026-09-28: New copy test treated any mention of signals as a defect, even though README correctly says this app does not receive GNSS signals. Narrow assertion to check that elevation bars are not described as signal strength.
- 2026-09-28: `pebble clean` was invoked from the repository root instead of `watchface/`, so the SDK could not find the project manifest. Run Pebble CLI commands from the watchface directory; unit suites passed before this path error.
- 2026-09-28: A manual emulator screenshot used `.tmp/emulator/...` relative to `watchface/`, where that directory does not exist; button actions ran, but captures were not saved. Use the script's absolute `.tmp/emulator` output path and verify files exist before inspection.
- 2026-09-28: Attempted to run Overpass's `execution/release_store.py` from Constellation, where that helper does not exist. Add/use a project-local verifier for Constellation's public PBW and app UUID rather than assuming scripts transfer between checkouts.
- 2026-09-28: The fresh emulator-capture run terminated during stale-process cleanup before rebuilding or capturing all panes; the broad `pkill -f` pattern can match its invoking command. Replacing it with exact-process matching and rerunning the clean capture before release.
- 2026-09-28: Final Constellation regression still expected the pre-layout `ELEVATION ONLY` label after the visible label was shortened to `ELEVATION` to avoid clipping; updating the assertion to the final UI string. Ruff also flags the shebang-bearing emulator helper as non-executable; setting its executable bit.
- 2026-09-28: Pebble CLI has no `whoami` subcommand; it returned usage and exposed no account identity. Use app UUID resolution plus the public release verifier for target verification.
- 2026-09-28: Workspace `session_capture.py` was started as required but remained blocked in its remote LLM capture call for over three minutes with no output; terminated the hung process. Session facts are recorded directly in this project's SESSION.md.
