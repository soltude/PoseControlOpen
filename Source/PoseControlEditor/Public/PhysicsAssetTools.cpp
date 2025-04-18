#include "PhysicsAssetTools.h"

#include "CustomLogging.h"
#include "EditorAssetLibrary.h"
#include "MeshUtilities.h"
#include "MeshUtilitiesCommon.h"
#include "PhysicsEditorBPLibrary.h"
#include "Animation/PcAnimInstance.h"
#include "Animation/SkeletalMeshActor.h"
#include "ConversionUtils/SceneComponentToDynamicMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Structs/FConstraintParams.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Subsystems/UnrealEditorSubsystem.h"


TArray<FName> TwistBoneNames = { FName("upperarm_twist_01_l"), FName("upperarm_twist_02_l"), FName("upperarm_twist_01_r"), FName("upperarm_twist_02_r"), FName("lowerarm_twist_01_l"), FName("lowerarm_twist_02_l"), FName("lowerarm_twist_01_r"), FName("lowerarm_twist_02_r"), FName("thigh_twist_01_l"), FName("thigh_twist_02_l"), FName("thigh_twist_01_r"), FName("thigh_twist_02_r") };

UPhysicsAssetTools::UPhysicsAssetTools()
{
	if(!BodyParamsDataTable)
	{
		static ConstructorHelpers::FObjectFinder<UDataTable> BodyParamsDT(TEXT("/Script/Engine.DataTable'/PoseControl/Data/DT_BodyParams.DT_BodyParams'"));
		BodyParamsDataTable =  BodyParamsDT.Object;
	}
	
}

bool UPhysicsAssetTools::SetDataAsset(UPcActorDataAsset* DataAsset)
{
	PcActorDataAsset = DataAsset;	
	if (!DataAsset) {
		LGE("No Data Asset found. Aborting.")
		return false; }
	
	// if TargetPhysicsAsset doesn't exist, create it. 
	TargetPhysicsAsset = DataAsset->TargetPhysicsAsset.LoadSynchronous();
	if (!TargetPhysicsAsset)
	{
		if(CopyPhysicsAsset(PcActorDataAsset, false) && PcActorDataAsset->TargetPhysicsAsset.LoadSynchronous())
		{
			TargetPhysicsAsset = PcActorDataAsset->TargetPhysicsAsset.LoadSynchronous();
			bPhysAssetHelperValid = true;
		}
		else
		{
			LGE("Target Physics Asset not found or created. Aborting.")
			return false;
		}
	}
	return true;
}


void LogBodies(UPhysicsAsset* PhysicsAsset)
{
	auto Setups = PhysicsAsset->SkeletalBodySetups;
	for(int i=0; i < Setups.Num();i++)
	{
		if(auto Setup = Setups[i])
			LG("Setup %d: %s", i, *Setup->BoneName.ToString())
		else
		{
			LGE("Setup %d: not found", i)
			Setups.RemoveAt(i--);
		}
	}
}

bool UPhysicsAssetTools::ProcessCharacterDA(UPcActorDataAsset* DataAsset, FMeshBakeOptions Options)
{
	CopyPhysicsAsset(DataAsset, Options.bOverwritePhysicsAsset);
	return true;
}

bool UPhysicsAssetTools::CopyPhysicsAsset(UPcActorDataAsset* DataAsset, bool bForceCopy = false)
{
	if (!DataAsset)
	{
		LGE("No Data Asset found. Aborting.")
		return false;
	}
	
	auto TargetPhysicsAssetPtr = DataAsset->TargetPhysicsAsset;
	UPhysicsAsset* TargetPhysicsAsset = TargetPhysicsAssetPtr.LoadSynchronous();
	
	if (!TargetPhysicsAsset || bForceCopy)
	{
		auto SourcePhysicsAssetPtr = DataAsset->SourcePhysicsAsset;
		UPhysicsAsset* SourcePhysicsAsset = SourcePhysicsAssetPtr.LoadSynchronous();
		if(SourcePhysicsAsset)
		{
			const FString TargetPath = MakeAssetPath(DataAsset->NewAssetPath,
			                                         DataAsset->CharacterName.ToString(),
			                                         "PA_");
			if(!UEditorAssetLibrary::DoesAssetExist(TargetPath))
			{
				if (auto NewAsset = UEditorAssetLibrary::DuplicateLoadedAsset(SourcePhysicsAsset, TargetPath))
				{
					if(auto NewPhysicsAsset = Cast<UPhysicsAsset>(NewAsset))
					{
						DataAsset->TargetPhysicsAsset = TSoftObjectPtr(NewPhysicsAsset);
						return true;
					}
					LGE("Physics Asset not duplicated. Aborting.")
					return false;
				}
				LGE("Physics Asset not duplicated. Aborting.")
				return false;
			}
			LGW("Physics Asset already exists. Returning true.")
			return true;
		}
		LGE("Source Physics Asset not found. Aborting.")
		return false;
	}
	LGE("Target Physics Asset already exists. Aborting.")
	return false;
}

