// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DiamondSquare.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UCurveFloat;

UCLASS()
class VIVAENGINE_API ADiamondSquare : public AActor
{
	GENERATED_BODY()

public:
	ADiamondSquare();

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
		int XSize = 0;
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
		int YSize = 0;
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
		float ZMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
		float NoiseScale = 1.0f;

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.000001))
		float Scale = 100.0f;
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.000001))
		float UVScale = 0.0f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
		UMaterialInterface* Material;
	UPROPERTY(EditAnywhere)
		UCurveFloat* HeightCurve;

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void Regenerate();
	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void ChangeHeight(float X, float Y, float HeightMultiplier,float DistanceMulti, float Falloff);


public:
	virtual void Tick(float DeltaTime) override;

private:
	UProceduralMeshComponent* ProceduralMesh;
	TArray<FVector> Vertices;
	TArray<int> Triangles;
	TArray<FVector2D> UV0;

	void CreateVertices();
	void CreateTriangles();
};