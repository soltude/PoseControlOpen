// Fill out your copyright notice in the Description page of Project Settings.


#include "PhysicsEditorBPLibrary.h"
#include "Utils.h"
#include "AnimationEditorPreviewActor.h"
#include "AssetViewUtils.h"
#include "CustomLogging.h"
#include "DynamicMeshBuilder.h"
#include "FileHelpers.h"
#include "MeshUtilities.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/ConvexElem.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "MeshUtilitiesCommon.h"
#include "Animation/PcAnimInstance.h"
#include "Animation/SkeletalMeshActor.h"
#include "AssetUtils/CreateSkeletalMeshUtil.h"
#include "EditorAssetLibrary.h"
#include "IPhysicsAssetEditor.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "EditorScriptingUtilities/Private/EditorScriptingUtils.h"
#include "ConversionUtils/SceneComponentToDynamicMesh.h"
#include "DataAssets/PcActorDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#include "Toolkits/ToolkitManager.h"
#include "UObject/PackageRelocation.h"

TArray<FName> TwistBoneNames = { FName("upperarm_twist_01_l"), FName("upperarm_twist_02_l"), FName("upperarm_twist_01_r"), FName("upperarm_twist_02_r"), FName("lowerarm_twist_01_l"), FName("lowerarm_twist_02_l"), FName("lowerarm_twist_01_r"), FName("lowerarm_twist_02_r"), FName("thigh_twist_01_l"), FName("thigh_twist_02_l"), FName("thigh_twist_01_r"), FName("thigh_twist_02_r") };

UPhysicsEditorBPLibrary::UPhysicsEditorBPLibrary()
{
	if(!BodyParamsDataTable)
	{
		static ConstructorHelpers::FObjectFinder<UDataTable> BodyParamsDT(TEXT("/Script/Engine.DataTable'/PoseControl/Data/DT_BodyParams.DT_BodyParams'"));
		BodyParamsDataTable =  BodyParamsDT.Object;
	}
	
}

FString MakeAssetPath(FString Directory, FString Filename, FString Prefix = FString())
{
	if(Directory[Directory.Len() - 1] != '/')
		Directory += "/";
	return Directory + Prefix + Filename;
}

bool UPhysicsEditorBPLibrary::SetDataAsset(UPcActorDataAsset* DataAsset)
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

bool UPhysicsEditorBPLibrary::ProcessCharacterDA(UPcActorDataAsset* DataAsset, FMeshBakeOptions Options)
{
	CopyPhysicsAsset(DataAsset, Options.bOverwritePhysicsAsset);
	return true;
}

bool UPhysicsEditorBPLibrary::CopyPhysicsAsset(UPcActorDataAsset* DataAsset, bool bForceCopy = false)
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

UDataTable* UPhysicsEditorBPLibrary::GetBodySetupsDataTable() const
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
					OutNames.Add(Row->BoneName);
				}
			}
		}
	}
	return OutNames;
}

bool UPhysicsEditorBPLibrary::CreatePhysicsAssetAndMorphedSkm(UPcActorDataAsset* DataAsset)
{
	UPhysicsEditorBPLibrary* PhysAssetHelper = NewObject<UPhysicsEditorBPLibrary>();
	PhysAssetHelper->SetDataAsset(DataAsset);
	if (PhysAssetHelper->bPhysAssetHelperValid)
	{
		PhysAssetHelper->CreatePhysicsAssetInternal();
		return true;
	}
	return false;
}

bool UPhysicsEditorBPLibrary::CreatePhysicsAssetInternal()
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
	
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &UPhysicsEditorBPLibrary::CreatePhysicsAssetDelayed);
	GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>()->GetEditorWorld()->GetTimerManager()
		.SetTimer(TimerHandle, this, &UPhysicsEditorBPLibrary::CreatePhysicsAssetDelayed, 3.f, false );
	
	return true;
}

// bool UPhysAssetHelper::CreatePhysicsAssetDelayed(USkeletalMeshComponent* SkelMeshComp, UPcActorDataAsset* DataAsset)
void UPhysicsEditorBPLibrary::CreatePhysicsAssetDelayed()
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

