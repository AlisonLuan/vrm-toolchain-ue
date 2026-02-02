#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FVrmToolchainEditorCommands : public TCommands<FVrmToolchainEditorCommands>
{
public:
	FVrmToolchainEditorCommands();

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> RecomputeAllImportReports;
};
