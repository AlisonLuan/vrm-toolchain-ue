# VRM Retarget Scaffolding Generator

## Overview

The VRM Retarget Scaffolding Generator is an editor-only workflow helper that automatically generates IKRig and IK Retargeter assets for imported VRM skeletal meshes, streamlining the setup process for animation retargeting in Unreal Engine 5.7.

## Features

- **Automatic IKRig generation** for VRM source skeletons
- **Bone chain inference** using deterministic, case-insensitive matching
- **Idempotent asset creation** - updates existing assets without duplication
- **Content Browser integration** - accessible via right-click context menu
- **Best-effort approach** - creates assets even when some bones are missing
- **Clear logging** - displays success messages and warnings only when needed

## How to Use

### Basic Workflow

1. **Import a VRM asset** using a VRM importer (e.g., VRM4U)
2. **Navigate to the skeletal mesh** in the Content Browser
3. **Right-click** on the skeletal mesh asset
4. **Select "Create Retarget Scaffolding"** from the context menu
5. **Check the output** folder for generated assets

### Generated Assets

The tool creates the following assets in a `Retarget/` subfolder next to your source mesh:

- **IKRig_\<MeshName\>** - IKRig definition for your VRM skeleton
- **IKRig_\<TargetName\>** - IKRig definition for target skeleton (optional)
- **RTG_\<MeshName\>_To_\<TargetName\>** - IK Retargeter linking source to target (when target is specified)

### Asset Placement

Assets are created at:
```
<YourContentFolder>/YourVRMAssets/Retarget/
```

For example, if your skeletal mesh is at:
```
/Game/Characters/MyCharacter/SK_MyCharacter
```

The generated IKRig will be at:
```
/Game/Characters/MyCharacter/Retarget/IKRig_SK_MyCharacter
```

## Bone Chain Inference

The tool automatically detects the following bone chains using case-insensitive pattern matching:

### Supported Chains

- **Spine** - pelvis/hips → neck/head/chest
- **Head** - head or neck bone
- **LeftArm** - left shoulder/clavicle/upperarm → left hand
- **RightArm** - right shoulder/clavicle/upperarm → right hand
- **LeftLeg** - left thigh/upperleg → left foot
- **RightLeg** - right thigh/upperleg → right foot

### Naming Variants Supported

The tool recognizes multiple naming conventions:
- Standard prefixes: `Left`, `Right`, `L_`, `R_`
- Case variations: `lefthand`, `LeftHand`, `LEFTHAND`
- Common synonyms: `UpperLeg`/`Thigh`, `LowerArm`/`Forearm`, etc.

## Best-Effort Behavior

If expected bones are missing:
- The tool logs a concise warning listing missing bones
- Creates assets anyway with available bones
- Marks incomplete chains appropriately
- Continues processing other chains

Example warning:
```
LogVrmToolchainEditor: Warning: No bone chains could be inferred from source skeleton
```

## Idempotent Operation

Running the tool multiple times on the same asset:
- **Updates** existing assets instead of creating duplicates
- Prevents "_1", "_2" naming spam
- Maintains stable asset references
- Safe to re-run after skeleton changes

## Troubleshooting

### "No bone chains could be inferred"

**Cause**: The skeleton doesn't match expected naming patterns.

**Solution**:
1. Check your skeleton bone names in the Skeleton Editor
2. Verify bones follow common naming conventions (pelvis, hips, lefthand, etc.)
3. Manually create IKRig if automatic inference fails

### "Target IKRig not found"

**Cause**: You're trying to create a retargeter without a target IKRig.

**Solution**:
1. First create a target IKRig manually for your target skeleton (e.g., UE5 Mannequin)
2. Or enable automatic target IKRig creation in future versions

### Assets not appearing in Content Browser

**Cause**: Content Browser view not refreshed.

**Solution**:
- Right-click in Content Browser and select "Refresh"
- Or restart the editor

## Limitations

This tool provides **scaffolding only** and does not:
- Tune IK chain settings or solve parameters
- Edit retarget poses
- Create Control Rigs or Animation Blueprints
- Fix or normalize skeletons (see Issue 4 for normalization)
- Guarantee perfect retargeting without manual adjustments

## Integration Points

### Future Issue 2 Integration

When Issue 2 (VRM metadata/skeleton mapping) is implemented, this tool will:
- Use stored skeleton coverage data instead of heuristics
- Provide more accurate bone chain inference
- Support VRM-specific bone mappings

### Settings (Future)

Future versions may add:
- Configurable output folder strategy
- Target skeleton presets (UE5 Mannequin, MetaHuman, etc.)
- Custom bone naming pattern overrides

## Technical Details

### Module Dependencies

The feature requires these Unreal Engine modules:
- `IKRig` - Core IK rig functionality
- `IKRigEditor` - Editor-only IK rig authoring
- `ContentBrowser` - Context menu integration
- `AssetTools` - Asset creation and management

### Determinism

All bone chain inference is deterministic:
- Always produces the same results for the same input
- Sorted bone lists ensure stable output
- Case-insensitive comparisons are consistent
- Tested via automated unit tests

### Performance

- Inference is fast (< 100ms for typical skeletons)
- No file system operations during inference
- Asset creation may take a few seconds depending on skeleton complexity

## See Also

- [Issue 2: VRM Metadata and Skeleton Mapping](../../../README.md#issue-2)
- [Issue 4: Skeleton Normalization](../../../README.md#issue-4)
- [Unreal Engine IKRig Documentation](https://docs.unrealengine.com/5.7/en-US/ik-rig-in-unreal-engine/)
