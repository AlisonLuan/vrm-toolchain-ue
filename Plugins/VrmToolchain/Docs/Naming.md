# VRM Source Asset Naming Contract

This document records the authoritative naming contract for VRM source assets used by the `VrmToolchain` plugin.
Follow these rules to ensure stable Content Browser behavior and to avoid subtle visibility and lookup bugs.

## Invariant
- **Package short name MUST equal Asset name.**
  - Package: `/Game/VRM/<Base>_VrmSource`
  - Asset name (short name): `<Base>_VrmSource`
  - Object path: `/Game/VRM/<Base>_VrmSource.<Base>_VrmSource`

> If package short name and asset name differ, the Content Browser can behave inconsistently on UE 5.7 (missing or duplicate entries, selection oddities).

## Good and bad examples
- ✅ Good
  - Package: `/Game/VRM/teucu_VrmSource`
  - Asset short name: `teucu_VrmSource`
  - Object path: `/Game/VRM/teucu_VrmSource.teucu_VrmSource`

- ❌ Bad
  - Package: `/Game/VRM/teucu` and Asset: `teucu_VrmSource` (short names differ)
  - Object path of the form `/Game/X/foo.foo_VrmSource` (invalid: object short name mismatch)

**Do not reintroduce object paths like `/Game/X/foo.foo_VrmSource`.** This will cause Content Browser visibility problems.

## Sanitization rules
All base names must be sanitized before use as a package or asset short name:
- Replace invalid characters (for example: `.` (dot), whitespace, `/`) with underscores (`_`).
- Remove or normalize any characters that are not valid in Unreal asset names.
- After sanitization, the plugin appends the standard suffix `_VrmSource` to form the package short name and asset name.

Example:
- Input base: `My.Character 01` → sanitized base: `My_Character_01` → final asset name `My_Character_01_VrmSource`

## Single source of truth
- ALL code that computes VRM source package paths or asset/short names MUST use the `FVrmAssetNaming` helpers in `VrmAssetNaming.h`.
- `FVrmAssetNaming` provides:
  - `MakeVrmSourceAssetName(BaseName)`
  - `MakeVrmSourcePackagePath(FolderPath, BaseName)`
  - `StripVrmSourceSuffix(Name)`
  - `SanitizeBaseName(BaseName)`

Using `VrmAssetNaming` ensures the invariant is preserved uniformly across factories, conversion services, and tests.

## Notes
- This contract is intentionally strict to avoid Content Browser instability in UE 5.7. Keep it simple: sanitized base name + `_VrmSource`, and the package short name must be identical to the asset short name.
- If you need to derive additional related asset names, derive them from the sanitized base (use `StripVrmSourceSuffix` when appropriate).