UDataTable* UPhysicsAssetTools::GetBodySetupsDataTable() const
{
	return BodyParamsDataTable;
}

TArray<FName> GetBoneNamesToAlign(UDataTable* DataTable)
{
	TArray<FName> OutNames;
	FString ContextString;
	if(DataTable)
	{
		auto RowNames = DataTable->GetRowNames();
		for ( auto BodyName : RowNames )
		{
			FPhysAssetCreateParamsRow* Row = DataTable->FindRow<FPhysAssetCreateParamsRow>(BodyName, ContextString);
			if(Row)
			{
				if (Row->GeomType == EPhysAssetFitGeomType::EFG_Sphyl)
				{
					OutNames.Add(BodyName);
				}
			}
		}
	}
	return OutNames;
}

bool UPhysicsAssetTools::CreatePhysicsAssetAndMorphedSkm(UPcActorDataAsset* DataAsset)
{
	UPhysicsAssetTools* PhysAssetHelper = NewObject<UPhysicsAssetTools>();
	PhysAssetHelper->SetDataAsset(DataAsset);
	if (PhysAssetHelper->bPhysAssetHelperValid)
	{
		PhysAssetHelper->CreatePhysicsAssetInternal();
		return true;
	}
	return false;
}

bool UPhysicsAssetTools::CreatePhysicsAssetInternal()
{
	UUnrealEditorSubsystem* UnrealEditorSubsystem = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
	
	// Spawn skeletal mesh actor, load AnimInstance, set curve and morph values
	if(!UnrealEditorSubsystem){
		LGE("Editor Subsystem not found.")
		return false; }
	
	UWorld* World = UnrealEditorSubsystem->GetEditorWorld();
	ASkeletalMeshActor* MeshActor = World->SpawnActor<ASkeletalMeshActor>();
	if(!MeshActor) {
		LGE("SkeletalMeshActor unable to be created.")
		return false; }
	
	SkelMeshComp = MeshActor->GetSkeletalMeshComponent();
	if(!SkelMeshComp) {
		LGE("SkeletalMeshComp unable to be created.")
		return false; }

	auto SourceSkeletalMesh = PcActorDataAsset->SourceSkeletalMesh.LoadSynchronous();
	if(!SourceSkeletalMesh) {
		LGE("SourceSkeletalMesh unable to be created.")
		return false; }
	
	SkelMeshComp->SetSkeletalMesh(SourceSkeletalMesh);
	SkelMeshComp->SetPhysicsAsset(TargetPhysicsAsset);
	
	SkelMeshComp->SetUpdateAnimationInEditor(true);
	
	SkelMeshComp->SetAnimInstanceClass(PcActorDataAsset->AnimBlueprintRef->GetAnimBlueprintGeneratedClass());
	auto AnimInstance = SkelMeshComp->GetAnimInstance();

	// auto AnimInstance = SkelMeshComp->GetPostProcessInstance();
	SkelMeshComp->SetDisablePostProcessBlueprint(true);

	if(!AnimInstance) {
		LGE("PostProcessAnimInstance unable to be created.")
		return false; }
	AnimInstance->InitializeAnimation();
	
	// for(auto MorphPair : PcActorDataAsset->MorphTargetMap)
	// {
	// 	AnimInstance->SetMorphTarget(MorphPair.Key, MorphPair.Value);
	// }
	// for(auto CurvePair : PcActorDataAsset->CurveMap)
	// {
	// 	AnimInstance->AddCurveValue(CurvePair.Key, CurvePair.Value);
	// }
	if(UPcAnimInstance* PcAnimInstance = Cast<UPcAnimInstance>(AnimInstance))
	{
		// PcAnimInstance->SetCurveMap(PcActorDataAsset->CurveMap);
		PcAnimInstance->CurveMap = PcActorDataAsset->CurveMap;
		PcAnimInstance->MorphTargetMap = PcActorDataAsset->MorphTargetMap;
	}
	AnimInstance->RefreshCurves(SkelMeshComp);
	AnimInstance->UpdateAnimation(.1f, false);
	// SkelMeshComp->TickAnimation(0.1f,false);
	SkelMeshComp->TickAnimation(0.2f,false);
	
	// NewSkeletalMesh = CopyMeshWithMorphs(SkelMeshComp, PcActorDataAsset);
	// if(!NewSkeletalMesh) {
	// 	LGE("NewSkeletalMesh unable to be created.")
	// 	return false; }
	// const auto RefSkeleton = NewSkeletalMesh->GetRefSkeleton();
	// auto BoneInfos = RefSkeleton.GetRefBoneInfo();
	// auto RefPose = RefSkeleton.GetRefBonePose();
	// for (int i = 0; i < BoneInfos.Num() && i < RefPose.Num(); i++)
	// {
	// 	PcActorDataAsset->RefPoseTransforms.Add(BoneInfos[i].Name, RefPose[i]);
	// }
	// // UPhysicsAsset* PhysicsAsset = PcActorDataAsset->PhysicsAsset;
	// SkelMeshComp->SetSkeletalMesh(NewSkeletalMesh);
	// TargetPhysicsAsset->SetPreviewMesh(NewSkeletalMesh);
	// LG("Created New Skeletal Mesh")
	
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UPhysicsAssetTools::CreatePhysicsAssetDelayed);
	GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>()->GetEditorWorld()->GetTimerManager()
		.SetTimer(TimerHandle, this, &UPhysicsAssetTools::CreatePhysicsAssetDelayed, 3.f, false );
	
	return true;
}

