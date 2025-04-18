// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityLibrary.h"
#include "Editor/Blutility/Classes/AssetActionUtility.h"
#include "UObject/Object.h"
#include "AnimSequenceActions.generated.h"

/**
 * 
 */
UCLASS()
class POSECONTROLEDITOR_API UAnimSequenceActions : public UAssetActionUtility
{
	GENERATED_BODY()
public:
	UAnimSequenceActions();


	// UFUNCTION(BlueprintCallable, CallInEditor)
	// void CreatePoseAssets();
	//
	// UFUNCTION(BlueprintCallable, CallInEditor)
	// void CreatePoseAssetsWithDataAsset(UDthImportDataAsset* InDataAsset);
	//
	// UFUNCTION(BlueprintCallable)
	// static TArray<FString> GetPoseNames(UDthImportDataAsset* DthImportDataAsset, FString AnimName);
	// 	
	// UPROPERTY(BlueprintReadWrite, EditAnywhere)
	// TObjectPtr<UDthImportDataAsset> DthImportDataAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> FbmPoseNames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> GenPoseNames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> MisPoseNames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> PhyPoseNames;
	
	TMap<FString, TArray<FName>*> GroupPoseNameMap;
	
};
