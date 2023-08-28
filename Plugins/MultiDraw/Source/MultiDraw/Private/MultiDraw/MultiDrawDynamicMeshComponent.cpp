// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#include "MultiDraw/MultiDrawDynamicMeshComponent.h"
#include "MultiDraw/MultiDrawDynamicMeshSceneProxy.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MaterialDomain.h"

UMultiDrawDynamicMeshComponent::UMultiDrawDynamicMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UMultiDrawDynamicMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
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

FMaterialRelevance UMultiDrawDynamicMeshComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result=Super::GetMaterialRelevance(InFeatureLevel);
	if (OverrideRenderMaterial)
	{
		Result |= OverrideRenderMaterial->GetRelevance_Concurrent(InFeatureLevel);
	}
	if (SecondaryRenderMaterial)
	{
		Result |= SecondaryRenderMaterial->GetRelevance_Concurrent(InFeatureLevel);
	}
	for(const FMultiDrawSettings& Material : PerPassDrawSettingses)
	{
		UMaterialInterface* MaterialInterface = Material.MaterialInterface ? Material.MaterialInterface :  UMaterial::GetDefaultMaterial(MD_Surface);
		Result |= MaterialInterface->GetRelevance_Concurrent(InFeatureLevel);
	}
	
	return Result;
}

FPrimitiveSceneProxy* UMultiDrawDynamicMeshComponent::CreateSceneProxy()
{
	// if this is not always the case, we have made incorrect assumptions
	ensure(GetCurrentSceneProxy() == nullptr);

	FMultiDrawDynamicMeshSceneProxy* NewProxy = nullptr;
	if (GetMesh()->TriangleCount() > 0)
	{
		NewProxy = new FMultiDrawDynamicMeshSceneProxy(this);

		if (TriangleColorFunc)
		{
			NewProxy->bUsePerTriangleColor = true;
			NewProxy->PerTriangleColorFunc = [this](const FDynamicMesh3* MeshIn, int TriangleID) { return GetTriangleColor(MeshIn, TriangleID); };
		}
		else if ( GetColorOverrideMode() == EDynamicMeshComponentColorOverrideMode::Polygroups )
		{
			NewProxy->bUsePerTriangleColor = true;
			NewProxy->PerTriangleColorFunc = [this](const FDynamicMesh3* MeshIn, int TriangleID) { return GetGroupColor(MeshIn, TriangleID); };
		}

		if (SecondaryTriFilterFunc)
		{
			NewProxy->bUseSecondaryTriBuffers = true;
			NewProxy->SecondaryTriFilterFunc = [this](const FDynamicMesh3* MeshIn, int32 TriangleID) 
			{ 
				return (SecondaryTriFilterFunc) ? SecondaryTriFilterFunc(MeshIn, TriangleID) : false;
			};
		}

		if (Decomposition)
		{
			NewProxy->InitializeFromDecomposition(Decomposition);
		}
		else
		{
			NewProxy->Initialize();
		}

		NewProxy->SetVerifyUsedMaterials(bProxyVerifyUsedMaterials);
	}

	bProxyValid = true;
	return NewProxy;
}

bool UMultiDrawDynamicMeshComponent::ChangeMaterialByIndex(UMaterialInterface* InMaterialInstance, int32 Index)
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

bool UMultiDrawDynamicMeshComponent::ChangeMultiDrawSettingsByIndex(FMultiDrawSettings InSettings, int32 Index)
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