// bool UPhysAssetHelper::CreatePhysicsAssetDelayed(USkeletalMeshComponent* SkelMeshComp, UPcActorDataAsset* DataAsset)
void UPhysicsAssetTools::CreatePhysicsAssetDelayed()
{
	NewSkeletalMesh = CopyMeshWithMorphs(SkelMeshComp, PcActorDataAsset);
	if(!NewSkeletalMesh) {
		LGE("NewSkeletalMesh unable to be created.")
		return; }
	PcActorDataAsset->TargetSkeletalMesh = NewSkeletalMesh;
	const auto RefSkeleton = NewSkeletalMesh->GetRefSkeleton();
	auto BoneInfos = RefSkeleton.GetRefBoneInfo();
	auto RefPose = RefSkeleton.GetRefBonePose();
	for (int i = 0; i < BoneInfos.Num() && i < RefPose.Num(); i++)
	{
		PcActorDataAsset->RefPoseTransforms.Add(BoneInfos[i].Name, RefPose[i]);
	}
	// UPhysicsAsset* PhysicsAsset = PcActorDataAsset->PhysicsAsset;
	SkelMeshComp->SetSkeletalMesh(NewSkeletalMesh);
	TargetPhysicsAsset->SetPreviewMesh(NewSkeletalMesh);
	LG("Created New Skeletal Mesh")

	LG("Saving Package")
	UEditorAssetLibrary::SaveLoadedAssets({TargetPhysicsAsset, NewSkeletalMesh}, false);	

	SkelMeshComp->DestroyComponent();
	
	// if (CreateBodiesFromDataTable(PcActorDataAsset->BodyParamsDataTable, PhysicsAsset, NewSkeletalMesh))
	// {
	// 	if (CopyTwistShapesToParents(PhysicsAsset, NewSkeletalMesh, TwistBoneNames, false))
	// 	{
	// 		auto BonesToAlign = GetBoneNamesToAlign(PcActorDataAsset->BodyParamsDataTable);
	// 		if(AlignCapsulesToBones(PhysicsAsset, NewSkeletalMesh, TArray<FName>()))
	// 		{
	// 			LG("Aligned Capsule Bones succeeded. ")
	// 		}
	// 	}
	// }
	// else
	// {
	// 	
	// }
	return;/**/
}

