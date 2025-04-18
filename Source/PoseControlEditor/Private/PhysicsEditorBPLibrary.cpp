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

#undef LOG_CAT
#define LOG_CAT LogPhysicsEditor

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

bool UPhysicsEditorBPLibrary::AdjustConstraints(USkeletalMeshComponent* SkelMeshComp, FAdjustConstraintsOptions Options,
                                                TArray<FName> JointNames = TArray<FName>())
{
	
	if(UPhysicsAsset* PhysicsAsset = SkelMeshComp->GetPhysicsAsset())
	{
		if(JointNames.IsEmpty())
		{
			PhysicsAsset->BodySetupIndexMap.GetKeys(JointNames);
			JointNames = FilterNames(JointNames, Options.MatchChildBodyRegex);
		}
		for(int i = 0; auto ConstraintTemplate : PhysicsAsset->ConstraintSetup)
		{
			if(!ConstraintTemplate)
				continue;
			auto Constraint = ConstraintTemplate->DefaultInstance;
			{
				if(JointNames.Contains(Constraint.JointName))
				{
					LG("    Adjusting Constraint: %s", *Constraint.JointName.ToString())	
					AlignConstraint(SkelMeshComp, i, Options.PositionRatio, 0.f);
				}
			}
			i++;
		}
		UEditorAssetLibrary::SaveLoadedAsset(PhysicsAsset, false);
	}

	return false;
}

bool UPhysicsEditorBPLibrary::AdjustPointBody(USkeletalMeshComponent* SkelMeshComp, FAdjustBodiesOptions Options, FName BodyName)
{
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
				FVector PosFinal = FMath::Lerp(BoneXform.GetLocation(), ParentXform.GetLocation(), Options.PositionRatio);
				// BodySetup->AggGeom.SphereElems[0].Center = BoneXform.GetRelativeTransform(FTransform(PosFinal)).GetLocation() * -1.f;
				BodySetup->AggGeom.SphereElems[0].Center = BoneXform.InverseTransformPosition(PosFinal);
				BodySetup->AggGeom.SphereElems[0].Radius = Options.RadiusRatio * Distance;
				LG("     %s: %s -> %s = %s", *BodyName.ToString(), *BoneXform.GetLocation().ToCompactString(),
				   *PosFinal.ToCompactString(), *BoneXform.InverseTransformPosition(PosFinal).ToCompactString())
				return true;
			}
		}
	}
	return false;
}

