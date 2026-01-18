# Pinned Build Inputs (freeze/2026-01-18)

This file records the deterministic inputs for the plugin at the time of the freeze.

- Date: 2026-01-18
- Project commit: 9374449db7bd2db73b30239d334bfc112cdc2f7d (HEAD of default branch at freeze)
- Unreal Engine: 5.7
- VRM SDK: repo "AlisonLuan/vrm-sdk" tag `v1.0.3` (see `Scripts/VrmSdkVersion.json`)

Third-party components and other notable inputs:
- No other binary third-party artifacts are shipped in the package. Refer to `Plugins/VrmToolchain/Config/FilterPlugin.ini` and `Scripts/PackagePlugin.ps1` for packaging rules.

Release artifacts:
- (To be filled by release automation) Packaged plugin ZIP URL: <TO_BE_ADDED>
- SHA256: <TO_BE_ADDED>

Notes:
- If release artifacts are used in CI or downstream consumption, add the artifact URL and SHA256 here.
- Do not redistribute developer-only executables (for example `vrm_validate.exe`).