USkeletalMesh* UPhysicsEditorBPLibrary::CopyMeshWithMorphs(USkeletalMeshComponent* SkeletalMeshComponent, UPcActorDataAsset* DataAsset)
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


bool UPhysicsEditorBPLibrary::SetRefPoseOverride(USkeletalMeshComponent* SkeletalMeshComponent, TArray<FTransform> NewRefPoseTransforms)
// const TArray<FTransform>& NewRefPoseTransforms)
{
	if(SkeletalMeshComponent)
	{
		
		if(NewRefPoseTransforms.Num() != SkeletalMeshComponent->GetNumBones())
		{
			LG("Skel Mesh has %d bones, Transform array has %d bones. Using current pose.")
			NewRefPoseTransforms = SkeletalMeshComponent->GetBoneSpaceTransforms();
		}
		LG("Setting Ref Pose Override");
		SkeletalMeshComponent->SetRefPoseOverride(NewRefPoseTransforms);
		return true;
	}
	return false;
}

bool UPhysicsEditorBPLibrary::CreateBodiesFromDataTable(UPcActorDataAsset* DataAsset)
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
		
		FPhysAssetCreateParamsRow* Row = DataTable->FindRow<FPhysAssetCreateParamsRow>(BodyName, ContextString);
		if ( Row )
		{
			int BodyIndex = PhysicsAsset->FindBodyIndex(BodyName);
			if (BodyIndex == INDEX_NONE)
			{
				LGW("Body %s not found on PhysicsAsset, trying to create it", *BodyName.ToString());
				TObjectPtr<USkeletalBodySetup> BodySetup = NewObject<USkeletalBodySetup>();
				BodySetup->BoneName = BodyName;
				PhysicsAsset->SkeletalBodySetups.Add(BodySetup);
				PhysicsAsset->UpdateBodySetupIndexMap();
				PhysicsAsset->UpdateBoundsBodiesArray();

				// Ensure it was added
				BodyIndex = PhysicsAsset->FindBodyIndex(BodyName);
				if (BodyIndex == INDEX_NONE) { LGW("Body %s not able to be created on PhysicsAsset", *BodyName.ToString());
					continue; }
			}
			
			FPhysAssetCreateParams CreateParams = Row->GetCreateParams();
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

	UEditorAssetLibrary::SaveLoadedAsset(PhysicsAsset, false);
	
	return true;
	
}

bool UPhysicsEditorBPLibrary::FixConstraintScale(UPhysicsAsset* PhysicsAsset)
{
	for (int i = 0; i < PhysicsAsset->ConstraintSetup.Num(); i++)
	{
		if (PhysicsAsset->ConstraintSetup[i])
		{
			UPhysicsConstraintTemplate* ConstraintTemplate = PhysicsAsset->ConstraintSetup[i];
			FConstraintInstance* Constraint = &ConstraintTemplate->DefaultInstance;
			//FConstraintInstance* constraint = PhysicsAsset->GetConstraintInstanceByIndex(i);

			for (int j = 0; j < 2; j++)
			{
				EConstraintFrame::Type Frame = j == 0 ? EConstraintFrame::Frame1 : EConstraintFrame::Frame2;
				FTransform Tform = Constraint->GetRefFrame(Frame);
				if (!Tform.Scale3DEquals(FTransform::Identity))
				{
					LG("constraint %s frame %d, scale: %s", *Constraint->JointName.ToString(), j + 1, *Tform.GetScale3D().ToCompactString());
					Tform.SetScale3D(FVector::OneVector);
					Constraint->SetRefFrame(Frame, Tform);
				}
			}
		}
	}
	return true;
}

bool UPhysicsEditorBPLibrary::AlignCapsulesToBones(UPcActorDataAsset* DataAsset)
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
	
	return true;
}

bool UPhysicsEditorBPLibrary::CopyTwistShapesToParents(UPcActorDataAsset* DataAsset, bool bDeleteChildBodies = false)
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
	}
	
	return true;
}

