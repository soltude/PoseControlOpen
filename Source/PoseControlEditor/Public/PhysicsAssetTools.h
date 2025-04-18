#pragma once

#include "IPhysicsAssetEditor.h"
#include "PhysicsAssetUtils.h"
#include "AssetUtils/CreateSkeletalMeshUtil.h"
#include "DataAssets/PcActorDataAsset.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "PhysicsAssetTools.generated.h"
 
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

UCLASS(MinimalAPI)
class UPhysicsAssetTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UDataTable* BodyParamsDataTable;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UPcActorDataAsset> PcActorDataAsset;
protected:
	UPROPERTY(BlueprintReadOnly)
	bool bPhysAssetHelperValid = false;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UPhysicsAsset> TargetPhysicsAsset; 
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> SkelMeshComp; 
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USkeletalMesh> NewSkeletalMesh; 

	FTimerHandle TimerHandle;
public:
	
	UPhysicsAssetTools();
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
	
	static bool AlignCapsule(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkeletalMesh, FName BodyName, TArray<FBoneVertInfo> Infos);

};


// Helper functions

static FMatrix ComputeCovarianceMatrix(const FBoneVertInfo& VertInfo);
static FVector ComputeEigenVector(const FMatrix& A);

