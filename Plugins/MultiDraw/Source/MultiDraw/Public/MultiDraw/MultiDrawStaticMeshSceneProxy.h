// Copyright 2022 BlueRose, Inc. All Rights Reserved.

#pragma once

#include "MultiDrawStaticMeshComponent.h"
#include "StaticMeshResources.h"

class FMultiDrawStaticMeshSceneProxy : public FStaticMeshSceneProxy
{
public:
	FMultiDrawStaticMeshSceneProxy(UMultiDrawStaticMeshComponent* Component, bool bForceLODsShareStaticLighting);

	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;

private:

	const UMultiDrawStaticMeshComponent* ComponentPtr;
	
	UMaterialInterface* OverlayMaterial;
	float OverlayMaterialMaxDrawDistance;
};