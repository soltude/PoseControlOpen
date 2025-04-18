// Copyright Epic Games, Inc. All Rights Reserved.

#include "PoseControlEditor.h"
#include "PoseControlEditorStyle.h"
#include "PoseControlEditorCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "CustomLogging.h"

DEFINE_LOG_CATEGORY(LogAnchor);
DEFINE_LOG_CATEGORY(LogPhysicsEditor);

static const FName PoseControlTabName("PoseControl");

#define LOCTEXT_NAMESPACE "FPoseControlEditorModule"

void FPoseControlEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FPoseControlStyle::Initialize();
	FPoseControlStyle::ReloadTextures();

	FPoseControlCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FPoseControlCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FPoseControlEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPoseControlEditorModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PoseControlTabName, FOnSpawnTab::CreateRaw(this, &FPoseControlEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FPoseControlTabTitle", "PoseControl"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	
	//Setup
	
}

void FPoseControlEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FPoseControlStyle::Shutdown();

	FPoseControlCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PoseControlTabName);
}

TSharedRef<SDockTab> FPoseControlEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FPoseControlEditorModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("PoseControlEditor.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(WidgetText)
			]
		];
}

void FPoseControlEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(PoseControlTabName);
}

void FPoseControlEditorModule::CreatePhysicsBodiesClicked()
{
	
}

void FPoseControlEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FPoseControlCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FPoseControlCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPoseControlEditorModule, PoseControlEditor)