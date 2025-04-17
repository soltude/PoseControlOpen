#pragma once

#include "CoreMinimal.h"
#include "MeshUtilitiesCommon.h"

static FMatrix ComputeCovarianceMatrix(const FBoneVertInfo& VertInfo)
{
	if (VertInfo.Positions.Num() == 0)
	{
		return FMatrix::Identity;
	}

	const TArray<FVector3f> & Positions = VertInfo.Positions;

	//get average
	const float N = Positions.Num();
	FVector U = FVector::ZeroVector;
	for (int32 i = 0; i < N; ++i)
	{
		U += (FVector)Positions[i];
	}

	U = U / N;

	//compute error terms
	TArray<FVector> Errors;
	Errors.AddUninitialized(N);

	for (int32 i = 0; i < N; ++i)
	{
		Errors[i] = (FVector)Positions[i] - U;
	}

	FMatrix Covariance = FMatrix::Identity;
	for (int32 j = 0; j < 3; ++j)
	{
		FVector Axis = FVector::ZeroVector;
		FVector::FReal* Cj = &Axis.X;
		for (int32 k = 0; k < 3; ++k)
		{
			float Cjk = 0.f;
			for (int32 i = 0; i < N; ++i)
			{
				const FVector::FReal* error = &Errors[i].X;
				Cj[k] += error[j] * error[k];
			}
			Cj[k] /= N;
		}

		Covariance.SetAxis(j, Axis);
	}

	return Covariance;
}

static FVector ComputeEigenVector(const FMatrix& A)
{
	//using the power method: this is ok because we only need the dominate eigenvector and speed is not critical: http://en.wikipedia.org/wiki/Power_iteration
	FVector Bk = FVector(0, 0, 1);
	for (int32 i = 0; i < 32; ++i)
	{
		float Length = Bk.Size();
		if (Length > 0.f)
		{
			Bk = A.TransformVector(Bk) / Length;
		}
	}

	return Bk.GetSafeNormal();
}
