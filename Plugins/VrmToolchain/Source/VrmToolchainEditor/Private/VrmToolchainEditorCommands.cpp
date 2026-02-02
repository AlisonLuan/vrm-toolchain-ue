#include "VrmToolchainEditorCommands.h"
#include "Styling/AppStyle.h"

#define LLOCTEXT_NAMESPACE "VrmToolchainEditor"

FVrmToolchainEditorCommands::FVrmToolchainEditorCommands()
	: TCommands<FVrmToolchainEditorCommands>(
		TEXT("VrmToolchain"),
		NSLOCTEXT("Contexts", "VrmToolchainEditor", "VRM Toolchain"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FVrmToolchainEditorCommands::RegisterCommands()
{
	UI_COMMAND(
		RecomputeAllImportReports,
		"Recompute All Import Reports",
		"Recomputes import reports for all VRM Meta Assets in the project.",
		EUserInterfaceActionType::Button,
		FInputChord());
}

#undef LLOCTEXT_NAMESPACE
