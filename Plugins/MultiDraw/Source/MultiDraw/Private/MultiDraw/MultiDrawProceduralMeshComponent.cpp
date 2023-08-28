// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#include "MultiDraw/MultiDrawProceduralMeshComponent.h"
#include "MultiDraw/MultiDrawProceduralMeshSceneProxy.h"
#include "MaterialDomain.h"

UMultiDrawProceduralMeshComponent::UMultiDrawProceduralMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UMultiDrawProceduralMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials,bool bGetDebugMaterials) const
{
	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);

	for (const auto& MultiDrawMaterial : PerPassDrawSettingses) 
	{
		if (MultiDrawMaterial.bDraw && MultiDrawMaterial.MaterialInterface)
		{
			if(MultiDrawMaterial.MaterialInterface->CheckMaterialUsage_Concurrent(MATUSAGE_SkeletalMesh))
				OutMaterials.Add(MultiDrawMaterial.MaterialInterface);
			else
				OutMaterials.Add(UMaterial::GetDefaultMaterial(MD_Surface));
		}
	}
}

FMaterialRelevance UMultiDrawProceduralMeshComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
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

	for(const FMultiDrawSettings& Material : PerPassDrawSettingses)
	{
		UMaterialInterface* MaterialInterface = Material.MaterialInterface ? Material.MaterialInterface :  UMaterial::GetDefaultMaterial(MD_Surface);
		Result |= MaterialInterface->GetRelevance_Concurrent(InFeatureLevel);
	}
	
	return Result;
}

FPrimitiveSceneProxy* UMultiDrawProceduralMeshComponent::CreateSceneProxy()
{
	return new FMultiDrawProceduralMeshSceneProxy(this);
}

bool UMultiDrawProceduralMeshComponent::ChangeMaterialByIndex(UMaterialInterface* InMaterialInstance, int32 Index)
{
	if(PerPassDrawSettingses.Num() < Index+1 || !IsValid(InMaterialInstance))
		return false;

	UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(InMaterialInstance);
	if (!((DynamicMaterial != nullptr && DynamicMaterial->Parent == PerPassDrawSettingses[Index].MaterialInterface) || PerPassDrawSettingses[Index].MaterialInterface == nullptr))
	{
		// Mark cached material parameter names dirty
		MarkCachedMaterialParameterNameIndicesDirty();
	}
	
	PerPassDrawSettingses[Index].MaterialInterface=InMaterialInstance;
	MarkRenderStateDirty();			
	if (InMaterialInstance)
	{
		InMaterialInstance->AddToCluster(this, true);
	}

	FBodyInstance* BodyInst = GetBodyInstance();
	if (BodyInst && BodyInst->IsValidBodyInstance())
	{
		BodyInst->UpdatePhysicalMaterials();
	}
	
	return true;
}

bool UMultiDrawProceduralMeshComponent::ChangeMultiDrawSettingsByIndex(FMultiDrawSettings InSettings, int32 Index)
{
	if(PerPassDrawSettingses.Num() < Index+1 || !IsValid(InSettings.MaterialInterface))
		return false;
	
	PerPassDrawSettingses[Index].bDraw=InSettings.bDraw;
	PerPassDrawSettingses[Index].bCastShadow=InSettings.bCastShadow;
	PerPassDrawSettingses[Index].CullingMode=InSettings.CullingMode;
	PerPassDrawSettingses[Index].ElementIndex=InSettings.ElementIndex;
	ChangeMaterialByIndex(InSettings.MaterialInterface,Index);
	
	return true;
}
