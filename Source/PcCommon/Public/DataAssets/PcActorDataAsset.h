// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Structs/FConstraintParams.h"
#include "UObject/Object.h"
#include "PcActorDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class PCCOMMON_API UPcActorDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName CharacterName;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString NewAssetPath = FString("Game/Characters/Meshes/");
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<USkeletalMesh> SourceSkeletalMesh;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<USkeletalMesh> TargetSkeletalMesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UPhysicsAsset> SourcePhysicsAsset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UPhysicsAsset> TargetPhysicsAsset;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UDataTable> BodyParamsDataTable;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UDataTable> ConstraintParamsDataTable;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FTransform> RefPoseTransforms;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, float> CurveMap;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, float> MorphTargetMap;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimBlueprint* AnimBlueprintRef;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> CapsuleNames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> TwistNames;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FPhatConstraintOptions PhatConstraintOptions;
};