USkeletalMesh* UPhysicsAssetTools::CopyMeshWithMorphs(USkeletalMeshComponent* SkeletalMeshComponent, UPcActorDataAsset* DataAsset)
{
	if(!SkeletalMeshComponent)
	{
		LGE("No SkeletalMeshComponent")
		return nullptr;
	}
	UE::Geometry::FDynamicMesh3 DynamicMesh;
	FTransform OutTransform;
	FText OutErrorMsg;
	UE::Conversion::FToMeshOptions Options = UE::Conversion::FToMeshOptions();

	// SetRefPoseOverride(SkeletalMeshComponent, TArray<FTransform>());
	
	// Copy the mesh from the SkeletalMeshComponent to the DynamicMesh
	UE::Conversion::SceneComponentToDynamicMesh(SkeletalMeshComponent, Options, false, DynamicMesh, OutTransform, OutErrorMsg);
	
	USkeleton* Skeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton();
	FReferenceSkeleton RefSkeleton = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetRefSkeleton();
	// auto AnimInstance = SkeletalMeshComponent->GetPostProcessInstance();
	auto AnimInstance = SkeletalMeshComponent->GetAnimInstance();

	
	if(AnimInstance)
	{
		FReferenceSkeletonModifier RefSkelModifier = FReferenceSkeletonModifier(RefSkeleton, Skeleton);
		AnimInstance->SavePoseSnapshot(FName("MorphedPose"));
		const FPoseSnapshot* Snapshot = AnimInstance->GetPoseSnapshot(FName("MorphedPose"));
		for(int i = 0; i < RefSkeleton.GetNum(); i++)
		{
			LG("Ori Transform %s: %s", *SkeletalMeshComponent->GetBoneName(i).ToString(), *RefSkeleton.GetRefBonePose()[i].ToString())
			LG("New Transform %s: %s", *SkeletalMeshComponent->GetBoneName(i).ToString(), *SkeletalMeshComponent->GetBoneSpaceTransforms()[i].ToString())
			if(Snapshot)
			{
				RefSkelModifier.UpdateRefPoseTransform(i, Snapshot->LocalTransforms[i]);
				LG("Sna Transform %s: %s", *SkeletalMeshComponent->GetBoneName(i).ToString(), *Snapshot->LocalTransforms[i].ToString())
			}
			else
				LGW("No snapshot.")
		}
	}

	UE::AssetUtils::FSkeletalMeshResults Results;
	UE::AssetUtils::FSkeletalMeshAssetOptions AssetOptions = UE::AssetUtils::FSkeletalMeshAssetOptions();
	AssetOptions.SourceMeshes.DynamicMeshes.Add(&DynamicMesh);
	AssetOptions.Skeleton = Skeleton;
	// AssetOptions.AssetMaterials = SkeletalMeshComponent->GetMaterials();
	AssetOptions.NumMaterialSlots = SkeletalMeshComponent->GetMaterials().Num();
	AssetOptions.RefSkeleton = &RefSkeleton;
	AssetOptions.NewAssetPath = MakeAssetPath(DataAsset->NewAssetPath, DataAsset->CharacterName.ToString(), "SKM_"); 
	
	UE::AssetUtils::CreateSkeletalMeshAsset(AssetOptions, Results);
	FString Result = Results.SkeletalMesh ? "Success" : "Failure";
	LG("Results: %s", *Result)
	return Results.SkeletalMesh;
}

