// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "UObject/Object.h"
#include "PhysicsAssetDefaults.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class POSECONTROLEDITOR_API UPhysicsAssetDefaults : public UObject
{
	GENERATED_BODY()

public:
	UPhysicsAssetDefaults();
	bool InitPointToCenter();
	TObjectPtr<UPhysicsConstraintTemplate> PointToCenterSetup;
};
