#pragma once

#include "PhysicsAssetUtils.h"
#include "Utils.h"

#include "FConstraintParams.generated.h"

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EConstraintOverwrite: uint8
{
	None		   = 0,
	RefFrames	   = 1 << 0,
	LinearDrive	   = 1 << 1,
	LinearLimits   = 1 << 2,
	AngularDrive   = 1 << 3,
	AngularLimits  = 1 << 4,
	Linear = LinearDrive | LinearLimits,
	Angular = AngularDrive | AngularLimits,
	Drives = LinearDrive | AngularDrive,
	Limits = LinearLimits | AngularLimits,
	DrivesLimits = Drives | Limits,
	All = RefFrames | DrivesLimits
};
ENUM_CLASS_FLAGS(EConstraintOverwrite);

// /** Parameters for PhysicsAsset creation */
// USTRUCT()
// struct FPhysAssetCreateParamsRow : public FPhysAssetCreateParams, public FTableRowBase
// {
// 	GENERATED_BODY()
// public:
// };

/** Simple params for initializing physics constraints */
USTRUCT(BlueprintType)
struct FConstraintParams : public FTableRowBase
{
	GENERATED_BODY()
public:
	FConstraintParams(){}
	
	FConstraintParams(FName InJointName, FName InConstraintBone1, FName InConstraintBone2)
			: JointName(InJointName), ConstraintBone1(InConstraintBone1), ConstraintBone2(InConstraintBone2) { }	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = EConstraintOverwrite))	
	EConstraintOverwrite ConstraintOverwrite = EConstraintOverwrite::DrivesLimits;
	
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Overwrite Existing Constraint", MakeStructureDefaultValue="None"))
	bool bOverwriteExisting = true;
	
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="JointName", MakeStructureDefaultValue="None"))
	FName JointName = FName();

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="ContraintBone1", MakeStructureDefaultValue="None"))
	FName ConstraintBone1 = FName();

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="ConstraintBone2", MakeStructureDefaultValue="None"))
	FName ConstraintBone2 = FName();

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="RefFrameNoScale1", MakeStructureDefaultValue="0.000000,0.000000,0.000000|0.000000,0.000000,0.000000|1.000000,1.000000,1.000000"))
	FTransform RefFrameNoScale1 = FTransform();

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="RefFrameNoScale2", MakeStructureDefaultValue="0.000000,0.000000,0.000000|0.000000,0.000000,0.000000|1.000000,1.000000,1.000000"))
	FTransform RefFrameNoScale2 = FTransform();

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="AngularRotationOffset", MakeStructureDefaultValue="0.000000,0.000000,0.000000"))
	FRotator AngularRotationOffset = FRotator();

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="LinearLimitedX", MakeStructureDefaultValue="False"))
	TEnumAsByte<ELinearConstraintMotion> LinearLimitedX = LCM_Locked;
	
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="LinearLimitedY", MakeStructureDefaultValue="False"))
	TEnumAsByte<ELinearConstraintMotion> LinearLimitedY = LCM_Locked;
	
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="LinearLimitedZ", MakeStructureDefaultValue="False"))
	TEnumAsByte<ELinearConstraintMotion> LinearLimitedZ = LCM_Locked;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="LinearLimit", MakeStructureDefaultValue="0.000000"))
	float LinearLimit = .0f;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="LinearTarget", MakeStructureDefaultValue="0.000000,0.000000,0.000000|0.000000,0.000000,0.000000|1.000000,1.000000,1.000000"))
	FVector LinearTarget = FVector();

	/** If > 0, LinearStrength on the constraint is set to this value * the mass of the body.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="LinearStrength", MakeStructureDefaultValue="0.000000"))
	float LinearStrengthMassMultiplier = 0.f;

	/** Linear strength for the constraint when LinearStrengthMassMultiplier <= 0 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="LinearStrength", MakeStructureDefaultValue="0.000000"))
	float LinearStrength = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="LinearDampingRatio", MakeStructureDefaultValue="None"))
	float LinearDampingRatio = .025f;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="TwistLimited", MakeStructureDefaultValue="False"))
	TEnumAsByte<EAngularConstraintMotion> TwistLimited = ACM_Locked;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="TwistLimit", MakeStructureDefaultValue="0.000000"))
	float TwistLimit = 45.f;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Swing1Limited", MakeStructureDefaultValue="False"))
	TEnumAsByte<EAngularConstraintMotion> Swing1Limited = ACM_Locked;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Swing1Limit", MakeStructureDefaultValue="0.000000"))
	float Swing1Limit = 45.f;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Swing2Limited", MakeStructureDefaultValue="False"))
	TEnumAsByte<EAngularConstraintMotion> Swing2Limited = ACM_Locked;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Swing2Limit", MakeStructureDefaultValue="0.000000"))
	float Swing2Limit = 45.f;

	/** If > 0, AngularStrength on the constraint is set to this value * the mass of the body.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="AngularStrength", MakeStructureDefaultValue="0.000000"))
	float AngularStrengthMassMultiplier = 0.f;

	/** Angular strength for the constraint when AngularStrengthMassMultiplier <= 0 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="AngularStrength", MakeStructureDefaultValue="0.000000"))
	float AngularStrength = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="AngularDampingRatio", MakeStructureDefaultValue="None"))
	float AngularDampingRatio = .025f;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="AngularTarget", MakeStructureDefaultValue="0.000000,0.000000,0.000000"))
	FRotator AngularTarget = FRotator();
	
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Use SLERP Drive", MakeStructureDefaultValue="0.000000,0.000000,0.000000"))
	bool bSlerp = false;
};

USTRUCT(BlueprintType)
struct FAdjustBodiesOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MatchBodyRegex;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PositionRatio = .3f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RadiusRatio = .3f;
};

USTRUCT(BlueprintType)
struct FAdjustConstraintsOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MatchParentBodyRegex;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MatchChildBodyRegex;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PositionRatio = .3f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float OrientationAlpha = 0.f;
};

USTRUCT(BlueprintType)
struct FAddPointConstraints
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PointPatternString = TEXT("glute_(\\d{2})_(\\d{2})_(\\d{2})_pt_l");
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumClosestPoints = 3;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxClosestPoints = 5;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddMirrorConstraints = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddPointToParentConstraints = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FConstraintParams PointToParentConstraintParams;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddPointToPointConstraints = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FConstraintParams PointToPointConstraintParams;

	/** Map of bone names -> num of points to constrain **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, int32> ExtraBonesToClosestPoints;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FConstraintParams ExtraBonesConstraintParams;


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAdjustBodies;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FAdjustBodiesOptions AdjustBodiesOptions;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAdjustConstraints;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FAdjustConstraintsOptions AdjustConstraintsOptions;
};


USTRUCT(BlueprintType)
struct FPhatConstraintOptions
{
	GENERATED_BODY()
public:

	FPhatConstraintOptions()
	{
		FConstraintParams Params = FConstraintParams();
		Params.LinearLimitedX = Params.LinearLimitedY = Params.LinearLimitedZ = LCM_Limited;
		Params.LinearLimit = 1.f;
		Params.LinearStrengthMassMultiplier = 10000;
		Params.AngularStrengthMassMultiplier = 10000;
		FAddPointConstraints AddPoints = FAddPointConstraints();
		AddPoints.PointToParentConstraintParams = AddPoints.PointToPointConstraintParams =
			AddPoints.ExtraBonesConstraintParams = Params;
		AddPoints.PointToParentConstraintParams.TwistLimited = AddPoints.PointToParentConstraintParams.Swing1Limited =
			AddPoints.PointToParentConstraintParams.Swing2Limited = ACM_Limited;
		GluteCores = GluteSpokes = GlutePoints = BreastCores = BreastSpokes = BreastPoints = AddPoints;
		
		GluteCores.PointPatternString = TEXT("glute_(\\d{2})_l");
		GluteSpokes.PointPatternString = TEXT("glute_(\\d{2})_(\\d{2})_l");
		GlutePoints.PointPatternString = TEXT("glute_(\\d{2})_(\\d{2})_(\\d{2})_pt_l");
		GlutePoints.ExtraBonesToClosestPoints.Add(FName("thigh_l"), 5);
		GlutePoints.ExtraBonesToClosestPoints.Add(FName("pelvis"), 5);
		
		BreastCores.PointPatternString = TEXT("breast_(\\d{2})_l");
		BreastSpokes.PointPatternString = TEXT("breast_pt_(\\d{2})_l");
		BreastPoints.PointPatternString = TEXT("breast_pt_(\\d{2})_(\\d{2})_l");
	}
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="ConstraintsToCopy", MakeStructureDefaultValue="None"))
	TArray<FName> ConstraintsToCopy;
	
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="All ConstraintParams", MakeStructureDefaultValue="None"))
	TArray<FConstraintParams> AllConstraintParams;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FConstraintParams> ConstraintParamsByName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = EConstraintOverwrite))	
	EConstraintOverwrite ConstraintOverwrite = EConstraintOverwrite::DrivesLimits;
	
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Overwrite Existing Constraint", MakeStructureDefaultValue="None"))
	bool bOverwriteExisting = true;
	
	
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddGluteCores;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FAddPointConstraints GluteCores;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddGluteSpokes;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FAddPointConstraints GluteSpokes;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddGlutePoints = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FAddPointConstraints GlutePoints;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddBreastCores;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FAddPointConstraints BreastCores;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddBreastSpokes;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FAddPointConstraints BreastSpokes;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAddBreastPoints;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FAddPointConstraints BreastPoints;
	TMap<FName, FAddPointConstraints> PointConstraintParamSetup;
	
	// UPROPERTY(BlueprintReadWrite, EditAnywhere)
	// FAddPointConstraints GluteConstraintsOptions;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(DisplayName="Position to Velocity ratio", MakeStructureDefaultValue="None"))
	float PositionVelocityRatio = 20.f;
};

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


inline FPhysAssetCreateParams FPhysAssetCreateParamsRow::GetCreateParams() const
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