bool UPhysicsAssetTools::CreateBodiesFromDataTable(UPcActorDataAsset* DataAsset)
{
	UDataTable* DataTable = DataAsset->BodyParamsDataTable.LoadSynchronous();
	if(!DataTable) {
		LGE("ERROR: DataTable not found. Aborting.");
		return false; }
	
	UPhysicsAsset* PhysicsAsset = DataAsset->TargetPhysicsAsset.LoadSynchronous();
	if(!PhysicsAsset) {
		LGE("ERROR: Target PhysicsAsset not found. Aborting.");
		return false; }
	
	USkeletalMesh* SkeletalMesh = DataAsset->TargetSkeletalMesh.LoadSynchronous();
	if(!SkeletalMesh) {
		LGE("ERROR: Target Skeletal Mesh not found. Aborting.");
		return false; }
	PhysicsAsset->SetPreviewMesh(SkeletalMesh);
	
	TArray<FPhysAssetCreateParamsRow> Rows;
	FString ContextString;
	TArray<FName> RowNames = DataTable->GetRowNames();

	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	TArray<FBoneVertInfo> Infos = TArray<FBoneVertInfo>();
	MeshUtilities.CalcBoneVertInfos(SkeletalMesh, Infos, true);

	FScopedSlowTask CreateBodiesTask = FScopedSlowTask(RowNames.Num(), NSLOCTEXT("CreateBodiesTask", "CreateBodies", "Adding Bodies..."));
	CreateBodiesTask.MakeDialog(true, true);
	
	for ( auto& BodyName : RowNames )
	{
		FText TaskText = FText::FromString(FString::Format(TEXT("Making body for bone {0}"), {BodyName.ToString()}));
		CreateBodiesTask.EnterProgressFrame(1, TaskText);

		if ( FPhysAssetCreateParamsRow* CreateParamsRow = DataTable->FindRow<FPhysAssetCreateParamsRow>(BodyName, ContextString) )
		{
			auto CreateParams = CreateParamsRow->GetCreateParams();
			int BodyIndex = PhysicsAsset->FindBodyIndex(BodyName);
			if (BodyIndex == INDEX_NONE)
			{
				LGW("Body %s not found on PhysicsAsset, trying to create it", *BodyName.ToString());
				TObjectPtr<USkeletalBodySetup> BodySetup = NewObject<USkeletalBodySetup>();
				BodySetup->BoneName = BodyName;
				PhysicsAsset->SkeletalBodySetups.Add(BodySetup);
				PhysicsAsset->UpdateBodySetupIndexMap();
				// Ensure it was added
				BodyIndex = PhysicsAsset->FindBodyIndex(BodyName);
				if (BodyIndex == INDEX_NONE) { LGW("Body %s not able to be created on PhysicsAsset", *BodyName.ToString());
					PhysicsAsset->SkeletalBodySetups.Pop();
					continue; }
			}
			
			USkeletalBodySetup* BodySetup = PhysicsAsset->SkeletalBodySetups[BodyIndex];
			FName BoneName = BodySetup->BoneName;
			int BoneIndex = SkeletalMesh->GetRefSkeleton().FindBoneIndex(BoneName);
			if(BoneIndex == INDEX_NONE) { LGW("Bone %s not found on Skeleton. Skipping this body.", *BodyName.ToString());
				continue;
			}	
			LG("Creating collision from bone %s", *BodyName.ToString());
			FPhysicsAssetUtils::CreateCollisionFromBone(BodySetup, SkeletalMesh, BoneIndex, CreateParams, Infos[BoneIndex]);

			if(CreateParams.GeomType == EFG_Sphyl)
			{
				DataAsset->CapsuleNames.Add(BodyName);	
			}
		}
	}
	
	PhysicsAsset->UpdateBodySetupIndexMap();
	PhysicsAsset->UpdateBoundsBodiesArray();
	LogBodies(PhysicsAsset);
	PhysicsAsset->RefreshPhysicsAssetChange();
	PhysicsAsset->MarkPackageDirty();
	// UEditorAssetLibrary::SaveLoadedAsset(PhysicsAsset, false);
	
	return true;
	
}



bool UPhysicsAssetTools::AlignCapsulesToBones(UPcActorDataAsset* DataAsset)
{
	LG("Aligning capsules to bones.")
	
	if(!DataAsset) {
		LGE("ERROR: Data Asset not found. Aborting.");
		return false; }
	
	UPhysicsAsset* PhysicsAsset = DataAsset->TargetPhysicsAsset.LoadSynchronous();
	if(!PhysicsAsset) {
		LGE("ERROR: Target PhysicsAsset not found. Aborting.");
		return false; }
	
	USkeletalMesh* SkeletalMesh = DataAsset->TargetSkeletalMesh.LoadSynchronous();
	if(!SkeletalMesh) {
		LGE("ERROR: Target SkeletalMesh not found. Aborting.");
		return false; }
	
	if(DataAsset->CapsuleNames.Num() == 0)
	{
		// TODO: find capsule bones from PhysicsAsset
		LGE("ERROR: No Capsule Names found. Aborting.");
	}
	
	TArray<FBoneVertInfo> Infos = TArray<FBoneVertInfo>();
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	MeshUtilities.CalcBoneVertInfos(SkeletalMesh, Infos, true);
	for(const auto Name : DataAsset->CapsuleNames)
	{
		AlignCapsule(PhysicsAsset, SkeletalMesh, Name, Infos);
	}
	
	PhysicsAsset->RefreshPhysicsAssetChange();
	PhysicsAsset->MarkPackageDirty();
	LogBodies(PhysicsAsset);
	return true;
}

