// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#pragma once
#include "StaticMeshResources.h"
#include "MultiDrawCommon.h"

class FOutlineStaticMeshSceneProxy : public FStaticMeshSceneProxy
{
public:
	FOutlineStaticMeshSceneProxy(UStaticMeshComponent* Component, bool bForceLODsShareStaticLighting,EMultiDrawCullingMode CullingMode,bool bCastShadow);

	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;
	
private:
	UPROPERTY()
	const UStaticMeshComponent* ComponentPtr;

	EMultiDrawCullingMode CullingMode;
	
	bool bCastShadow;

	UMaterialInterface* OverlayMaterial;
	float OverlayMaterialMaxDrawDistance;
};