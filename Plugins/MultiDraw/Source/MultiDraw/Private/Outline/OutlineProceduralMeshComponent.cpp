// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#include "Outline/OutlineProceduralMeshComponent.h"
#include "Outline/OutlineProceduralMeshSceneProxy.h"

UOutlineProceduralMeshComponent::UOutlineProceduralMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),CullingMode(EMultiDrawCullingMode::FrontfaceCulling),bCastShadow(false)
{

}

void UOutlineProceduralMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials,bool bGetDebugMaterials) const
{
	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);

	for(UMaterialInterface* Material : OutlinePassMaterialSet)
	{
		OutMaterials.Add(Material);
	}
}

FMaterialRelevance UOutlineProceduralMeshComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	// Combine the material relevance for all materials.
	FMaterialRelevance Result;
	for(int32 ElementIndex = 0;ElementIndex < GetNumMaterials();ElementIndex++)
	{
		UMaterialInterface const* MaterialInterface = GetMaterial(ElementIndex);
		if(!MaterialInterface)
		{
			MaterialInterface = UMaterial::GetDefaultMaterial(MD_Surface);
		}
		Result |= MaterialInterface->GetRelevance_Concurrent(InFeatureLevel);
	}

	if (OverlayMaterial != nullptr)
	{
		Result |= OverlayMaterial->GetRelevance_Concurrent(InFeatureLevel);
	}

	for(UMaterialInterface* Material : OutlinePassMaterialSet)
	{
		UMaterialInterface* MaterialInterface = Material ? Material :  UMaterial::GetDefaultMaterial(MD_Surface);
		Result |= MaterialInterface->GetRelevance_Concurrent(InFeatureLevel);
	}
	
	return Result;
}

FPrimitiveSceneProxy* UOutlineProceduralMeshComponent::CreateSceneProxy()
{
	return new FOutlineProceduralMeshSceneProxy(this,CullingMode,bCastShadow);
}

bool UOutlineProceduralMeshComponent::ChangeMaterialByIndex(UMaterialInterface* InMaterialInstance, int32 Index)
{
	if(OutlinePassMaterialSet.Num() < Index+1 || !IsValid(InMaterialInstance))
		return false;

	OutlinePassMaterialSet[Index]=InMaterialInstance;
	return true;
}
