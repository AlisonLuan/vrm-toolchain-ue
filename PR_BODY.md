## Build Fixes: Module Boundaries, Include Standardization, Deterministic Packaging

### Summary

Comprehensive fix for VrmToolchain plugin build stability addressing three interdependent issues:
1. **Include paths**: Standardized to idiomatic UE pattern (`VrmToolchain/Header.h`)
2. **Module boundaries**: Runtime ownership of `UVrmMetadataAsset` with proper editor module layering
3. **Packaging**: Two-stage deterministic build eliminating locked-directory issues and `-AllowBinaries` workaround

All changes tested via clean build from `git clean -xdf`.

---

## Proof Blocks

### 1. Clean Packaging Validation

**Command**: `git clean -xdf && Remove-Item build -Force && .\Scripts\PackagePlugin.ps1`

**Binary Check**:
```
Found forbidden files (will be removed):
  - UnrealEditor-VrmToolchain.pdb
  - UnrealEditor-VrmToolchainEditor.pdb
  Removed: 2 file(s)

Running final validation gate...
✓ Package structure validated: allowed folders only (Binaries, Resources, Source)
✓ Binary validation passed: no forbidden binaries found
```

**Output Verification**:
```
.exe count: 0
.pdb count: 0
HostProject: absent ✓
Intermediate: absent ✓
Saved: absent ✓
Total files: 42
```

---

### 2. Packaging Architecture

**Raw Build Stage** (discardable):
```
build/PackageRaw/VrmToolchain/
  (Full UE output with all artifacts)
```

**Filtered Distribution Stage** (final package):
```
build/Package/VrmToolchain/
  Binaries/        ← copied via robocopy
  Resources/       ← copied via robocopy
  Source/          ← copied via robocopy
  VrmToolchain.uplugin
  README.md
```

**Robocopy Allowlist**: `Binaries/ Resources/ Source/`

**Robocopy Denylist**: `HostProject/ Intermediate/ Saved/` + all other build artifacts

**Binary Stripping**: Runs only on dist folder (avoids locked dir issues from UE's cache)

**Validation Gates**: Run on final dist folder only

---

### 3. Module Boundary Invariant

**Runtime Module** (`VrmToolchain`):
- Canonical `UVrmMetadataAsset` class definition at `Public/VrmToolchain/VrmMetadataAsset.h`
- Enum `EVrmVersion` (single definition, no duplicates)

**Editor Module** (`VrmToolchainEditor`):
- Imports via runtime module dependency in `.Build.cs`
- Direct includes: `#include "VrmToolchain/VrmMetadataAsset.h"` (no wrapper re-exports)
- Provides mutation operations only (reading from editor tools)

**Verification**:
```
git grep -c "UCLASS.*UVrmMetadataAsset" Plugins/VrmToolchain/Source/VrmToolchain/
  → 1 (runtime module only)
git grep -c "UCLASS.*UVrmMetadataAsset" Plugins/VrmToolchain/Source/VrmToolchainEditor/
  → 0 (no editor copy)
git grep -n "#include \"VrmToolchain/VrmMetadataAsset.h\"" Plugins/VrmToolchain/Source/
  → 9 files, all properly namespaced
```

---

### 4. Checkpoint Reference

- **Tag**: `wip/build-fix-checkpoint` (base of all fixes)
- **Commit count**: 15 commits since checkpoint
- **Files changed**: 14 files (source code, scripts, docs)
- **No build artifacts**: Verified via `git diff --name-only origin/integration/all-issues..HEAD`

---

## What Changed

### Phase 1: Module Boundary Restoration (1 commit)
- Moved `UVrmMetadataAsset` from editor-only back to runtime module (was over-scoped)
- Deleted editor wrapper header that caused duplicate definitions
- Direct editor imports use runtime header via proper module dependency

### Phase 2: Deterministic Packaging (3 commits)
- Rewrote `Scripts/PackagePlugin.ps1` with two-stage approach
- Eliminated `-AllowBinaries` workaround (now forbidden in CI)
- Binary stripping/validation on dist folder only (robocopy avoids locked dirs)
- Transparent reporting: lists forbidden files before removal

### Phase 3: Include Path Standardization (5 commits)
- Reorganized headers into namespace subfolder: `Public/VrmToolchain/`
- Updated all includes to idiomatic pattern: `#include "VrmToolchain/VrmMetadata.h"`
- 9 files updated; zero remaining short-form includes
- Matches UE module naming conventions

### Phase 4: Documentation (2 commits)
- Comprehensive `BUILD_FIX_SUMMARY.md` with rationale and architecture diagrams
- Commit messages with review-friendly line references

---

## Why These Changes Matter

**Before**: 
- Packaging required `-AllowBinaries` flag (blocked by CI policy)
- Locked directories during cleanup caused intermittent failures
- Ad-hoc include paths were brittle and non-idiomatic
- Module boundaries unclear (duplicate types, wrapper headers)

**After**:
- Packaging is deterministic and CI-compliant
- No locked-directory issues (raw staging is discarded, dist is already filtered)
- Includes follow UE conventions (good for IDE support, external consumers)
- Single authoritative type definition per module (clear ownership)

---

## For Reviewers

Please focus on:

1. **Packaging robustness** (`Scripts/PackagePlugin.ps1`)
   - Are robocopy switches correct? (`/E /NP /NFL /NDL`)
   - Allowlist/denylist coverage for all UE artifacts?
   - Validation gates sufficient?

2. **Include path impact**
   - Does public header relocation break any external consumers?
   - Are all 9 updated files caught?
   - Any forward-compatibility concerns?

3. **Runtime/editor boundary**
   - Is `UVrmMetadataAsset` in runtime correct (not editor-only)?
   - Editor module dependencies correct?
   - No circular dependencies introduced?

---

## Testing

- ✅ Clean build from `git clean -xdf`
- ✅ Packaging produces 42 files with 0 forbidden binaries
- ✅ All 15 commits build successfully
- ✅ Module boundary verified (single UCLASS definition)
- ✅ Include paths verified (9 files, zero short-form includes)

Ready to merge into `integration/all-issues`.
