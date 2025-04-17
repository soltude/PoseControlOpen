// Copyright Epic Games, Inc. All Rights Reserved.

#include "PoseControlEditorCommands.h"

#define LOCTEXT_NAMESPACE "FPoseControlModule"

void FPoseControlCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "PoseControl", "Bring up PoseControl window", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(CreatePhysicsBodies, "PoseControl", "Create Physics Bodies from DataTable", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
