// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#include "MultiDraw/MultiDrawStaticMeshComponent.h"
#include "MultiDraw/MultiDrawStaticMeshSceneProxy.h"
#include "Engine/StaticMesh.h"
#define LOCTEXT_NAMESPACE "MultiDrawStaticMeshComponent"

UMultiDrawStaticMeshComponent::UMultiDrawStaticMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UMultiDrawStaticMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
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
			
			for(const auto& Material : PerPassDrawSettingses)
			{
				if(Material.bDraw && Material.MaterialInterface && Material.ElementIndex <= RenderData->LODResources[0].Sections.Num())
				{
					OutMaterials.Add(Material.MaterialInterface);
				}
			}

			if (OverlayMaterial != nullptr)
			{
				OutMaterials.Add(OverlayMaterial);
			}
		}
	}
}

FMaterialRelevance UMultiDrawStaticMeshComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result=Super::GetMaterialRelevance(InFeatureLevel);
	for(const FMultiDrawSettings& Material : PerPassDrawSettingses)
	{
		UMaterialInterface* MaterialInterface = Material.MaterialInterface ? Material.MaterialInterface :  UMaterial::GetDefaultMaterial(MD_Surface);
		Result |= MaterialInterface->GetRelevance_Concurrent(InFeatureLevel);
	}
	
	return Result;
}

FPrimitiveSceneProxy* UMultiDrawStaticMeshComponent::CreateSceneProxy()
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

	FPrimitiveSceneProxy* Proxy = ::new FMultiDrawStaticMeshSceneProxy(this, false);
#if STATICMESH_ENABLE_DEBUG_RENDERING
	SendRenderDebugPhysics(Proxy);
#endif

	return Proxy;
}

bool UMultiDrawStaticMeshComponent::ChangeMaterialByIndex(UMaterialInterface* InMaterialInstance, int32 Index)
{
	if(PerPassDrawSettingses.Num() < Index+1 || !IsValid(InMaterialInstance))
		return false;

	PerPassDrawSettingses[Index].MaterialInterface=InMaterialInstance;
	return true;
}

bool UMultiDrawStaticMeshComponent::ChangeMultiDrawSettingsByIndex(FMultiDrawSettings InSettings, int32 Index)
{
	if(PerPassDrawSettingses.Num() < Index+1 || !IsValid(InSettings.MaterialInterface))
		return false;

	PerPassDrawSettingses[Index]=InSettings;
	return true;
}

#undef LOCTEXT_NAMESPACE
