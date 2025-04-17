#pragma once

#include "IPhysicsAssetEditor.h"
#include "PhysicsAssetUtils.h"
#include "AssetUtils/CreateSkeletalMeshUtil.h"
#include "DataAssets/PcActorDataAsset.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "PhysicsEditorBPLibrary.generated.h"

UCLASS(MinimalAPI)
class PhysicsAssetTools : public UBlueprintFunctionLibrary
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

}