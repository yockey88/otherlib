# Versioning

Other Environment ships under one semantic version, `MAJOR.MINOR.PATCH`. The root
`VERSION` file is the single source of truth; everything else derives from it.
Current: **0.1.1** — the first enforced release (the full networking stack).

## Bump rules

- **PATCH** — bugfixes only; no change to any behavior surface, wire protocol, or file format.
- **MINOR** — network comms changes, scene changes, binary format changes, or new features,
  **when** compatibility logic can (and does) bridge the two versions: an old editor and a
  new editor remain fully interoperable.
- **MAJOR** — the same kinds of change when no compatibility logic can bridge them; builds
  that cannot interoperate differ in MAJOR.

## Where the version lives

- `VERSION` — the source of truth, read by cmake and the tag pipeline.
- `project(other VERSION ...)` — populated from `VERSION`; flows into the SDK package
  (`oecli package`), the installer name, and the cmake package config.
- `other-core/src/core/version.hpp` — regenerated at configure time; exposes
  `OTHER_ENVIRONMENT_VERSION_{MAJOR,MINOR,PATCH,STRING}` to code (oecli, driver boot log).
- Plugins declare their own `x.x.x` in `OTHER_PLUGIN(...)`; in-tree plugins track the
  release version.

## Format and protocol versions (separate axes)

Wire/file compatibility is negotiated by dedicated version fields, not the release number:
`kMeshProtocolVersion` (mesh links), the scene document `schema-version` + scene binary
format version, and the OCMD file format version. Bump the field with the change that
breaks it; whether compatibility logic bridges the break decides MINOR vs MAJOR above.

## Enforcement

- The tag pipeline refuses to run unless the pushed tag equals `v<VERSION>`.
- `version_tests` asserts the compiled version macros match the `VERSION` file.
- Mesh links refuse peers with a different `kMeshProtocolVersion` at LINK_HELLO; scene and
  ocmd parsers refuse documents with an unsupported format version.

## Release procedure

1. Set `VERSION`, reconfigure (regenerates `core/version.hpp`), commit.
2. Tag the release commit `v<VERSION>` and push the tag. The tag pipeline runs every build
   config through the extensive gates (unit suites, soak, network, fuzz, stress) and
   packages the installer + SDK zip.