bool UPhysicsAssetTools::CopyTwistShapesToParents(UPcActorDataAsset* DataAsset, bool bDeleteChildBodies = false)
{
	LG("Copying Twist bones to parents.")

	if(!DataAsset) {
		LGE("ERROR: Data Asset not found. Aborting.");
		return false; }
	
	UPhysicsAsset* PhysicsAsset = DataAsset->TargetPhysicsAsset.LoadSynchronous();
	if(!PhysicsAsset) {
		LGE("ERROR: Target PhysicsAsset not found. Aborting.");
		return false; }
	
	USkeletalMesh* SkeletalMesh = DataAsset->TargetSkeletalMesh.LoadSynchronous();
	if(!SkeletalMesh) {
		LGE("ERROR: Target Skeletal Mesh not found. Aborting.");
		return false; }
	
	if(DataAsset->TwistNames.Num() == 0)
	{
		DataAsset->TwistNames = TwistBoneNames;
	}
	for(FName BoneName : DataAsset->TwistNames)
	{
		LG("Copying twist shape from %s", *BoneName.ToString())
		CopyTwistShapeToParent(PhysicsAsset, SkeletalMesh, BoneName, bDeleteChildBodies);
	}
	if(bDeleteChildBodies)
	{
		for(int i = PhysicsAsset->SkeletalBodySetups.Num() - 1; i >= 0; i--)
		{
			if (DataAsset->TwistNames.Contains(PhysicsAsset->SkeletalBodySetups[i]->BoneName))
			{
				LG("Deleting body %s from Physics Asset", *PhysicsAsset->SkeletalBodySetups[i]->BoneName.ToString())
				PhysicsAsset->SkeletalBodySetups.RemoveAt(i);
			}
		}
		PhysicsAsset->UpdateBodySetupIndexMap();
		PhysicsAsset->UpdateBoundsBodiesArray();
		LogBodies(PhysicsAsset);
		PhysicsAsset->MarkPackageDirty();
	}
	
	return true;
}

bool UPhysicsAssetTools::CopyTwistShapeToParent(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkeletalMesh,
                                              FName BodyName, bool bDeleteChildBody = false)
{
	FReferenceSkeleton RefSkeleton = SkeletalMesh->GetRefSkeleton();
	auto RefBonePose = RefSkeleton.GetRefBonePose();
	int BodyIndex = PhysicsAsset->FindBodyIndex(BodyName);
	if (BodyIndex == INDEX_NONE) {
		LG("BodyName %s can't be found in Physics Asset. Aborting.", *BodyName.ToString());
		return false;
	}
	int BoneIndex = RefSkeleton.FindBoneIndex(BodyName);
	if (BoneIndex == INDEX_NONE) {
		LG("BodyName %s can't be found on skeletal mesh. Aborting.", *BodyName.ToString());
		return false;
	}
	int ParentBoneIndex = RefSkeleton.GetParentIndex(BoneIndex);
	if (ParentBoneIndex == INDEX_NONE) {
		LG("Body %s's parent can't be found on skeletal mesh. Aborting.", *BodyName.ToString());
		return false;
	}
	FName ParentBoneName = RefSkeleton.GetBoneName(ParentBoneIndex);
	int ParentBodyIndex = PhysicsAsset->FindBodyIndex(ParentBoneName);
	if (ParentBodyIndex == INDEX_NONE) {
		LG("Body %s's parent can't be found on Physics Asset. Aborting.", *BodyName.ToString());
		return false;
	}
	FTransform BoneTransform = RefBonePose[BoneIndex];
	FTransform ParentBoneTransform = RefBonePose[ParentBoneIndex];

	USkeletalBodySetup* Body = PhysicsAsset->SkeletalBodySetups[BodyIndex];
	USkeletalBodySetup* ParentBody = PhysicsAsset->SkeletalBodySetups[ParentBodyIndex];

	FKAggregateGeom* AggGeom = &Body->AggGeom;
	FKAggregateGeom* ParentAggGeom = &ParentBody->AggGeom;
	
	if (AggGeom->ConvexElems.IsValidIndex(0)) {
		FKConvexElem Shape = AggGeom->ConvexElems[0];
		FKConvexElem NewShape = FKConvexElem(Shape);

		// FTransform ShapeLocal = Shape.GetTransform();
		// FTransform ShapeWorld = ShapeLocal * BoneTransform;
		// FTransform ShapeLocalParent = ShapeWorld * ParentBoneTransform.Inverse();
		// NewShape.SetTransform(ShapeLocalParent);
		// NewShape.SetTransform(BoneTransform);

		//ParentAggGeom->ConvexElems.Add(NewShape);
		ParentBody->AddCollisionElemFrom(*AggGeom, EAggCollisionShape::Convex, 0);
		//ParentBody->AddCollisionFrom(*AggGeom);
		// ParentAggGeom->ConvexElems.Last().SetTransform(ShapeLocalParent);
		ParentAggGeom->ConvexElems.Last().SetTransform(BoneTransform);
		ParentBody->CreatePhysicsMeshes();
		return true;
	}
	else {
		LGW("No convex elements found on body %s. Aborting.", *BodyName.ToString());
		return false;
	}
}

