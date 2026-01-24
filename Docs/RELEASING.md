# Releasing VrmToolchain (Authoritative Guide)

This document defines the **only supported release process** for the VrmToolchain Unreal Engine plugin.

It exists to ensure releases are:
- deterministic
- reproducible
- CI-validated
- auditable after the fact

If a step below is skipped, the release is **invalid**.

---

## 1. Definitions

### Green build
A commit on `main` that satisfies **all** of the following:
- `CI Lite (No UE) / gates` → **PASS**
- `Build plugin (self-hosted) / build-plugin` → **PASS**
- No uncommitted changes
- No local-only patches

Only green builds may be released.

### Green tag
An annotated Git tag of the form:

```

green-ue<UE_VERSION>-YYYY-MM-DD

```

Example:
```

green-ue5.7-2026-01-24

````

Green tags **must never be moved or rewritten**.

---

## 2. Branch Protection Rules (Enforced)

The `main` branch is protected with the following rules:

- Direct pushes are **blocked**
- All changes must go through a Pull Request
- Required checks:
  - `CI Lite (No UE) / gates`
  - `Build plugin (self-hosted) / build-plugin`
- Strict mode enabled (branch must be up to date)
- At least **1 approving review**
- Admins are **not exempt**

Releases are only allowed from `main`.

---

## 3. Pre-Release Checklist (Mandatory)

Before releasing, confirm:

- [ ] You are on `main`
- [ ] `git status` shows a clean working tree
- [ ] Latest commit is green in both CI pipelines
- [ ] No CI reruns are in progress
- [ ] No hotfixes are pending

---

## 4. Canonical Release Flow (Required)

### Step 1: Run the release gate locally

Use the ReleaseGate script as the **single source of truth**:

```powershell
.\Scripts\ReleaseGate.ps1 `
  -UE "C:\Program Files\Epic Games\UE_5.7" `
  -Tag "green-ue5.7-YYYY-MM-DD"
````

By default, the script:

* fails on a dirty working tree
* runs packaging + contract validation
* creates a deterministic ZIP
* calculates SHA256
* writes `RELEASES/<tag>.sha256`
* prints (but does not execute) the `gh release create` command

⚠️ Use `-AllowDirty` **only** for emergency investigation, never for production releases.

---

### Step 2: Commit the SHA256 manifest

```powershell
git add RELEASES/<tag>.sha256
git commit -m "docs: add SHA256 manifest for <tag>"
git push
```

The SHA256 file is part of the release contract and **must be tracked**.

---

### Step 3: Create the GitHub Release

Run the exact `gh release create` command printed by `ReleaseGate.ps1`.

This ensures:

* asset name matches the hash
* release notes match the validated artifact
* no manual transcription errors

---

## 5. What Is Forbidden

The following actions are **not allowed**:

* ❌ Tagging a commit that is not green
* ❌ Moving or force-updating a green tag
* ❌ Releasing from any branch other than `main`
* ❌ Editing release ZIPs manually
* ❌ Publishing binaries not produced by `PackagePlugin.ps1`
* ❌ Skipping CI because “it passed locally”

If any of the above happens, the release is invalid and must be revoked.

---

## 6. Hotfix Policy

Hotfixes follow the **same process**:

1. Fix via PR into `main`
2. CI Lite + Tier B must pass
3. New green tag
4. New release

There are **no fast paths** around CI or ReleaseGate.

---

## 7. Auditing & Reproducibility

Every release can be audited by:

* checking the green tag commit
* verifying CI run IDs
* re-running `ReleaseGate.ps1`
* validating the published SHA256

This guarantees long-term reproducibility.

---

## 8. TL;DR

If it’s not:

* green in CI
* tagged on `main`
* produced by `ReleaseGate.ps1`
* published with a tracked SHA256

…it is **not** a real release.


