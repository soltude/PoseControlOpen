// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IPhysicsAssetEditor.h"
#include "PhysicsAssetUtils.h"
#include "AssetUtils/CreateSkeletalMeshUtil.h"
#include "DataAssets/PcActorDataAsset.h"
#include "PhysicsEngine/ConstraintInstance.h"
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

USTRUCT(BlueprintType)
struct FMeshBakeOptions
{
	GENERATED_BODY()
public:
	bool bOverwritePhysicsAsset;
	bool bOverwriteSkeletalMeshAsset;
	bool bGeneratePhysicsBodies;
	bool bCopyTwistBones;
	bool bDeleteTwistBones;
	bool bAlignCapsulesToBones;
};

USTRUCT(BlueprintType)
struct FAdjustBodiesOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MatchBodyRegex;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PositionRatio;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RadiusRatio;
};

USTRUCT(BlueprintType)
struct FAdjustConstraintOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MatchParentBodyRegex;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MatchChildBodyRegex;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PositionAlpha;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float OrientationAlpha;
};
USTRUCT(BlueprintType)
struct FAddConstraintsOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddRootConstraints = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddRingConstraints = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddTangentConstraints = true;
	
};
/**
 * 
 */
UCLASS(MinimalAPI)

class UPhysicsEditorBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UPhysicsEditorBPLibrary();
	bool SetDataAsset(UPcActorDataAsset* DataAsset);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Process Character", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool ProcessCharacterDA(UPcActorDataAsset* DataAsset, FMeshBakeOptions MeshBakeOptions);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Copy Mesh With Morphs", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool CopyPhysicsAsset(UPcActorDataAsset* DataAsset, bool bForceCopy);

	UDataTable* GetBodySetupsDataTable() const;
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Physics Asset and Morphed SKM", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool CreatePhysicsAssetAndMorphedSkm(UPcActorDataAsset* DataAsset);
	
	bool CreatePhysicsAssetInternal();

	UFUNCTION()
	void CreatePhysicsAssetDelayed();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Copy Mesh With Morphs", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static USkeletalMesh* CopyMeshWithMorphs(USkeletalMeshComponent* SkeletalMeshComponent, UPcActorDataAsset* DataAsset);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Create Bodies From Data Table", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool CreateBodiesFromDataTable(UPcActorDataAsset* DataAsset);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Copy Twist Shape To Parent", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool CopyTwistShapeToParent(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkeletalMesh, FName BodyName, bool bDeleteChildBody);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Copy Twist Shapes To Parents", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool CopyTwistShapesToParents(UPcActorDataAsset* DataAsset, bool bDeleteChildBodies);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Align Capsules to Bones", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool AlignCapsulesToBones(UPcActorDataAsset* DataAsset);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Align Capsule to Bone", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool AlignCapsuleToBone(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkeletalMesh, FName BodyName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Fix Constraint Scale", Keywords = "PhysicsAsset"), Category = "PhysicsAssetLibrary")
	static bool FixConstraintScale(UPhysicsAsset* PhysicsAsset);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Ref Pose Override", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool SetRefPoseOverride(USkeletalMeshComponent* SkeletalMeshComponent, TArray<FTransform> NewRefPoseTransforms);

	static bool AlignCapsule(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkeletalMesh,
	                         FName BodyName, TArray<FBoneVertInfo> Infos);

	static bool AlignConstraint(USkeletalMeshComponent* SkelMesh, int32 ConstraintIndex, float PositionBlend = 0.f, float OrientationBlend = 0.f);

	static USkeletalMeshComponent* GetSkelMeshComponent(FPersonaAssetEditorToolkit* PhysicsAssetEditor);

	 
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Adjust Constraints", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool AdjustConstraints(USkeletalMeshComponent* SkelMeshComp, FAdjustConstraintOptions Options);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Adjust Breast Point Body", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool AdjustBreastPointBody(USkeletalMeshComponent* SkelMeshComp, FAdjustBodiesOptions Options, FName BodyName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Adjust Bodies", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool AdjustBodies(USkeletalMeshComponent* SkelMeshComp, FAdjustBodiesOptions Options);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Focus Or Open Physics Asset Editor", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static USkeletalMeshComponent* FocusOrOpenPhysAssetEditor(UPhysicsAsset* PhysicsAsset);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Breast Point Constraints", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static bool AddBreastPointConstraints(USkeletalMeshComponent* SkelMeshComp, FAddConstraintsOptions Options);
	
	static FName MakeConstraintNameBreastRing(int n, int side, int ring, int point);
	static FName MakeConstraintNameBreastUp(int side, int ring, int point);

	static bool AddBreastPointRingConstraints(USkeletalMeshComponent* SkelMeshComp, FAddConstraintsOptions Options);
	
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Make New Constraint", Keywords = "PhysicsAsset"), Category = "PoseControlEditor")
	static UPhysicsConstraintTemplate* MakeNewConstraint(UPhysicsAsset* PhysicsAsset, FName ConstraintName,
	                                              FName ChildBodyName,
	                                              FName ParentBodyName);
	static UPhysicsConstraintTemplate* MakeNewConstraint(UPhysicsAsset* PhysicsAsset, FName ConstraintName,
	                                                     int32 ChildBodyIndex, int32 ParentBodyIndex);
	static bool RegexMatch(FRegexPattern Pattern, FString Str);
	
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

// Helper functions

static FMatrix ComputeCovarianceMatrix(const FBoneVertInfo& VertInfo);
static FVector ComputeEigenVector(const FMatrix& A);

/** Parameters for PhysicsAsset creation */
USTRUCT(BlueprintType)
struct FPhysAssetCreateParamsRow : public FTableRowBase
{
	GENERATED_BODY()

	FPhysAssetCreateParamsRow()
	{
		MinBoneSize = 20.0f;
		MinWeldSize = KINDA_SMALL_NUMBER;
		GeomType = EFG_Sphyl;
		VertWeight = EVW_DominantWeight;
		bAutoOrientToBone = true;
		bCreateConstraints = true;
		bWalkPastSmall = true;
		bBodyForAll = false;
		bDisableCollisionsByDefault = true;
		AngularConstraintMode = ACM_Limited;
		HullCount = 4;
		MaxHullVerts = 16;
		LevelSetResolution = 8;
		LatticeResolution = 8;
	}

	FPhysAssetCreateParams GetCreateParams() const;	
	
	/** Bone name */
	UPROPERTY(EditAnywhere, Category = "Body Creation")
	FName								BoneName;
	
	/** Bones that are shorter than this value will be ignored for body creation */
	UPROPERTY(EditAnywhere, Category = "Body Creation")
	float								MinBoneSize;

	/** Bones that are smaller than this value will be merged together for body creation */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Body Creation")
	float								MinWeldSize;

	/** The geometry type that should be used when creating bodies */
	UPROPERTY(EditAnywhere, Category = "Body Creation", meta=(DisplayName="Primitive Type"))
	TEnumAsByte<EPhysAssetFitGeomType> GeomType;

	/** How vertices are mapped to bones when approximating them with bodies */
	UPROPERTY(EditAnywhere, Category = "Body Creation", meta=(DisplayName="Vertex Weighting Type"))
	TEnumAsByte<EPhysAssetFitVertWeight> VertWeight;

	/** Whether to automatically orient the created bodies to their corresponding bones */
	UPROPERTY(EditAnywhere, Category = "Body Creation")
	bool								bAutoOrientToBone;

	/** Whether to create constraints between adjacent created bodies */
	UPROPERTY(EditAnywhere, Category = "Constraint Creation")
	bool								bCreateConstraints;

	/** Whether to skip small bones entirely (rather than merge them with adjacent bones) */
	UPROPERTY(EditAnywhere, Category = "Body Creation", meta=(DisplayName="Walk Past Small Bones"))
	bool								bWalkPastSmall;

	/** Forces creation of a body for each bone */
	UPROPERTY(EditAnywhere, Category = "Body Creation", meta=(DisplayName="Create Body for All Bones"))
	bool								bBodyForAll;

	/** Whether to disable collision of body with other bodies on creation */
	UPROPERTY(EditAnywhere, Category = "Body Creation")
	bool								bDisableCollisionsByDefault;

	/** The type of angular constraint to create between bodies */
	UPROPERTY(EditAnywhere, Category = "Constraint Creation", meta=(EditCondition="bCreateConstraints"))
	TEnumAsByte<EAngularConstraintMotion> AngularConstraintMode;

	/** When creating multiple convex hulls, the maximum number that will be created. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Body Creation")
	int32								HullCount;

	/** When creating convex hulls, the maximum verts that should be created */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Body Creation")
	int32								MaxHullVerts;

	/** When creating level sets, the grid resolution to use */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Body Creation", 
		meta = (ClampMin = 1, UIMin = 10, UIMax = 100, ClampMax = 500, EditCondition = "GeomType == EPhysAssetFitGeomType::EFG_LevelSet || GeomType == EPhysAssetFitGeomType::EFG_SkinnedLevelSet"))
	int32								LevelSetResolution;

	/** When creating skinned level sets, the embedding grid resolution to use*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Body Creation",
		meta = (ClampMin = 1, UIMin = 10, UIMax = 100, ClampMax = 500, EditCondition = "GeomType == EPhysAssetFitGeomType::EFG_SkinnedLevelSet"))
	int32								LatticeResolution;


	bool operator==(const FPhysAssetCreateParamsRow& OtherItem) const
	{
		if (BoneName == OtherItem.BoneName)
			return true;
		return false;
	}
	
};
