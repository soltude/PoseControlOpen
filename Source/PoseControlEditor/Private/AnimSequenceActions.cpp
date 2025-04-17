// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimSequenceActions.h"

#include "AssetToolsModule.h"
#include "CustomLogging.h"
#include "FileHelpers.h"
#include "Animation/PoseAsset.h"
#include "AssetUtils/CreateMaterialUtil.h"
#include "Factories/PoseAssetFactory.h"
#include "UObject/SavePackage.h"

UAnimSequenceActions::UAnimSequenceActions()
{
	SupportedClasses = {UAnimSequence::StaticClass()};
}

FString GetFolderPath(FString Path)
{
	int LastIndex;
	Path.FindLastChar('/', LastIndex);
	if(LastIndex == INDEX_NONE || LastIndex == 0)
	{
		return "/";
	}
	return Path.Mid(0,LastIndex + 1);
}

TArray<FString> UAnimSequenceActions::GetPoseNames(UDthImportDataAsset* DthImportDataAsset, FString AnimName)
{
	TMap<FString, TArray<FString>> GroupPoseNameMap;
	TArray<FString> GroupNames;
	if(DthImportDataAsset)
	{
		GroupPoseNameMap = DthImportDataAsset->GetGroupPoseMap();
		GroupPoseNameMap.GetKeys(GroupNames);
	}
	else
	{
		LGW("No DthImportDataAsset found.")
		return GroupNames;
	}
	for(auto GroupName : GroupNames )
	{
		if(AnimName.Contains(GroupName))
		{
			if(auto PoseNamesPtr = GroupPoseNameMap.Find(GroupName))
			{
				if(!PoseNamesPtr->IsEmpty())
				{
					return *PoseNamesPtr;
				}
				break;
			}
		}
	}
	LGE("Pose group not found or has no pose names")
	return GroupNames;
}

void UAnimSequenceActions::CreatePoseAssets()
{
	IAssetTools &AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UPoseAssetFactory* PoseAssetFactory = NewObject<UPoseAssetFactory>(UPoseAssetFactory::StaticClass());
	TArray<UObject*> Assets = UEditorUtilityLibrary::GetSelectedAssets();
	for (UObject* Asset : Assets )
	{
		if (const auto AnimSequence = Cast<UAnimSequence>(Asset))
		{
			FString AnimName = AnimSequence->GetName();
			FString AnimPath = AnimSequence->GetPathName();
			FString FolderPath = GetFolderPath(AnimPath);
			if(DthImportDataAsset->SubfolderName != FName())
				FolderPath += DthImportDataAsset->SubfolderName + "/";
			
			TArray<FString> PoseNames = GetPoseNames(DthImportDataAsset, AnimName);
			if(PoseNames.IsEmpty()) {
				LGW("Pose Names not found for %s.", *AnimName); }
				
			PoseAssetFactory->PoseNames = PoseNames;
			PoseAssetFactory->SourceAnimation = AnimSequence;
			PoseAssetFactory->TargetSkeleton = AnimSequence->GetSkeleton();
			
			FString Search = DthImportDataAsset->SearchString;
			FString Replace = DthImportDataAsset->ReplaceString;
			FString PoseAssetName = AnimName.Replace(*Search, *Replace);
			FString PackagePath = FolderPath; // + PoseAssetName;
			//auto Package = CreatePackage(*PackagePath);
			UObject* NewAsset = AssetTools.Get().CreateAsset(PoseAssetName, PackagePath,
			                                                       UPoseAsset::StaticClass(), PoseAssetFactory);
			if(NewAsset)
			{
				if(UPoseAsset* PoseAsset = Cast<UPoseAsset>(NewAsset))
				{
					if(!PoseNames.IsEmpty())
					{
						const FName BasePose = FName(PoseNames[0]);
						//PoseAsset->SetBasePoseName(BasePose);
					}
					// FSavePackageArgs SaveArgs = FSavePackageArgs();
					// SaveArgs.SaveFlags = RF_Public | RF_Standalone;
					// FString Filename = PackagePath + "." + PoseAssetName;
					
					bool bOutSuccess = UEditorLoadingAndSavingUtils::SavePackages({ PoseAsset->GetPackage() }, false);
					if (bOutSuccess) {
						LG("PoseAsset %s successfully created and saved", *PoseAssetName)	
					}
					else {
						LGE("PoseAsset %s failed to be saved", *PoseAssetName)	
					}
				}
			}
			else {
				LGE("PoseAsset %s not created", *PoseAssetName)	
			}
		}
	}	
}

void UAnimSequenceActions::CreatePoseAssetsWithDataAsset(UDthImportDataAsset* InDataAsset)
{
	DthImportDataAsset = InDataAsset;
	CreatePoseAssets();
}