bool UPhysicsEditorBPLibrary::CopyTwistShapeToParent(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkeletalMesh,
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

bool UPhysicsEditorBPLibrary::AlignCapsuleToBone(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkeletalMesh,
                                          FName BodyName)
{
	TArray<FBoneVertInfo> Infos = TArray<FBoneVertInfo>();
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	MeshUtilities.CalcBoneVertInfos(SkeletalMesh, Infos, true);

	return AlignCapsule(PhysicsAsset, SkeletalMesh, BodyName, Infos);
}

bool UPhysicsEditorBPLibrary::AlignCapsule(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkeletalMesh, FName BodyName,
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
	LGW("Body %s, Distance: %f", *BodyName.ToString(), Distance);


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


	LG("Body %s, Center: %s", *BodyName.ToString(), *BoxCenter.ToCompactString());
	LG("Body %s, BoxExtent: %s", *BodyName.ToString(), *BoxExtent.ToCompactString());

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

bool UPhysicsEditorBPLibrary::AlignConstraint(USkeletalMeshComponent* SkelMesh, int32 ConstraintIndex,
	float PositionBlend, float OrientationBlend)
{
	if(UPhysicsAsset* PhysicsAsset = SkelMesh->GetPhysicsAsset())
	{
		if(UPhysicsConstraintTemplate* Setup = PhysicsAsset->ConstraintSetup[ConstraintIndex])
		{
			if(FConstraintInstance* Constraint = &Setup->DefaultInstance)
			{
				FBodyInstance* Body1 = SkelMesh->GetBodyInstance(Constraint->ConstraintBone1);
				FBodyInstance* Body2 = SkelMesh->GetBodyInstance(Constraint->ConstraintBone2);
				if(Body1 && Body2)
				{
					FVector Pos1 = SkelMesh->GetBoneLocation(SkelMesh->GetBoneName(Body1->InstanceBoneIndex));
					FVector Pos2 = SkelMesh->GetBoneLocation(SkelMesh->GetBoneName(Body2->InstanceBoneIndex));
					FTransform xform1 = Constraint->GetRefFrame(EConstraintFrame::Frame1);
					FTransform xform2 = Constraint->GetRefFrame(EConstraintFrame::Frame2);
					
					FVector PosFinal = FMath::Lerp(xform1.GetLocation(), xform2.GetLocation(), PositionBlend);
					FTransform xformFinal = FTransform(PosFinal);
					Constraint->SetRefFrame(EConstraintFrame::Frame1, xform1.GetRelativeTransform(xformFinal));
					Constraint->SetRefFrame(EConstraintFrame::Frame2, xform2.GetRelativeTransform(xformFinal));
					// Constraint->SetRefPosition(EConstraintFrame::Frame1, );
					// Constraint->SetRefPosition(EConstraintFrame::Frame2, PosFinal - Pos2);
					Constraint->PhysScene = nullptr;
					Constraint->ConstraintHandle.Reset();
					// PhysicsAsset->ConstraintSetup[Constraint.ConstraintIndex]->DefaultInstance = Constraint;
					LG("Constraint %s position set to %s;  %s;  %s", *Constraint->JointName.ToString(), *(PosFinal - Pos1).ToCompactString(), *(PosFinal - Pos2).ToCompactString(), *PosFinal.ToCompactString())
					LG("  Pos1 %s;  Pos2 %s", *(Constraint->Pos1).ToCompactString(), *(Constraint->Pos2).ToCompactString())
					
					return true;
				}
			}
			LGE("One or more bodies not found in physics asset")
			return false;
		}
	}
	LGE("Physics asset not found");
	return false;
}

USkeletalMeshComponent* UPhysicsEditorBPLibrary::GetSkelMeshComponent(FPersonaAssetEditorToolkit* PhysicsAssetEditor)
{
	auto ToolKitHost = PhysicsAssetEditor->GetToolkitHost();
	if (UWorld* World = ToolKitHost->GetWorld())
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Actors);
		for(auto Actor : Actors)
		{
			if(auto Component = Actor->GetComponentByClass(UDebugSkelMeshComponent::StaticClass()))
			{
				return Cast<USkeletalMeshComponent>(Component);
			}	
		}
	}
	return nullptr;
}

