# Release Pipeline

**Version:** 0.1.0
**Status:** Active
**Date:** 2026-05-11

How the firmware gets from `git tag` to a published download on
[`anchor-drag-pro-releases`](https://github.com/Temple-of-Epiphany/anchor-drag-pro-releases).

## Overview

```
git tag v0.2.1 && git push --tags
              │
              ▼
   .github/workflows/release.yml
              │
              ├── checkout firmware repo
              ├── espressif/esp-idf-ci-action build (IDF 5.5, target esp32s3)
              ├── stage anchor-drag-pro_v0.2.1.bin + .sha256
              ├── extract release notes from CHANGELOG.md section [v0.2.1]
              └── softprops/action-gh-release publishes to
                    anchor-drag-pro-releases as a tagged release
                    (using RELEASES_REPO_TOKEN secret)
              │
              ▼
   https://github.com/Temple-of-Epiphany/anchor-drag-pro-releases/releases
              │
              ▼
   Customer downloads .bin + .sha256, copies to SD /firmware/,
   device prompts + flashes + auto-rollback on failure (Phase 2D).
```

## One-time setup — required before the workflow can run

The release workflow needs a fine-grained Personal Access Token (PAT) with
write access to the `anchor-drag-pro-releases` repo. The default
`GITHUB_TOKEN` only has access to the current repo, which is why a separate
PAT is required for cross-repo publishing.

### Step 1 — Create the PAT

1. Visit https://github.com/settings/personal-access-tokens (fine-grained tokens, not the legacy "classic" tokens)
2. **Generate new token**
3. Name: `anchor-drag-pro-releases publisher`
4. Expiration: 1 year (re-rotate when it expires)
5. **Repository access:** Only select repositories → choose `Temple-of-Epiphany/anchor-drag-pro-releases`
6. **Repository permissions:**
    - **Contents:** Read and write
    - **Metadata:** Read (auto-granted)
    - (Leave everything else on No access)
7. Generate token → copy the value (you only see it once)

### Step 2 — Add the PAT as a repo secret

In the **firmware** repo (`Temple-of-Epiphany/anchor-drag-pro`):

1. Settings → Secrets and variables → Actions → New repository secret
2. Name: `RELEASES_REPO_TOKEN`
3. Value: paste the PAT from Step 1
4. Save

That's it. The next tag push triggers the workflow and publishes to the
public repo.

## Cutting a release

### Normal path (tag-triggered)

```bash
# 1. Update CHANGELOG.md — move [Unreleased] content under a new [v0.2.1] - YYYY-MM-DD section
# 2. Commit the changelog update
git add CHANGELOG.md
git commit -m "Release v0.2.1"

# 3. Tag and push
git tag -a v0.2.1 -m "v0.2.1 — short summary"
git push origin master
git push origin v0.2.1

# 4. Watch the release workflow run
gh run watch
```

The workflow does the rest. The release appears on `anchor-drag-pro-releases`
within ~10 minutes of the tag push.

### Manual path (workflow_dispatch)

If you need to rebuild a release without retagging (rare):

1. Go to the Actions tab in the firmware repo
2. Select "Release" workflow
3. Run workflow → enter version (e.g., `v0.2.1`) → choose draft or public
4. Submit

This is useful for:
- Debugging the workflow itself
- Republishing after a build environment fix
- Producing a draft release for review before going public

## What the workflow does

| Step | Purpose |
|---|---|
| Resolve version | From tag name (push) or workflow input (manual) |
| Checkout firmware | `actions/checkout` |
| Build firmware | `espressif/esp-idf-ci-action` v1.1.0 with IDF 5.5, target esp32s3, path platform/esp32 |
| Stage assets | Copy `.bin` + compute SHA256 sidecar in `sha256sum` format |
| Extract notes | Parse the matching section from CHANGELOG.md via `.github/scripts/extract-changelog.py` |
| Publish | `softprops/action-gh-release` with `repository: ...releases` and the PAT |
| Archive | `actions/upload-artifact` — `.bin`, `.elf`, `.map`, `partition-table.bin` for 30 days for debugging |

All third-party actions are SHA-pinned per Hard Rule 11. The pinned SHAs
are commented with the tag name for legibility.

## Filename / sidecar conventions

The OTA firmware-detection code in `platform/esp32/main/ota.c` expects:

```
anchor-drag-pro_v<MAJOR>.<MINOR>.<PATCH>.bin
anchor-drag-pro_v<MAJOR>.<MINOR>.<PATCH>.sha256
```

The workflow generates these from the tag. **If you change the filename
pattern in the workflow, you must update `ota.c` to match.**

The sidecar format is standard `sha256sum` output:

```
<64 hex chars>  anchor-drag-pro_v0.2.1.bin
```

The OTA verifier reads the first 64 hex chars and ignores the filename
field (so a manually-renamed file still validates).

## Variant handling (future)

When the IMU/GPS variant build lands (Workstream 0 prereq #49 +
Workstream 5), the workflow becomes a matrix over `[display, imu_gps]`,
producing two release assets:

```
anchor-drag-pro_v0.3.0_display.bin
anchor-drag-pro_v0.3.0_display.sha256
anchor-drag-pro_v0.3.0_imu_gps.bin
anchor-drag-pro_v0.3.0_imu_gps.sha256
```

And the OTA logic learns to pick the right variant for the running
hardware via the PRODUCT_VARIANT compile flag check. Tracked under
Workstream 8 #8.5 (release pipeline child issue).

## CI smoke testing (future)

Currently the workflow only verifies that the firmware *builds*. It
does not exercise any runtime behavior — that needs hardware-in-the-loop
which is out of scope for v0.2.

When Workstream 1 (core extraction) lands with host-side unit tests, a
parallel `ci.yml` workflow runs `cmake && ctest` against the `core/`
library on every PR. That gives us actual test coverage without
hardware.

## Troubleshooting

### "Bad credentials" or 403 from softprops/action-gh-release

The `RELEASES_REPO_TOKEN` is missing, expired, or doesn't grant write to
the target repo.

- Check **Settings → Secrets and variables → Actions** has `RELEASES_REPO_TOKEN`
- Re-create the PAT with the correct repo + permissions (Step 1 above)
- PAT expiration: fine-grained PATs default to 30-90 days. Set to 1 year max.

### "Release already exists" / 422

A release with that tag already exists on `anchor-drag-pro-releases`.
Either:
- Delete the existing release manually, then re-run
- Use a different tag (recommended — never reuse a version number)

### "no section found for vX.Y.Z"

The CHANGELOG.md doesn't have a `## [vX.Y.Z]` (or `## [X.Y.Z]`) heading
matching the tag. The workflow falls back to a generic release-notes
template — not an error, but you'll want to update CHANGELOG.md and
re-run if you want detailed notes.

### Build failure on `idf.py build`

The `espressif/esp-idf-ci-action` pins to IDF 5.5. If a managed
component in `platform/esp32/main/idf_component.yml` is incompatible
with that IDF version, builds will fail.

- Check `dependencies.lock` in the repo (not currently committed) for the
  resolved component versions
- Pin the troubling component to a known-working version in `idf_component.yml`

## References

- Espressif's CI action: https://github.com/espressif/esp-idf-ci-action
- GitHub fine-grained PATs: https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens
- Keep a Changelog: https://keepachangelog.com/en/1.1.0/
- Public releases repo: https://github.com/Temple-of-Epiphany/anchor-drag-pro-releases

## Change log

- **0.1.0** (2026-05-11): Initial release pipeline. Tag-triggered + workflow_dispatch. Cross-repo publish via fine-grained PAT.