bool UPhysicsAssetTools::AlignCapsuleToBone(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkeletalMesh,
                                          FName BodyName)
{
	TArray<FBoneVertInfo> Infos = TArray<FBoneVertInfo>();
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	MeshUtilities.CalcBoneVertInfos(SkeletalMesh, Infos, true);

	return AlignCapsule(PhysicsAsset, SkeletalMesh, BodyName, Infos);
}

bool UPhysicsAssetTools::AlignCapsule(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkeletalMesh, FName BodyName,
                                    TArray<FBoneVertInfo> Infos)
{
	const float	MinPrimSize = 0.5f;
	float Distance = 0.f;
	FKSphylElem Capsule;
	FTransform BoneTransform, OtherTransform;

	FReferenceSkeleton RefSkeleton = SkeletalMesh->GetRefSkeleton();
	int32 BodyIndex = PhysicsAsset->FindBodyIndex(BodyName);
	if (BodyIndex == INDEX_NONE) {
		LGW("BodyName %s can't be found in Physics Asset. Aborting.", *BodyName.ToString());
		return false;
	}
	USkeletalBodySetup* Body = PhysicsAsset->SkeletalBodySetups[BodyIndex];

	FName BoneName = Body->BoneName;
	int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
	if (BoneIndex == INDEX_NONE) {
		LGW("BodyName %s can't be found on skeletal mesh. Aborting.", *BodyName.ToString());
		return false;
	}
	FBoneVertInfo Info = Infos[BoneIndex];

	
	BoneTransform = RefSkeleton.GetRefBonePose()[BoneIndex];
	LGV("Body %s, Distance: %f", *BodyName.ToString(), Distance);


	/***********************************|***|***|***|****|*****|** 
	* Copied from PhysicsAssetUtils.cpp |   |   |   |    |     |
	************************************v***v***v***v****v*****v**/
	FMatrix ElemTM = FMatrix::Identity;
	bool ComputeFromVerts = false;

	if (true)
	{
		// Compute covariance matrix for verts of this bone
		// Then use axis with largest variance for orienting bone box
		const FMatrix CovarianceMatrix = ComputeCovarianceMatrix(Info);
		FVector ZAxis = ComputeEigenVector(CovarianceMatrix);
		FVector XAxis, YAxis;
		ZAxis.FindBestAxisVectors(YAxis, XAxis); 
		ElemTM = FMatrix(XAxis, YAxis, ZAxis, FVector::ZeroVector);
	}

	FTransform ElemTransform(ElemTM);

	FBox BoneBox(ForceInit);
	for (int32 j = 0; j < Info.Positions.Num(); j++)
	{
		BoneBox += ElemTransform.InverseTransformPosition((FVector)Info.Positions[j]);
	}

	FVector BoxCenter(0, 0, 0), BoxExtent(0, 0, 0);

	FBox TransformedBox = BoneBox;
	if (BoneBox.IsValid)
	{
		// make sure to apply scale to the box size
		FMatrix BoneMatrix = SkeletalMesh->GetComposedRefPoseMatrix(BoneIndex);
		TransformedBox = BoneBox.TransformBy(FTransform(BoneMatrix));
		BoneBox.GetCenterAndExtents(BoxCenter, BoxExtent);
	}


	LGV("Body %s, Center: %s", *BodyName.ToString(), *BoxCenter.ToCompactString());
	LGV("Body %s, BoxExtent: %s", *BodyName.ToString(), *BoxExtent.ToCompactString());

	float MinRad = TransformedBox.GetExtent().GetMin();
	float MinAllowedSize = MinPrimSize;


	// If the primitive is going to be too small - just use some default numbers and let the user tweak.
	if (MinRad < MinAllowedSize)
	{
		// change min allowed size to be min, not DefaultPrimSize
		BoxExtent = FVector(MinAllowedSize, MinAllowedSize, MinAllowedSize);
	}

	FVector BoneOrigin = BoneTransform.TransformPosition(BoxCenter);
	//BoneTransform.SetTranslation(BoneOrigin);

	if (BoxExtent.X > BoxExtent.Z && BoxExtent.X > BoxExtent.Y) {
		//X is the biggest so we must rotate X-axis into Z-axis
		//Capsule.SetTransform(FTransform(FQuat(FVector(0, 1, 0), -PI * 0.5f)) * BoneTransform);
		Capsule.Radius = FMath::Min(BoxExtent.Y, BoxExtent.Z) * 1.01f;
		//Capsule.Length = Distance;// BoxExtent.X * 1.01f;

	} else if (BoxExtent.Y > BoxExtent.Z && BoxExtent.Y > BoxExtent.X) {
		//Y is the biggest so we must rotate Y-axis into Z-axis
		//Capsule.SetTransform(FTransform(FQuat(FVector(1, 0, 0), PI * 0.5f)) * BoneTransform);
		//Capsule.Radius = FMath::Max(BoxExtent.X, BoxExtent.Z) * 1.01f;
		Capsule.Radius = FMath::Min(BoxExtent.X, BoxExtent.Z) * 1.01f;
		//Capsule.Length = Distance;// BoxExtent.Y * 1.01f;
	} else {
		//Z is the biggest so use transform as is
		//Capsule.SetTransform(BoneTransform);

		Capsule.Radius = FMath::Min(BoxExtent.X, BoxExtent.Y) * 1.01f;
		//Capsule.Length =  Distance;//BoxExtent.Z * 1.01f;
	}


	if (true) {
		//UE_LOG(LogCore, Warning, "Body {}, tranlation: {}", BodyName, BoneTransform.GetLocation());
		FVector BoneLocation = BoneTransform.GetTranslation();
		//UE_LOGFMT(LogCore, Warning, "Body {name}, Location: {vector}", BodyName, BoneLocation.ToCompactString());
		TArray<int32> ChildIndices = TArray<int32>();
		if (RefSkeleton.GetDirectChildBones(BoneIndex, ChildIndices)) {
			OtherTransform = RefSkeleton.GetRefBonePose()[ChildIndices[0]];
			Distance = FVector::Dist(BoneLocation, OtherTransform.GetTranslation());
		}
		else {
			OtherTransform = RefSkeleton.GetRefBonePose()[RefSkeleton.GetParentIndex(BoneIndex)];
			//UE_LOGFMT(LogCore, Warning, "Other Transform, Location: {vector}", OtherTransform.GetTranslation().ToCompactString());
			Distance = FVector::Dist(BoneLocation, OtherTransform.GetTranslation()) * 2 / 3;
		}
		Capsule.Length = Distance;
	}

	Capsule.SetTransform(BoneTransform);
	float xDisplacement = BodyName.ToString().EndsWith("r") ? (-Distance / 2) : Distance / 2;
	Capsule.Center = FVector(xDisplacement, 0, 0);
	//Capsule.Radius = Distance *  .3;
	Capsule.Rotation = FRotator(90, 0, 0);

	FKAggregateGeom* AggGeom = &Body->AggGeom;
	if (AggGeom->SphylElems.IsValidIndex(0))\
		AggGeom->SphylElems[0] = Capsule;
	else {
		AggGeom->SphylElems.Add(Capsule);
	}
	

	return true;
}


