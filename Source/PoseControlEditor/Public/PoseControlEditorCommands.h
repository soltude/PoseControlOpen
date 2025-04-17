// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "PoseControlEditorStyle.h"

class FPoseControlCommands : public TCommands<FPoseControlCommands>
{
public:

	FPoseControlCommands()
		: TCommands<FPoseControlCommands>(TEXT("PoseControl"), NSLOCTEXT("Contexts", "PoseControl", "PoseControl Plugin"), NAME_None, FPoseControlStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
	TSharedPtr< FUICommandInfo > CreatePhysicsBodies;
};