bool UPhysicsEditorBPLibrary::AdjustConstraints(USkeletalMeshComponent* SkelMeshComp, FAdjustConstraintOptions Options)
{
	FRegexPattern ParentPattern = FRegexPattern(Options.MatchParentBodyRegex);	
	FRegexPattern ChildPattern = FRegexPattern(Options.MatchChildBodyRegex);	
	if(UPhysicsAsset* PhysicsAsset = SkelMeshComp->GetPhysicsAsset())
	{
		// TArray<FConstraintInstanceAccessor> Constraints;
		// PhysicsAsset->GetConstraints(false, Constraints);
		for(int i = 0; auto ConstraintTemplate : PhysicsAsset->ConstraintSetup)
		{
			if(!ConstraintTemplate)
				continue;
			auto Constraint = ConstraintTemplate->DefaultInstance;
			{
				if(RegexMatch(ChildPattern, Constraint.ConstraintBone1.ToString())
					&& RegexMatch(ParentPattern, Constraint.ConstraintBone2.ToString()))
				{
					auto ConstraintAccessor = FConstraintInstanceAccessor(SkelMeshComp, i);	
					AlignConstraint(SkelMeshComp, i, Options.PositionAlpha, 0.f);
				}
			}
			i++;
		}
		UEditorAssetLibrary::SaveLoadedAsset(PhysicsAsset, false);
	}

	return false;
}

bool UPhysicsEditorBPLibrary::AdjustBreastPointBody(USkeletalMeshComponent* SkelMeshComp, FAdjustBodiesOptions Options, FName BodyName)
{
	float PositionAlpha = .35f;
	float RadiusFactor = .45f;
	if(auto PhysicsAsset = SkelMeshComp->GetPhysicsAsset())
	{
		if(int* BodyIndex = PhysicsAsset->BodySetupIndexMap.Find(BodyName))
		{
			int ParentIndex = PhysicsAsset->FindParentBodyIndex(SkelMeshComp->GetSkeletalMeshAsset(), *BodyIndex);
			auto BodySetup = PhysicsAsset->SkeletalBodySetups[*BodyIndex];
			if(BodySetup)
			{
				auto BoneXform = SkelMeshComp->GetBoneTransform(BodySetup->BoneName);
				auto ParentXform = SkelMeshComp->GetBoneTransform(SkelMeshComp->GetParentBone(BodySetup->BoneName));
				float Distance = FVector::Dist(BoneXform.GetLocation(), ParentXform.GetLocation());
				if (!BodySetup->AggGeom.SphereElems.IsValidIndex(0))
					BodySetup->AggGeom.SphereElems.Add(FKSphereElem());
				FVector PosFinal = FMath::Lerp(BoneXform.GetLocation(), ParentXform.GetLocation(), PositionAlpha);
				// BodySetup->AggGeom.SphereElems[0].Center = BoneXform.GetRelativeTransform(FTransform(PosFinal)).GetLocation() * -1.f;
				BodySetup->AggGeom.SphereElems[0].Center = BoneXform.InverseTransformPosition(PosFinal);
				BodySetup->AggGeom.SphereElems[0].Radius = RadiusFactor * Distance;
				return true;
			}
		}
	}
	return false;
}

bool UPhysicsEditorBPLibrary::AdjustBodies(USkeletalMeshComponent* SkelMeshComp, FAdjustBodiesOptions Options)
{
	FRegexPattern BodyPattern = FRegexPattern(Options.MatchBodyRegex);	
	if(UPhysicsAsset* PhysicsAsset = SkelMeshComp->GetPhysicsAsset())
	{
		int i = 0;
		TArray<FName> BodyNames;
		PhysicsAsset->BodySetupIndexMap.GetKeys(BodyNames);
		for(auto BodyName : BodyNames)
		{
			if(RegexMatch(BodyPattern, BodyName.ToString()))
			{
				AdjustBreastPointBody(SkelMeshComp, Options, BodyName);
			}
			i++;
		}
		UEditorAssetLibrary::SaveLoadedAsset(PhysicsAsset, false);
		return true;
	}
	return false;
}

