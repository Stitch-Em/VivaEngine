// Copyright 2022 BlueRose, Inc. All Rights Reserved.

#pragma once

#include "MultiDrawSkeletalMeshComponent.h"
#include "Engine/Public/SkeletalMeshTypes.h"

class FMultiDrawSkeletalMeshSceneProxy : public FSkeletalMeshSceneProxy
{
public:
	FMultiDrawSkeletalMeshSceneProxy(const UMultiDrawSkeletalMeshComponent* Component, FSkeletalMeshRenderData* InSkelMeshRenderData);

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	
	void GetDynamicElementsSection(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
	                               const FSkeletalMeshLODRenderData& LODData, const int32 LODIndex, const int32 SectionIndex, bool bSectionSelected,
	                               const FSectionElementInfo& SectionElementInfo, bool bInSelectable, FMeshElementCollector& Collector, bool bCastShadow=true, EMultiDrawCullingMode
	                               CullingMode=EMultiDrawCullingMode::BackfaceCulling) const;
private:

	void CreateBaseMeshBatch(const FSceneView* View, const FSkeletalMeshLODRenderData& LODData, const int32 LODIndex, const int32 SectionIndex, const FSectionElementInfo& SectionElementInfo, FMeshBatch& Mesh,ESkinVertexFactoryMode VFMode = ESkinVertexFactoryMode::Default) const;
	void UpdateLooseParametersUniformBuffer(const FSceneView* View, const int32 SectionIndex, const FMeshBatch& Mesh, const FGPUSkinBatchElementUserData* BatchUserData) const;
	const UMultiDrawSkeletalMeshComponent* ComponentPtr;
	
	UMaterialInterface* OverlayMaterial;
	float OverlayMaterialMaxDrawDistance;
};