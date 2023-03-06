// Copyright 2022 BlueRose, Inc. All Rights Reserved.

#include "Outline/OutlineStaticMeshComponent.h"
#include "Outline/OutlineStaticMeshSceneProxy.h"
#include "Engine/StaticMesh.h"
#define LOCTEXT_NAMESPACE "OutlineStaticMeshComponent"

UOutlineStaticMeshComponent::UOutlineStaticMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),CullingMode(EMultiDrawCullingMode::FrontfaceCulling),bCastShadow(false)
{

}

void UOutlineStaticMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
{
	if (GetStaticMesh() && GetStaticMesh()->GetRenderData())
	{
		FStaticMeshRenderData* RenderData = GetStaticMesh()->GetRenderData();

		TSet<int32> UniqueIndex;
		for (int32 LODIndex = 0, Num = RenderData->LODResources.Num(); LODIndex < Num; LODIndex++)
		{
			FStaticMeshLODResources& LODResources = RenderData->LODResources[LODIndex];
			for (int32 SectionIndex = 0; SectionIndex < LODResources.Sections.Num(); SectionIndex++)
			{
				// Get the material for each element at the current lod index
				UniqueIndex.Add(LODResources.Sections[SectionIndex].MaterialIndex);
			}
		}
		
		if (UniqueIndex.Num() > 0)
		{
			//We need to output the material in the correct order (follow the material index)
			//So we sort the map with the material index
			UniqueIndex.Sort([](int32 A, int32 B) {
				return A < B; // sort keys in order
			});

			OutMaterials.Reserve(UniqueIndex.Num());
			for (int32 MaterialIndex : UniqueIndex)
			{
				OutMaterials.Add(GetMaterial(MaterialIndex));
			}
			
			for(UMaterialInterface* Material : OutlinePassMaterialSet)
			{
				OutMaterials.Add(Material);
			}

			if (OverlayMaterial != nullptr)
			{
				OutMaterials.Add(OverlayMaterial);
			}
		}
	}
}

FMaterialRelevance UOutlineStaticMeshComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result=Super::GetMaterialRelevance(InFeatureLevel);
	for(UMaterialInterface* Material : OutlinePassMaterialSet)
	{
		UMaterialInterface* MaterialInterface = Material ? Material :  UMaterial::GetDefaultMaterial(MD_Surface);
		Result |= MaterialInterface->GetRelevance_Concurrent(InFeatureLevel);
	}
	
	return Result;
}

FPrimitiveSceneProxy* UOutlineStaticMeshComponent::CreateSceneProxy()
{
	if (GetStaticMesh() == nullptr || GetStaticMesh()->GetRenderData() == nullptr)
	{
		return nullptr;
	}

	const TIndirectArray<FStaticMeshLODResources>& LODResources = GetStaticMesh()->GetRenderData()->LODResources;
	if (LODResources.Num() == 0 || LODResources[FMath::Clamp<int32>(GetStaticMesh()->GetMinLOD().Default, 0, LODResources.Num() - 1)].VertexBuffers.StaticMeshVertexBuffer.GetNumVertices() == 0)
	{
		return nullptr;
	}

	FPrimitiveSceneProxy* Proxy = ::new FOutlineStaticMeshSceneProxy(this, false,CullingMode,bCastShadow);
#if STATICMESH_ENABLE_DEBUG_RENDERING
	SendRenderDebugPhysics(Proxy);
#endif

	return Proxy;
}

bool UOutlineStaticMeshComponent::ChangeMaterialByIndex(UMaterialInterface* InMaterialInstance, int32 Index)
{
	if(OutlinePassMaterialSet.Num() < Index+1 || !IsValid(InMaterialInstance))
		return false;

	OutlinePassMaterialSet[Index]=InMaterialInstance;
	return true;
}

#undef LOCTEXT_NAMESPACE
