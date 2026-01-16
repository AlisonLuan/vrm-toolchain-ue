# Host Project

This is a minimal Unreal Engine 5 project for testing the VrmToolchain plugin during development.

## Purpose

The HostProject serves as:
- A smoke test environment for the plugin
- A development sandbox for testing plugin features
- A reference implementation for plugin integration

## Usage

### Opening the Project

1. Ensure VRM SDK is set up (see main README)
2. Open `HostProject.uproject` with Unreal Engine 5.x
3. The VrmToolchain plugin will be automatically loaded

### Testing Plugin Features

The project automatically enables the VrmToolchain plugin. You can:

1. Test VRM file imports
2. Verify plugin modules load correctly
3. Test API functionality in Blueprints or C++
4. Run quick sanity checks

### Project Structure

```
HostProject/
├── HostProject.uproject      # Project descriptor
├── Source/
│   ├── HostProject.Target.cs          # Game target rules
│   ├── HostProjectEditor.Target.cs    # Editor target rules
│   └── HostProject/
│       ├── HostProject.Build.cs       # Module build rules
│       ├── HostProject.h
│       └── HostProject.cpp
├── Config/
│   ├── DefaultEngine.ini     # Engine configuration
│   ├── DefaultGame.ini       # Game configuration
│   └── DefaultEditor.ini     # Editor configuration
└── Content/                  # (Empty - add test assets here)
```

## Building

### From Unreal Editor

Simply open the `.uproject` file and the editor will compile everything.

### From Command Line

**Windows:**
```cmd
"%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" HostProjectEditor Win64 Development "%CD%\HostProject\HostProject.uproject"
```

**Linux:**
```bash
"$UE_ROOT/Engine/Build/BatchFiles/Build.sh" HostProjectEditor Linux Development "$PWD/HostProject/HostProject.uproject"
```

## Adding Test Content

You can add test VRM files and assets to the `Content/` directory for testing:

```
Content/
├── TestModels/
│   └── test.vrm
└── TestMaps/
    └── TestLevel.umap
```

## Notes

- This is a minimal project - it doesn't include any game logic
- Keep it lightweight - it's only for plugin testing
- Don't commit large binary assets to the repository
- The project is configured for UE 5.0+ compatibility
