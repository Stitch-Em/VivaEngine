// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#pragma once
#include "Engine/Public/SkeletalMeshTypes.h"
#include "MultiDrawCommon.h"

class FOutlineSkeletalMeshSceneProxy : public FSkeletalMeshSceneProxy
{
public:
	FOutlineSkeletalMeshSceneProxy(const USkinnedMeshComponent* Component, FSkeletalMeshRenderData* InSkelMeshRenderData,EMultiDrawCullingMode CullingMode,bool bCastShadow);

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	void GetDynamicElementsSection(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, 
		const FSkeletalMeshLODRenderData& LODData, const int32 LODIndex, const int32 SectionIndex, bool bSectionSelected,
		const FSectionElementInfo& SectionElementInfo, bool bInSelectable, FMeshElementCollector& Collector ) const;
	
private:
	void CreateBaseMeshBatch(const FSceneView* View, const FSkeletalMeshLODRenderData& LODData, const int32 LODIndex, const int32 SectionIndex, const FSectionElementInfo& SectionElementInfo, FMeshBatch& Mesh,ESkinVertexFactoryMode VFMode = ESkinVertexFactoryMode::Default) const;
	const USkinnedMeshComponent* ComponentPtr;

	EMultiDrawCullingMode CullingMode;

	bool bCastShadow;
	
	UMaterialInterface* OverlayMaterial;
	float OverlayMaterialMaxDrawDistance;
};