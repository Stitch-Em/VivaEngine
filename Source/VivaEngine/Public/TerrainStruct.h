// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
struct VIVAENGINE_API TerrainStruct
{
public:
	
	TArray<FVector> normals;
	TArray<FVector> tangents;
	TArray<FVector> Vertices;
	TArray<FVector2D> UV0;
	TArray<int32> Triangles;
	UMaterialInterface* material;
	FVector2D XY;

	TerrainStruct();
	~TerrainStruct();
};