bool UPhysicsEditorBPLibrary::AdjustBodies(USkeletalMeshComponent* SkelMeshComp, FAdjustBodiesOptions Options,
                                           TArray<FName> BodyNames = TArray<FName>())
{
	if(UPhysicsAsset* PhysicsAsset = SkelMeshComp->GetPhysicsAsset())
	{
		
		if(BodyNames.IsEmpty())
		{
			PhysicsAsset->BodySetupIndexMap.GetKeys(BodyNames);
			BodyNames = FilterNames(BodyNames, Options.MatchBodyRegex);
		}
		for(auto BodyName : BodyNames)
		{
			LG("    Adjusting Body: %s", *BodyName.ToString())	
			AdjustPointBody(SkelMeshComp, Options, BodyName);
		}
		PhysicsAsset->MarkPackageDirty();
		// UEditorAssetLibrary::SaveLoadedAsset(PhysicsAsset, false);
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

bool UPhysicsEditorBPLibrary::ApplyConstraintParams(UPhysicsAsset* PhysicsAsset, FConstraintParams Params)
{
		int32 Index = PhysicsAsset->FindConstraintIndex(Params.JointName);
		if(Index != INDEX_NONE)
		{
			auto Constraint = PhysicsAsset->ConstraintSetup[Index];
			float Mass = 0.f;
			int BodyIndex = PhysicsAsset->FindBodyIndex(Params.ConstraintBone1);
			if(BodyIndex != INDEX_NONE)
			{
				Mass = PhysicsAsset->SkeletalBodySetups[BodyIndex]->CalculateMass();
			}
			if( EnumHasAnyFlags(Params.ConstraintOverwrite, EConstraintOverwrite::RefFrames))
			{
				Constraint->DefaultInstance.SetRefFrame(EConstraintFrame::Frame1, Params.RefFrameNoScale1);
				Constraint->DefaultInstance.SetRefFrame(EConstraintFrame::Frame2, Params.RefFrameNoScale2);
			}
			if( EnumHasAnyFlags(Params.ConstraintOverwrite, EConstraintOverwrite::LinearLimits))
			{
				Constraint->DefaultInstance.SetLinearLimits(Params.LinearLimitedX, Params.LinearLimitedY,
															Params.LinearLimitedZ, Params.LinearLimit);
			}
			if( EnumHasAnyFlags(Params.ConstraintOverwrite, EConstraintOverwrite::LinearDrive))
			{
				float LinearStrength = (Params.LinearStrengthMassMultiplier > 0.f && Mass > 0.f)
					? Params.LinearStrengthMassMultiplier * Mass
					: Params.LinearStrength;
				Constraint->DefaultInstance.SetLinearDriveParams(LinearStrength, LinearStrength * Params.LinearDampingRatio, 0.f);
				Constraint->DefaultInstance.SetLinearPositionDrive(LinearStrength > 0.f,LinearStrength > 0.f,LinearStrength > 0.f);
				Constraint->DefaultInstance.SetLinearVelocityDrive(LinearStrength > 0.f,LinearStrength > 0.f,LinearStrength > 0.f);
				Constraint->DefaultInstance.SetLinearPositionTarget(Params.LinearTarget);
			}
			if( EnumHasAnyFlags(Params.ConstraintOverwrite, EConstraintOverwrite::AngularDrive))
			{
				Constraint->DefaultInstance.SetAngularTwistLimit(Params.TwistLimited, Params.TwistLimit);
				Constraint->DefaultInstance.SetAngularSwing1Limit(Params.Swing1Limited, Params.Swing1Limit);
				Constraint->DefaultInstance.SetAngularSwing2Limit(Params.Swing2Limited, Params.Swing2Limit);
			}
			if( EnumHasAnyFlags(Params.ConstraintOverwrite, EConstraintOverwrite::AngularDrive))
			{
				float AngularStrength = (Params.AngularStrengthMassMultiplier > 0.f && Mass > 0.f)
					? Params.AngularStrengthMassMultiplier * Mass
					: Params.AngularStrength;
				Constraint->DefaultInstance.SetAngularDriveParams(AngularStrength, AngularStrength * Params.AngularDampingRatio, 0.f);
				Constraint->DefaultInstance.SetAngularOrientationTarget(Params.AngularTarget.Quaternion());
				Constraint->DefaultInstance.SetAngularDriveMode(Params.bSlerp ? EAngularDriveMode::SLERP : EAngularDriveMode::TwistAndSwing);
			}
			return true;
		}
	LGE("Constraint %s not found", *Params.JointName.ToString())
	return false;
}

bool UPhysicsEditorBPLibrary::ApplyAllConstraintOptions(UPhysicsAsset* PhysicsAsset, FPhatConstraintOptions Options)
{
	for (auto ConstraintParams : Options.AllConstraintParams)
	{
		int32 Index = MakeNewConstraint(PhysicsAsset, ConstraintParams);
		if(Index != INDEX_NONE)
		{
			ApplyConstraintParams(PhysicsAsset, ConstraintParams);
		}
	}
	return true;
}



bool UPhysicsEditorBPLibrary::GetConstraintParams(UPhysicsAsset* PhysicsAsset, int32 ConstraintIndex, FConstraintParams& OutParams)
{
	if(PhysicsAsset && ConstraintIndex < PhysicsAsset->ConstraintSetup.Num())
	{
		float _;
		const auto Constraint = PhysicsAsset->ConstraintSetup[ConstraintIndex];

		int BodyIndex = PhysicsAsset->FindBodyIndex(Constraint->DefaultInstance.ConstraintBone1);
		float Mass = PhysicsAsset->SkeletalBodySetups[BodyIndex]->CalculateMass();
		
		OutParams.RefFrameNoScale1 = Constraint->DefaultInstance.GetRefFrame(EConstraintFrame::Frame1);
		OutParams.RefFrameNoScale2 = Constraint->DefaultInstance.GetRefFrame(EConstraintFrame::Frame2);
		
		OutParams.LinearLimitedX  = Constraint->DefaultInstance.GetLinearXMotion();
		OutParams.LinearLimitedY  = Constraint->DefaultInstance.GetLinearYMotion();
		OutParams.LinearLimitedZ  = Constraint->DefaultInstance.GetLinearZMotion();
		OutParams.LinearLimit = Constraint->DefaultInstance.GetLinearLimit();
		Constraint->DefaultInstance.GetLinearDriveParams(OutParams.LinearStrength, _, _);
		OutParams.LinearStrengthMassMultiplier = OutParams.LinearStrength / Mass;
		OutParams.LinearTarget = Constraint->DefaultInstance.GetLinearPositionTarget();
		
		Constraint->DefaultInstance.GetAngularDriveParams(OutParams.AngularStrength, _, _);
		OutParams.AngularStrengthMassMultiplier = OutParams.AngularStrength / Mass;
		OutParams.AngularTarget = Constraint->DefaultInstance.GetAngularOrientationTarget();
		OutParams.bSlerp = Constraint->DefaultInstance.GetAngularDriveMode() == EAngularDriveMode::SLERP;
		OutParams.TwistLimited = Constraint->DefaultInstance.GetAngularTwistMotion();
		OutParams.TwistLimit = Constraint->DefaultInstance.GetAngularTwistLimit();
		OutParams.Swing1Limited = Constraint->DefaultInstance.GetAngularSwing1Motion();
		OutParams.Swing1Limit = Constraint->DefaultInstance.GetAngularSwing1Limit();
		OutParams.Swing2Limited = Constraint->DefaultInstance.GetAngularSwing2Motion();
		OutParams.Swing2Limit = Constraint->DefaultInstance.GetAngularSwing2Limit();
		
		return true;
	}
	return false;
}

bool UPhysicsEditorBPLibrary::GetAllConstraintParams(UPhysicsAsset* PhysicsAsset, TArray<FConstraintParams>& OutParams,
	TArray<FName> ConstraintNames = TArray<FName>())
{
	bool bAll = !ConstraintNames.IsEmpty();
	for(int i=0;i<ConstraintNames.Num();i++)
	{
		auto Constraint = PhysicsAsset->ConstraintSetup[i];
		if(bAll || ConstraintNames.Contains(Constraint->DefaultInstance.JointName))
		{
			FConstraintParams Param;
			if(GetConstraintParams(PhysicsAsset, i, Param))
			{
				OutParams.Add(Param);
			}
		}
	}
	return (!OutParams.IsEmpty());
}


// TArray<FConstraintParams>& UPhysicsEditorBPLibrary::SelectConstraints(UPhysicsAsset* PhysicsAsset, TArray<FConstraintParams>& Options)

bool UPhysicsEditorBPLibrary::ScaleConstraintsByMass(UPhysicsAsset* PhysicsAsset, FPhatConstraintOptions& PhatConstraintOptions,
                                                     TArray<FName> ConstraintNames, float ScaleFactor = .025f)
{
	// for(auto Option : Options)
	// for(int i=0; i<Options.Num(); i++)
	for(auto Name : ConstraintNames)
	{
		if(auto ConstraintOptions = PhatConstraintOptions.ConstraintParamsByName.Find(Name))
		{
			int32 BodyIndex = PhysicsAsset->FindBodyIndex(ConstraintOptions->ConstraintBone1);
			if(BodyIndex  != INDEX_NONE)
			{
				if(auto Body = PhysicsAsset->SkeletalBodySetups[BodyIndex])
				{
					ConstraintOptions->LinearStrength = ScaleFactor * Body->DefaultInstance.GetBodyMass();
					ConstraintOptions->AngularStrength = ScaleFactor * Body->DefaultInstance.GetBodyMass();
				}
			}
		}
	}
	return true;	
}

bool UPhysicsEditorBPLibrary::MirrorConstraintOptions(UPhysicsAsset* PhysicsAsset, TArray<FName> ConstraintNames,
                                                      FPhatConstraintOptions Options, bool bRightToLeft)
{
	FString Suffix = bRightToLeft ? "_r" : "_l"; 
	FString MirrorSuffix = !bRightToLeft ? "_r" : "_l"; 
	FConstraintParams Params = FConstraintParams();
	
	if(PhysicsAsset)
	{
		for(FName Name : ConstraintNames)
		{
			if(Name.ToString().EndsWith(Suffix))
			{
				int32 Index = PhysicsAsset->FindConstraintIndex(Name);
				if(Index != INDEX_NONE)
				{
					if(GetConstraintParams(PhysicsAsset, Index, Params))
					{
						FName MirrorName = FName(Name.ToString().LeftChop(2) + MirrorSuffix);
						int32 MirrorIndex = PhysicsAsset->FindConstraintIndex(MirrorName);
						if(MirrorIndex != INDEX_NONE)
						{
							auto MirrorConstraint = PhysicsAsset->ConstraintSetup[MirrorIndex];
							Params.JointName = MirrorConstraint->DefaultInstance.JointName;
							Params.ConstraintBone1 = MirrorConstraint->DefaultInstance.ConstraintBone1;
							Params.ConstraintBone2 = MirrorConstraint->DefaultInstance.ConstraintBone2;
							ApplyConstraintParams(PhysicsAsset, Params);
							// Options.AllConstraintParams.Add(Params);
						}
						LGE("Mirror bone not found for %s", *Name.ToString())
						
					}
				}
			}
		}
		// return ApplyAllConstraintOptions(PhysicsAsset, Options);
	}
	return false;
}

bool UPhysicsEditorBPLibrary::CopyConstraintOptions(UPhysicsAsset* PhysicsAsset, UPhysicsAsset* SourcePhysicsAsset, FPhatConstraintOptions Options)
{
	FConstraintParams ConstraintOptions;
	for(FName Name : Options.ConstraintsToCopy)
	{
		int32 Index = SourcePhysicsAsset->FindConstraintIndex(Name);
		if(Index != INDEX_NONE)
		{
			if(GetConstraintParams(SourcePhysicsAsset, Index, ConstraintOptions))
				Options.AllConstraintParams.Add(ConstraintOptions);
		}
	}
	return ApplyAllConstraintOptions(PhysicsAsset, Options);
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

inline FName BreastSpokeName(int32 Spoke, int32 Side)
{
	char buffer[40];
	sprintf_s(buffer,"breast_pt_%02d_%c", Spoke, Side == 0 ? 'l' : 'r');
	return FName(buffer);
}

inline FName BreastPtName(int32 Spoke, int32 Point, int32 Side)
{
	char buffer[40];
	sprintf_s(buffer,"breast_pt_%02d_%02d_%c", Spoke, Point, Side == 0 ? 'l' : 'r');
	return FName(buffer);
}
inline FName BreastConstraintName(int32 Spoke, int32 Point, int32 Side, const FString& Kind)
{
	char buffer[40];
	sprintf_s(buffer,"breast_%ls_%02d_%02d_%c", *Kind, Spoke, Point, Side == 0 ? 'l' : 'r');
	return FName(buffer);
}
inline FName BreastBoneName(int32 Number, int32 Side)
{
	char buffer[40];
	sprintf_s(buffer,"breast_%02d_%c", Number, Side == 0 ? 'l' : 'r');
	return FName(buffer);
}


TArray<int32> UPhysicsEditorBPLibrary::AddPointToParentConstraints(USkeletalMeshComponent* SkeletalMeshComponent,
                                                                   FAddPointConstraints Options, TArray<FName> ChildBodies)
{
	TArray<int32> NewConstraintIndexes = TArray<int32>();
	
	TArray<FName> BodyNames;
	UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetPhysicsAsset();
	PhysicsAsset->BodySetupIndexMap.GetKeys(BodyNames);
	for(auto BodyName : ChildBodies)
	{
		FName ParentBone = SkeletalMeshComponent->GetParentBone(BodyName);
		if(ParentBone == NAME_None)
		{
			LGE("No parent bone found for body: %s", *BodyName.ToString())
			continue;
		}
		FConstraintParams Params = Options.PointToParentConstraintParams;
		Params.ConstraintBone1 = BodyName;
		Params.ConstraintBone2 = ParentBone;
		Params.JointName = BodyName;
		LG("      Added parent constraint %s -> %s", *Params.ConstraintBone1.ToString(), *Params.ConstraintBone2.ToString())
		int32 NewIndex = MakeNewConstraint(PhysicsAsset, Params);
		if(NewIndex != INDEX_NONE)
		{
			NewConstraintIndexes.Add(NewIndex);
		}
	}

	return NewConstraintIndexes;
}

TArray<int32> UPhysicsEditorBPLibrary::AddClosestPointConstraints(USkeletalMeshComponent* SkeletalMeshComponent,
                                                            FConstraintParams DefaultParams,
                                                            TArray<FName> TargetBodies,
                                                            TArray<FName> SourceBodies = TArray<FName>(),
                                                            int32 NumClosestPoints = 3,
                                                            bool bExtraBones = false,
                                                            TArray<int32> ClosestBones = TArray<int32>())
{
	TArray<int32> NewConstraintIndexes = TArray<int32>();
	TArray<FName> BodyNames;
	UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetPhysicsAsset();
	PhysicsAsset->BodySetupIndexMap.GetKeys(BodyNames);
	int32 N = NumClosestPoints;
	TArray<FVector> TargetLocations, SourceLocations;
	
	// Generate Location vector arrays
	for(auto BodyName : TargetBodies)
	{
		TargetLocations.Add(SkeletalMeshComponent->GetBoneTransform(BodyName).GetLocation());
	}
	if(SourceBodies.IsEmpty())
	{
		SourceBodies = TargetBodies;
		SourceLocations = TargetLocations;
	} else {
		for(auto BodyName : SourceBodies)
		{
			SourceLocations.Add(SkeletalMeshComponent->GetBoneTransform(BodyName).GetLocation());
		} 
	}
	int32 NumTargets = TargetBodies.Num();
	int32 NumSources = SourceBodies.Num();
	TArray<TArray<float>> DistMatrix = TArray<TArray<float>>();
	TArray<TArray<bool>> AdjMatrix = TArray<TArray<bool>>();

	// Calculate the distance between each point and store in matrix
	for(int i = 0; i < NumSources; i++)
	{
		DistMatrix.Add(TArray<float>());
		AdjMatrix.Add(TArray<bool>());
		for(int j = 0; j < NumTargets; j++)
		{
			DistMatrix[i].Add( FVector::Distance(SourceLocations[i], TargetLocations[j]));
			AdjMatrix[i].Add(false);
		}
	}
	// Create Constraints
	// for each source bone
	for(int i = 0; i < NumSources; i++)
	{
		LG("    Source: %s", *SourceBodies[i].ToString())
		TArray<int32> ClosestIndex;
		ClosestIndex.Init(-1, N);
		TArray<float> Closest;
		Closest.Init(FLT_MAX, N);
		// for  Find the N closest points, store the indices in an array
		for(int j = 0; j < NumTargets; j++)
		{
			if(SourceBodies[i] == TargetBodies[j])
				continue;
			for(int k = 0; k < N; k++)
			{
				if(DistMatrix[i][j] < Closest[k])
				{
					for(int p = N-1; p > k; --p)
					{
						Closest[p] = Closest[p-1];
						ClosestIndex[p] = ClosestIndex[p-1];
					}
					Closest[k] = DistMatrix[i][j];
					ClosestIndex[k] = j;
					break;
				}
			}
		}
		// for each of N closest points, create a constraint
		if(bExtraBones)
			N = ClosestBones.IsValidIndex(i) ? ClosestBones[i] : NumClosestPoints;
		for(int m = 0; m < N; m++)
		{
			if(!ClosestIndex.IsValidIndex(m))
				continue;
			int32 PointIndex = ClosestIndex[m];
			if(PointIndex == INDEX_NONE || AdjMatrix[i][PointIndex])
				continue; 
			if(TargetBodies.IsValidIndex(PointIndex))
			{
				FConstraintParams Params = DefaultParams;
				// Target body is the child. In case of extra bones, we're anchoring the point to a skeletal bone
				Params.ConstraintBone1 = TargetBodies[PointIndex];
				Params.ConstraintBone2 = SourceBodies[i];
				FString Prefix = bExtraBones ? "pt_bone_" : "pt_pt_";
				Params.JointName = FName(Prefix + Params.ConstraintBone1.ToString() + "__" + Params.ConstraintBone2.ToString());
				int32 NewIndex = MakeNewConstraint(PhysicsAsset, Params);
				if(NewIndex != INDEX_NONE)
				{
					LG("      Created Constraint: %s", *Params.JointName.ToString())
					NewConstraintIndexes.Add(NewIndex);
					AdjMatrix[i][PointIndex] = AdjMatrix[i][PointIndex] = true;
				}
			}
		}
	}
	return NewConstraintIndexes;
}

inline FName MirrorBoneName(FName InName)
{
	if(InName.ToString().EndsWith("_l"))
		return FName(InName.ToString().LeftChop(1) + "r");
	if(InName.ToString().EndsWith("_r"))
		return FName(InName.ToString().LeftChop(1) + "l");
	return InName;
}

inline FString MirrorPatternString(FString InPatternString)
{
	if(InPatternString.EndsWith("_l"))
		return InPatternString.LeftChop(1) + "r";
	if(InPatternString.EndsWith("_r"))
		return InPatternString.LeftChop(1) + "l";
	return InPatternString;
}

TArray<int32> UPhysicsEditorBPLibrary::AddPointConstraints(USkeletalMeshComponent* SkeletalMeshComponent,
                                                            FAddPointConstraints Options)
{
	
	TArray<FName> BodyNames, TargetBodies;
	UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetPhysicsAsset();
	PhysicsAsset->BodySetupIndexMap.GetKeys(BodyNames);
	TArray<int32> ConstraintIndexes, TempConstraintIndexes;
	for(int i = 0; i < Options.bAddMirrorConstraints + 1; i++)
	{
		FString PatternString = (i == 0) ? Options.PointPatternString : MirrorPatternString(Options.PointPatternString);
		TargetBodies = FilterNames(BodyNames, PatternString);
		if (Options.bAdjustBodies)
		{
			LG("  Adjusting Bodies for pattern: %s", *Options.PointPatternString)	
			AdjustBodies(SkeletalMeshComponent, Options.AdjustBodiesOptions, TargetBodies);
		}
		if (Options.bAdjustConstraints)
		{
			LG("  Adjusting Constraints for pattern: %s", *Options.PointPatternString)	
			AdjustConstraints(SkeletalMeshComponent, Options.AdjustConstraintsOptions, TargetBodies);
		}
		if (Options.bAddPointToPointConstraints)
		{
			// Point to Point constraints
			LG("  Add Closest Point to Point Constraints for pattern: %s", *Options.PointPatternString)	
			TempConstraintIndexes = AddClosestPointConstraints(SkeletalMeshComponent, Options.PointToPointConstraintParams,
			                                                   TargetBodies, TArray<FName>(), Options.NumClosestPoints);
			ConstraintIndexes.Append(TempConstraintIndexes);
		}
		if (Options.bAddPointToParentConstraints)
		{
			// Point to Parent constraints
			LG("  Add Closest Point to Parent Constraints for pattern: %s", *Options.PointPatternString)	
			TempConstraintIndexes = AddPointToParentConstraints(SkeletalMeshComponent, Options, TargetBodies);
			ConstraintIndexes.Append(TempConstraintIndexes);
		}

		if ( !Options.ExtraBonesToClosestPoints.IsEmpty() )
		{
			LG("  Add closest points to other body Constraints for pattern: %s", *Options.PointPatternString)	
			// Point to Bone Constraints
			TArray<FName> SourceBodies;
			TArray<int32> ClosestPoints;
			Options.ExtraBonesToClosestPoints.GetKeys(SourceBodies);
			int j = 0;
			for(auto BodyName : SourceBodies)
			{
				if(i == 1)
				{
					BodyName = MirrorBoneName(BodyName);
					SourceBodies[j++] = BodyName;
				}
				ClosestPoints.Add(Options.ExtraBonesToClosestPoints.FindOrAdd(BodyName, 3));
			}
			TempConstraintIndexes = AddClosestPointConstraints(SkeletalMeshComponent, Options.ExtraBonesConstraintParams,
			                                                   TargetBodies, SourceBodies,
			                                                   Options.NumClosestPoints, true, ClosestPoints);
			ConstraintIndexes.Append(TempConstraintIndexes);
		}
	}
	LG("Added or modified %d constraints.", ConstraintIndexes.Num())
	return ConstraintIndexes;
}

TArray<int32> UPhysicsEditorBPLibrary::AddPhatConstraints(USkeletalMeshComponent* SkeletalMeshComponent,
	FPhatConstraintOptions Options)
{
	UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetPhysicsAsset();
	TArray<int32> NewConstraintIndexes;
	if(Options.bAddGluteCores)
		NewConstraintIndexes += AddPointConstraints(SkeletalMeshComponent, Options.GluteCores);
	if(Options.bAddGluteSpokes)
		NewConstraintIndexes += AddPointConstraints(SkeletalMeshComponent, Options.GluteSpokes);
	if(Options.bAddGlutePoints)
		NewConstraintIndexes += AddPointConstraints(SkeletalMeshComponent, Options.GlutePoints);
	if(Options.bAddBreastCores)
		NewConstraintIndexes += AddPointConstraints(SkeletalMeshComponent, Options.BreastCores);
	if(Options.bAddBreastSpokes)
		NewConstraintIndexes += AddPointConstraints(SkeletalMeshComponent, Options.BreastSpokes);
	if(Options.bAddBreastPoints)
		NewConstraintIndexes += AddPointConstraints(SkeletalMeshComponent, Options.BreastPoints);

	PhysicsAsset->MarkPackageDirty();
	PhysicsAsset->RefreshPhysicsAssetChange();
	return NewConstraintIndexes;
}

int32 UPhysicsEditorBPLibrary::MakeNewConstraint(UPhysicsAsset* PhysicsAsset,
                                                 FConstraintParams Params)
{
	FName ConstraintBone1 = Params.ConstraintBone1;
	FName ConstraintBone2 = Params.ConstraintBone2;
	FName JointName = Params.JointName;
	int32 ChildIndex = PhysicsAsset->FindBodyIndex(ConstraintBone1);
	int32 ParentIndex = PhysicsAsset->FindBodyIndex(ConstraintBone2);
	if( ChildIndex == INDEX_NONE || ParentIndex == INDEX_NONE) {
		LGE("Bone %s or Bone %s not found in physics asset", *ConstraintBone1.ToString(), *ConstraintBone2.ToString())
		return INDEX_NONE;
	}
	int32 ConstraintIndex = PhysicsAsset->FindConstraintIndex(JointName);
	if (ConstraintIndex != INDEX_NONE && !Params.bOverwriteExisting) {
		LGV("Constraint %s already exists, not overwriting", *JointName.ToString())
		return INDEX_NONE;
	}
	
	if(JointName == FName())
		JointName = FName(ConstraintBone1.ToString() + "__" + ConstraintBone2.ToString());
	
	ConstraintIndex = FPhysicsAssetUtils::CreateNewConstraint(PhysicsAsset, JointName);
	if (ConstraintIndex != INDEX_NONE)
	{
		UPhysicsConstraintTemplate* ConstraintSetup = PhysicsAsset->ConstraintSetup[ConstraintIndex];
		ConstraintSetup->DefaultInstance.ConstraintBone1 = ConstraintBone1;
		ConstraintSetup->DefaultInstance.ConstraintBone2 = ConstraintBone2;
		ApplyConstraintParams(PhysicsAsset, Params);
		ConstraintSetup->DefaultInstance.SnapTransformsToDefault(EConstraintTransformComponentFlags::All, PhysicsAsset);
		PhysicsAsset->DisableCollision(ChildIndex, ParentIndex);
		
		return ConstraintIndex;
	}
	LGE("Constraint %s not able to be created", *JointName.ToString())
	return INDEX_NONE;
}


bool UPhysicsEditorBPLibrary::RegexMatch(FRegexPattern Pattern, FString Str)
{
	FRegexMatcher Matcher = FRegexMatcher(Pattern, Str);
	if (Matcher.FindNext())
		return true;
	return false;
}

TArray<FName> UPhysicsEditorBPLibrary::FilterNames(TArray<FName> Names, FString PatternString)
{
	TArray<FName> OutNames;
	FRegexPattern Pattern = FRegexPattern(PatternString);
	for(FName Name : Names)
	{
		if(RegexMatch(Pattern, Name.ToString()))
			OutNames.Add(Name);
	}
	return OutNames;
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
