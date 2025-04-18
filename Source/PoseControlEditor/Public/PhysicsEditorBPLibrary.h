// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IPhysicsAssetEditor.h"
#include "PhysicsAssetUtils.h"
#include "Utils.h"
#include "AssetUtils/CreateSkeletalMeshUtil.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "PcCommon/Public/DataAssets/PcActorDataAsset.h"
#include "PcCommon/Public/Structs/FConstraintParams.h"
#include "PhysicsEditorBPLibrary.generated.h"

USTRUCT(BlueprintType)
struct FAssetOptionsWrapper
{
	GENERATED_BODY()
public:
	FAssetOptionsWrapper()
	{
		Options = UE::AssetUtils::FSkeletalMeshAssetOptions();
	}
	FAssetOptionsWrapper(UE::AssetUtils::FSkeletalMeshAssetOptions InOptions)
	{
		Options = InOptions;	
	}
	
	UE::AssetUtils::FSkeletalMeshAssetOptions Options;
};



/**
 * 
 */
UCLASS(MinimalAPI)

class UPhysicsEditorBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Fix Constraint Scale", Keywords = "PhysicsAsset"), Category = "PhysicsAssetLibrary")
	static bool FixConstraintScale(UPhysicsAsset* PhysicsAsset);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Ref Pose Override", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool SetRefPoseOverride(USkeletalMeshComponent* SkeletalMeshComponent, TArray<FTransform> NewRefPoseTransforms);


	static bool AlignConstraint(USkeletalMeshComponent* SkelMesh, int32 ConstraintIndex, float PositionBlend = 0.f, float OrientationBlend = 0.f);

	static USkeletalMeshComponent* GetSkelMeshComponent(FPersonaAssetEditorToolkit* PhysicsAssetEditor);

	 
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Adjust Constraints", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool AdjustConstraints(USkeletalMeshComponent* SkelMeshComp, FAdjustConstraintsOptions Options, TArray<FName> JointNames);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Adjust Breast Point Body", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool AdjustPointBody(USkeletalMeshComponent* SkelMeshComp, FAdjustBodiesOptions Options, FName BodyName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Adjust Bodies", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool AdjustBodies(USkeletalMeshComponent* SkelMeshComp, FAdjustBodiesOptions Options, TArray<FName> BodyNames);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Focus Or Open Physics Asset Editor", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static USkeletalMeshComponent* FocusOrOpenPhysAssetEditor(UPhysicsAsset* PhysicsAsset);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Apply All Constraint Options", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool ApplyConstraintParams(UPhysicsAsset* PhysicsAsset, FConstraintParams Params);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Apply All Constraint Options", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool ApplyAllConstraintOptions(UPhysicsAsset* PhysicsAsset, FPhatConstraintOptions Options);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "GetConstraintParams", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool GetConstraintParams(UPhysicsAsset* PhysicsAsset, int32 ConstraintIndex, FConstraintParams& OutParams);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get All Constraint Params", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool GetAllConstraintParams(UPhysicsAsset* PhysicsAsset, TArray<FConstraintParams>& OutParams,
	                                   TArray<FName> ConstraintNames);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "ScaleConstraintsByMass", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool ScaleConstraintsByMass(UPhysicsAsset* PhysicsAsset, FPhatConstraintOptions& PhatConstraintOptions, TArray<FName> ConstraintNames, float
	                                   ScaleFactor);

	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Mirror Constraints", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool MirrorConstraintOptions(UPhysicsAsset* PhysicsAsset, TArray<FName> ConstraintNames,
                                                      FPhatConstraintOptions Options = FPhatConstraintOptions(),
                                                      bool bRightToLeft = false);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Copy Constraint Options", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool CopyConstraintOptions(UPhysicsAsset* PhysicsAsset, UPhysicsAsset* SourcePhysicsAsset,
	                                     FPhatConstraintOptions Options);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Closest Point Constraints", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static TArray<int32> AddPointToParentConstraints(USkeletalMeshComponent* SkeletalMeshComponent, FAddPointConstraints Options, TArray<FName> ChildBodies);
		
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Closest Point Constraints", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static TArray<int32> AddClosestPointConstraints(USkeletalMeshComponent* SkeletalMeshComponent, FConstraintParams DefaultParams, TArray<FName> TargetBodies, TArray<
	                                                FName>
	                                                SourceBodies, int32 NumClosestPoints, bool bExtraBones, TArray<int32> ClosestBones);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Point Constraints", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static TArray<int32> AddPointConstraints(USkeletalMeshComponent* SkeletalMeshComponent, FAddPointConstraints Options);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Phat Constraints", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static TArray<int32> AddPhatConstraints(USkeletalMeshComponent* SkeletalMeshComponent, FPhatConstraintOptions Options);
	
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Make New Constraint", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static int32 MakeNewConstraint(UPhysicsAsset* PhysicsAsset, FConstraintParams Params);
	
	
	static bool RegexMatch(FRegexPattern Pattern, FString Str);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Filter Names", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static TArray<FName> FilterNames(TArray<FName> Names, FString PatternString);
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UDataTable* BodyParamsDataTable;

	UPROPERTY(BlueprintReadOnly)
	bool bPhysAssetHelperValid = false;
	
protected:
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UPcActorDataAsset> PcActorDataAsset;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UPhysicsAsset> TargetPhysicsAsset; 
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> SkelMeshComp; 
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USkeletalMesh> NewSkeletalMesh; 

	FTimerHandle TimerHandle;

public:

	/**
	¦* Editor Only Will not work in packaged build.
	*
	* Saves an asset. Doesn't matter if the asset is modified or not.
	*/
	UFUNCTION(BlueprintCallable, Category = "Asset Helpers")
	static void SaveAsset (FString AssetPath, bool& bOutSuccess, FString& OutInfoMessage);
	// /**
	// * Editor Only - Will not work in packaged build.
	// *
	// * Mark an asset as modified for when you want the user to know he has to save it.
	// */
	// UFUNCTION(BlueprintCallable, Category = "Alex Quevillon |56 - Save Assets")
	// static void MarkAssetModified(FString AssetPath, bool& bOutSuccess, FString& OutInfoMessage);
	//
	//
	// /**
	// * Editor Only Will not work in packaged build.
	// * Gets the list of all the assets marked as modified */
	// UFUNCTION(BlueprintCallable, Category = "Alex Quevillon |56 - Save Assets")
	// static TArray<UObject*> GetModifiedAssets(bool& boutSuccess, FString& OutInfoMessage);
	// /**
	// * Editor Only - Will not work in packaged build.
	// */
	// UFUNCTION(BlueprintCallable, Category = "Alex Quevillon|56 - Save Assets")
	// static void SaveAllModifiedAssets(bool bPrompt, bool& boutSuccess, FString& OutInfoMessage);	
	//
	//
};