USkeletalMeshComponent* UPhysicsEditorBPLibrary::FocusOrOpenPhysAssetEditor(UPhysicsAsset* PhysicsAsset)
{
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	AssetEditorSubsystem->OpenEditorForAsset(PhysicsAsset);
	
	TArray<AActor*> Actors;
	auto WorldContexts = GEditor->GetWorldContexts();
	for(auto Context : WorldContexts)
	{
		UGameplayStatics::GetAllActorsOfClass(Context.World(), AAnimationEditorPreviewActor::StaticClass(), Actors);
		for(auto Actor : Actors)
		{
			if(auto Component = Actor->GetComponentByClass(UDebugSkelMeshComponent::StaticClass()))
			{
				auto SkelMeshComp = Cast<UDebugSkelMeshComponent>(Component);
				if(SkelMeshComp->GetPhysicsAsset() == PhysicsAsset)
				{
					return SkelMeshComp;
				}
			}	
		}
	}
	return nullptr;
}

bool UPhysicsEditorBPLibrary::AddBreastPointConstraints(USkeletalMeshComponent* SkelMeshComp, FAddConstraintsOptions Options)
{
	return AddBreastPointRingConstraints(SkelMeshComp, Options);	
}


FName UPhysicsEditorBPLibrary::MakeConstraintNameBreastRing(int n, int side, int ring, int point)
{
	char buffer2[40];
	sprintf_s(buffer2, "breast_ring_%02d_%02d_%02d_%c",ring+1, point+1, ((point+1)%n) + 1, side == 0 ? 'l' : 'r');
	return FName(buffer2);
}

FName UPhysicsEditorBPLibrary::MakeConstraintNameBreastUp(int side, int ring, int point) 
{
	char buffer2[40];
	sprintf_s(buffer2, "breast_up_%02d_%02d_%c",ring+1, point+1, side == 0 ? 'l' : 'r');
	return FName(buffer2);
}

inline int32 GetClosestPoint(FTransform Transform, TArray<FTransform> Points)
{
	float min_dist = MAX_FLT;
	float min_index = -1;
	for(int i = 0; i < Points.Num(); i++)
	{
		float dist = FVector::Dist(Transform.GetLocation(), Points[i].GetLocation());
		if(dist < min_dist)
		{
			min_dist = dist;
			min_index = i; 
		}
	}
	return min_index;	
}

