// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#include "Outline/OutlineDynamicMeshComponent.h"
#include "Outline/OutlineDynamicMeshSceneProxy.h"

UOutlineDynamicMeshComponent::UOutlineDynamicMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),CullingMode(EMultiDrawCullingMode::FrontfaceCulling),bCastShadow(false)
{

}

void UOutlineDynamicMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
{
	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);

	for(UMaterialInterface* Material : OutlinePassMaterialSet)
	{
		OutMaterials.Add(Material);
	}
}

FMaterialRelevance UOutlineDynamicMeshComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
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
	for(UMaterialInterface* Material : OutlinePassMaterialSet)
	{
		UMaterialInterface* MaterialInterface = Material ? Material :  UMaterial::GetDefaultMaterial(MD_Surface);
		Result |= MaterialInterface->GetRelevance_Concurrent(InFeatureLevel);
	}
	
	return Result;
}

FPrimitiveSceneProxy* UOutlineDynamicMeshComponent::CreateSceneProxy()
{
	// if this is not always the case, we have made incorrect assumptions
	ensure(GetCurrentSceneProxy() == nullptr);

	FOutlineDynamicMeshSceneProxy* NewProxy = nullptr;
	if (GetMesh()->TriangleCount() > 0)
	{
		NewProxy = new FOutlineDynamicMeshSceneProxy(this, CullingMode, bCastShadow);

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

bool UOutlineDynamicMeshComponent::ChangeMaterialByIndex(UMaterialInterface* InMaterialInstance, int32 Index)
{
	if(OutlinePassMaterialSet.Num() < Index+1 || !IsValid(InMaterialInstance))
		return false;

	OutlinePassMaterialSet[Index]=InMaterialInstance;
	return true;
}