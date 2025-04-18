// Fill out your copyright notice in the Description page of Project Settings.


#include "PhysicsAssetDefaults.h"

UPhysicsAssetDefaults::UPhysicsAssetDefaults()
{
	
}

bool UPhysicsAssetDefaults::InitPointToCenter()
{
	
	auto Instance = FConstraintInstance();
	auto Profile = Instance.ProfileInstance;
	Profile.LinearLimit.Limit = .25f;
	Profile.LinearLimit.XMotion = Profile.LinearLimit.YMotion = Profile.LinearLimit.YMotion = LCM_Limited;
	auto LinearDrive = FLinearDriveConstraint();
	LinearDrive.XDrive.Stiffness = LinearDrive.XDrive.Stiffness = LinearDrive.XDrive.Stiffness = 300.f;
	LinearDrive.XDrive.Damping = LinearDrive.XDrive.Damping = LinearDrive.XDrive.Damping = 100.f;
	Profile.LinearDrive = LinearDrive;
	
	Profile.ConeLimit.Swing1Motion = Profile.ConeLimit.Swing1Motion = Profile.TwistLimit.TwistMotion = ACM_Free;

	Instance.ProfileInstance = Profile;
	PointToCenterSetup = NewObject<UPhysicsConstraintTemplate>();
	PointToCenterSetup->DefaultInstance = Instance;
	return true;
}
	// Profile.AngularDrive = FAngularDriveConstraint();
	// Profile.AngularDrive.