bool UPhysicsEditorBPLibrary::AddBreastPointRingConstraints(USkeletalMeshComponent* SkelMeshComp,
                                                            FAddConstraintsOptions Options)
{
	TArray<FName> BodyNames;
	UPhysicsAsset* PhysicsAsset = SkelMeshComp->GetPhysicsAsset();
	PhysicsAsset->BodySetupIndexMap.GetKeys(BodyNames);
	int n_rings = 3;	
	for(int side = 0; side < 2; side++)
	{
		TArray<TArray<FName>> RingNamesList;
		TArray<TArray<FTransform>> RingTransformsList;
		for (int ring = 0; ring < n_rings; ring++)
		{
			TArray<FName> RingNames;
			RingNamesList.Add(RingNames);
			TArray<FTransform> RingTransforms;
			RingTransformsList.Add(RingTransforms);
			char buffer[30];
			sprintf_s(buffer,"breast_pt_%02d_.._%c", ring+1, side == 0 ? 'l' : 'r');
			FRegexPattern RegexPattern = FRegexPattern(FString(buffer));
			for(auto BodyName : BodyNames)
			{
				if(RegexMatch(RegexPattern, BodyName.ToString()))
				{
					if(auto BodySetup = PhysicsAsset->SkeletalBodySetups[PhysicsAsset->BodySetupIndexMap[BodyName]])
					{
						RingTransforms.Add( SkelMeshComp->GetBoneTransform(BodySetup->BoneName));
						RingNames.Add(BodyName);
					}
					LGE("Body Setup for %s not found", *BodyName.ToString())
				}
			}
			// RingNames.Sort();

			int n = RingNames.Num();
			for(int point = 0; point < n; point++)
			{
				// Add constraints between each point and its neighbors in a ring
				if (Options.bAddRingConstraints)
				{
					FName ConstraintBone1 = RingNames[point];
					FName ConstraintBone2 = RingNames[(point+1) % n];
					FName ConstraintName = MakeConstraintNameBreastRing(n, side, ring, point);
					UPhysicsConstraintTemplate* ConstraintSetup = MakeNewConstraint(
						PhysicsAsset, ConstraintName, ConstraintBone1, ConstraintBone2);
					// AlignConstraint(SkelMeshComp, ConstraintSetup->DefaultInstance.ConstraintIndex, .5f, .5f);
				}
				// Add constraints between each point and its neighbors in a ring
				if (Options.bAddRootConstraints)
				{
					FName ConstraintBone1 = RingNames[point];
					char Bone2[15];
					sprintf_s(Bone2, "breast_%02d_%c", ring+1, side ? 'l' : 'r');
					FName ConstraintBone2 = FName(Bone2);
					FName ConstraintName = ConstraintBone1;
					UPhysicsConstraintTemplate* ConstraintSetup = MakeNewConstraint(
						PhysicsAsset, ConstraintName, ConstraintBone1, ConstraintBone2);
					AlignConstraint(SkelMeshComp, ConstraintSetup->DefaultInstance.ConstraintIndex, .5f, .5f);
				}
			}
		} // end `ring` loop

		if (Options.bAddTangentConstraints)
		{
			// Add constraint from each point to closest point in the higher ring	
			for (int ring = 0; ring < n_rings; ring++)
			{
			
				TArray<FName> RingNames = RingNamesList[ring];
				FName ChildBodyName, ParentBodyName;
				for(int point = 0; point < RingNames.Num(); point++)
				{
					ParentBodyName = RingNamesList[ring][point];
					if(ring < n_rings - 1)
					{
						int32 ClosestIndex = GetClosestPoint(RingTransformsList[ring][point], RingTransformsList[ring+1]);
						ChildBodyName = RingNamesList[ring+1][ClosestIndex];
					} else {
						ChildBodyName = FName("breast_0" + FString::FromInt(n_rings + 1) + "_" + (side == 0 ? 'l' : 'r'));		
					}
					FName ConstraintName = MakeConstraintNameBreastUp(side, ring, point);
				
					UPhysicsConstraintTemplate* ConstraintSetup = MakeNewConstraint(
						PhysicsAsset, ConstraintName, ChildBodyName, ParentBodyName);
					
					// AlignConstraint(SkelMeshComp, ConstraintSetup->DefaultInstance.ConstraintIndex, .5f, .5f);
				}
			}
		}
		return true;
	}
	return false;
}


