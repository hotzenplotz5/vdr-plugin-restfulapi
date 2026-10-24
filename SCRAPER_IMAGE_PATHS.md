# RESTfulAPI scraper image roots — distribution-neutral fix

This branch fixes the RESTfulAPI recording JSON hang that can occur when an
optional EPG image directory is missing. It also makes recording artwork
metadata and `/scraper/image/...` use the scraper's own published image root.

## Source contract

- TVScraper and current scraper2vdr plugins implement VDR
  `Service("GetEnvironment", &environment)` with `cEnvironment`
  fields `basePath`, `seriesPath`, `moviesPath`.
- `basePath` is the authoritative root for images returned by this provider.
  It honors TVScraper's `--dir` override and otherwise its normal VDR
  plugin-cache directory; it must **not** be replaced by an inferred
  `/var/cache/vdr/epgimages` or a guessed `/var/cache/vdr/plugins/tvscraper`.
- If an older scraper does not implement `GetEnvironment`, keep the
  explicitly configured legacy `EpgImageDirectory()` as a fallback.
  Absence of both locations is valid and means no artwork.
- `/events/image` and any external EPG-images behavior remain separate from
  TVScraper's recording-artwork root.
- Both JSON image references and GET image lookup use the *same* resolved root.

## Regression and security boundaries

`StringExtension::replace()` now returns the input unchanged for an empty
search substring. Previously this could loop without progress when the EPG
image path was empty and TVScraper metadata reached `cleanImagePath()`.

A missing EPG images root no longer turns event artwork scanning into a scan
of `/*`.

The image endpoint validates relative URLs, decodes percent-encoded file names
once, rejects path traversal/control characters and confines reads to
canonical regular files inside the scraper root. A symlink escaping the root
is denied. Resized images are written only inside RESTfulAPI's private
cache with checked target paths; the old shell-based `mkdir -p` has been
removed. Unknown image types are not served.

The helper is independent of VDR and cxxtools. Run:

```sh
make test-scraper-image-paths
```

For the full plugin: compile with the target VDR/cxxtools toolchain and run
an integration test on both Ubuntu/yaVDR and MLD 6.6. The isolated helper
tests do **not** establish full-plugin compilation or target acceptance.

## Required integration checks

1. With no `/var/cache/vdr/epgimages` and no `-e` override, a bounded
   `/recordings.json?start=0&limit=1` should return without hanging.
2. With TVScraper loaded, JSON poster/fanart paths should be relative to
   `GetEnvironment.basePath`, and the matching `/scraper/image/`
   GET should return the correct artwork.
3. Repeat with TVScraper using nondefault `--dir`, and no scraper installed.
4. Verify escaped spaces, resized-image requests (when Magick++ is built),
   symlink/path traversal rejection, no new root-scanning behavior, and
   `/events/image` backward compatibility.
5. Separate the MLD VDR's reported four-minute restart after full Suite
   installation from this JSON timeout until logs/core evidence establishes
   a causal link.

## MLD packaging note

MLD currently pins upstream `yavdr/vdr-plugin-restfulapi` at
`79f9bd26370a08d6c5186c19cefa300c569045ea`. **Do not simply replace
that pin with this fork's `vdr-suite-integration` SHA**: that branch contains
other Suite-specific RESTfulAPI features. Maintain the minimal empty-root
vendor patch already staged under the VDR-Suite MLD topic branch, or
port these commits into an isolated upstream/MLD-compatible series before
bumping an MLD recipe. Keep Ubuntu and MLD package acceptance separate.