UPhysicsConstraintTemplate* UPhysicsEditorBPLibrary::MakeNewConstraint(UPhysicsAsset* PhysicsAsset,
                                                                       FName ConstraintName, FName ChildBodyName,
                                                                       FName ParentBodyName)
{
	int32 ChildBodyIndex = PhysicsAsset->FindBodyIndex(ChildBodyName);
	int32 ParentBodyIndex = PhysicsAsset->FindBodyIndex(ParentBodyName);
	UPhysicsConstraintTemplate* ConstraintSetup = MakeNewConstraint(
		PhysicsAsset, ConstraintName, ChildBodyIndex, ParentBodyIndex);
	return ConstraintSetup;
	
}
UPhysicsConstraintTemplate* UPhysicsEditorBPLibrary::MakeNewConstraint(UPhysicsAsset* PhysicsAsset,
                                                                       FName ConstraintName, int32 ChildBodyIndex,
                                                                       int32 ParentBodyIndex)
{
	int32 ConstraintIndex = PhysicsAsset->FindConstraintIndex(ConstraintName);
	if (ConstraintIndex == INDEX_NONE)
	{
		ConstraintIndex = FPhysicsAssetUtils::CreateNewConstraint(PhysicsAsset, ConstraintName);
	}
	if (ConstraintIndex != INDEX_NONE)
	{
		if (auto ConstraintSetup = PhysicsAsset->ConstraintSetup[ConstraintIndex])
		{
			ConstraintSetup->Modify(false);
			UBodySetup* ChildBodySetup = PhysicsAsset->SkeletalBodySetups[ ChildBodyIndex ];
			UBodySetup* ParentBodySetup = PhysicsAsset->SkeletalBodySetups[ ParentBodyIndex ];
			if(ChildBodySetup && ParentBodySetup)
			{
				FName ConstraintBone1 = ChildBodySetup->BoneName;
				FName ConstraintBone2 = ParentBodySetup->BoneName;
				ConstraintSetup->DefaultInstance.ConstraintBone1 = ConstraintBone1;
				ConstraintSetup->DefaultInstance.ConstraintBone2 =  ConstraintBone2;
				ConstraintSetup->SetDefaultProfile(ConstraintSetup->DefaultInstance);
				PhysicsAsset->DisableCollision(ChildBodyIndex, ParentBodyIndex);
				return ConstraintSetup;
			}
		}
	}
	return nullptr;
}


bool UPhysicsEditorBPLibrary::RegexMatch(FRegexPattern Pattern, FString Str)
{
	FRegexMatcher Matcher = FRegexMatcher(Pattern, Str);
	if (Matcher.FindNext())
		return true;
	return false;
}

FPhysAssetCreateParams FPhysAssetCreateParamsRow::GetCreateParams() const
{
	FPhysAssetCreateParams CreateParams;	
	CreateParams.MinBoneSize = MinBoneSize;
	CreateParams.MinWeldSize = MinWeldSize;
	CreateParams.GeomType = GeomType;
	CreateParams.VertWeight = VertWeight;
	CreateParams.bAutoOrientToBone = bAutoOrientToBone;
	CreateParams.bCreateConstraints = bCreateConstraints;
	CreateParams.bWalkPastSmall = true;
	CreateParams.bBodyForAll = bBodyForAll;
	CreateParams.bDisableCollisionsByDefault = bDisableCollisionsByDefault;
	CreateParams.AngularConstraintMode = AngularConstraintMode;
	CreateParams.HullCount = HullCount;
	CreateParams.MaxHullVerts = MaxHullVerts;
	CreateParams.LevelSetResolution = LevelSetResolution;
	CreateParams.LatticeResolution = LatticeResolution;
	return CreateParams;
}

void UPhysicsEditorBPLibrary::SaveAsset(FString AssetPath, bool& bOutSuccess, FString& OutInfoMessage)
{
	// Load the asset
	UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
	if (Asset == nullptr)
	{
		bOutSuccess = false;
		OutInfoMessage = FString::Printf(TEXT("Save Asset Failed – Asset is not valid '%s'"), *AssetPath);
		return;
	}
	// Get the package from the asset
	UPackage* Package = Asset->GetPackage();
	if (Package == nullptr)
	{
		bOutSuccess = false;
		OutInfoMessage = FString::Printf(TEXT("Save Asset Failed Package is not valid '%s'"), *AssetPath); return;
	}
	// Save the package
	bOutSuccess = UEditorLoadingAndSavingUtils::SavePackages({ Package }, false);
	OutInfoMessage = FString::Printf(TEXT("Save Asset %s - '%s'"), *FString(bOutSuccess? "Succeeded" : "Failed"), *AssetPath);
}
//
// void UPhysicsEditorBPLibrary::MarkAssetModified(FString AssetPath, bool& bOutSuccess, FString& OutInfoMessage)
// {
// }
//
// TArray<UObject*> UPhysicsEditorBPLibrary::GetModifiedAssets(bool& boutSuccess, FString& OutInfoMessage)
// {
// 	return {};
// }

