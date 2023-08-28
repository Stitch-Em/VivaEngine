// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#include "MultiDraw/MultiDrawSkeletalMeshComponent.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "MultiDraw/MultiDrawSkeletalMeshSceneProxy.h"
#include "Engine/Public/Rendering/SkeletalMeshRenderData.h"
#include "Engine/Public/SkeletalRenderPublic.h"
#include "Materials/MaterialInstanceDynamic.h"

#define LOCTEXT_NAMESPACE "MultiDrawSkeletalMeshComponent"

UMultiDrawSkeletalMeshComponent::UMultiDrawSkeletalMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super()
{

}

void UMultiDrawSkeletalMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
{
	USkeletalMesh* SkeletalMeshPtr=GetSkeletalMeshAsset();
	if (SkeletalMeshPtr)
	{
		// The max number of materials used is the max of the materials on the skeletal mesh and the materials on the mesh component
		const int32 NumMaterials = FMath::Max(SkeletalMeshPtr->GetMaterials().Num(), OverrideMaterials.Num());
		for (int32 MatIdx = 0; MatIdx < NumMaterials; ++MatIdx)
		{
			// GetMaterial will determine the correct material to use for this index.  

			UMaterialInterface* MaterialInterface = GetMaterial(MatIdx);
			OutMaterials.Add(MaterialInterface);
		}
		for (const auto& MultiDrawMaterial : PerPassDrawSettingses) 
		{
			if (MultiDrawMaterial.bDraw && MultiDrawMaterial.MaterialInterface && MultiDrawMaterial.ElementIndex <= NumMaterials)
			{
				if(MultiDrawMaterial.MaterialInterface->CheckMaterialUsage_Concurrent(MATUSAGE_SkeletalMesh))
					OutMaterials.Add(MultiDrawMaterial.MaterialInterface);
				else
					OutMaterials.Add(UMaterial::GetDefaultMaterial(MD_Surface));
			}
		}

		if (OverlayMaterial != nullptr)
		{
			OutMaterials.Add(OverlayMaterial);
		}

		SkeletalMeshPtr->UpdateUVChannelData(false);
	}

	if (bGetDebugMaterials)
	{
#if WITH_EDITOR
		//if (UPhysicsAsset* PhysicsAssetForDebug = GetPhysicsAsset())
		//{
		//	PhysicsAssetForDebug->GetUsedMaterials(OutMaterials);
		//}
#endif
	}
}

FMaterialRelevance UMultiDrawSkeletalMeshComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result=Super::GetMaterialRelevance(InFeatureLevel);
	for(const FMultiDrawSettings& Material : PerPassDrawSettingses)
	{
		UMaterialInterface* MaterialInterface = Material.MaterialInterface ? Material.MaterialInterface :  UMaterial::GetDefaultMaterial(MD_Surface);
		Result |= MaterialInterface->GetRelevance_Concurrent(InFeatureLevel);
	}
	
	return Result;
}

FPrimitiveSceneProxy* UMultiDrawSkeletalMeshComponent::CreateSceneProxy()
{
	ERHIFeatureLevel::Type SceneFeatureLevel = GetWorld()->FeatureLevel;

	FMultiDrawSkeletalMeshSceneProxy* Result = nullptr;
	//FSkeletalMeshSceneProxy* Result = nullptr;
	FSkeletalMeshRenderData* SkelMeshRenderData = GetSkeletalMeshRenderData();

	// Only create a scene proxy for rendering if properly initialized
	if (SkelMeshRenderData &&
		SkelMeshRenderData->LODRenderData.IsValidIndex(GetPredictedLODLevel()) &&
		!bHideSkin &&
		MeshObject)
	{
		// Only create a scene proxy if the bone count being used is supported, or if we don't have a skeleton (this is the case with destructibles)
		int32 MaxBonesPerChunk = SkelMeshRenderData->GetMaxBonesPerSection();
		int32 MaxSupportedNumBones = MAX_int32;
		if (MaxBonesPerChunk <= MaxSupportedNumBones)
		{
			Result = ::new FMultiDrawSkeletalMeshSceneProxy(this, SkelMeshRenderData);
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	SendRenderDebugPhysics(Result);
#endif

	return Result;
}

bool UMultiDrawSkeletalMeshComponent::ChangeMaterialByIndex(UMaterialInterface* InMaterialInstance, int32 Index)
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

bool UMultiDrawSkeletalMeshComponent::ChangeMultiDrawSettingsByIndex(FMultiDrawSettings InSettings, int32 Index)
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

#if WITH_EDITOR
void UMultiDrawSkeletalMeshComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bool bNeedsRecompile = true;
	FProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if ( PropertyThatChanged != nullptr )
	{
		if(PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED( UMultiDrawSkeletalMeshComponent, PerPassDrawSettingses))
		{
			if (PerPassDrawSettingses.Num())
			{
				//Check Has Cloth Data
				bool bClothSection=false;
				USkeletalMesh* SkeletalMeshPtr=GetSkeletalMeshAsset();
				if(SkeletalMeshPtr)
				{
					FSkeletalMeshRenderData* SkelMeshRenderData = GetSkeletalMeshRenderData();
					const FSkeletalMeshLODRenderData& LODData = SkelMeshRenderData->LODRenderData[0];
					for(int32 SectionIndex = 0;SectionIndex < LODData.RenderSections.Num();SectionIndex++)
					{
						const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];
						if(Section.HasClothingData())
						{
							bClothSection = true;
						}
					}
				}
				
				//Check bUsedWithSkeletalMesh Is True
				for(int i=0;i<PerPassDrawSettingses.Num();i++)
				{
					auto Material=PerPassDrawSettingses[i].MaterialInterface;
					if(Material==nullptr)
						continue;
					
					if(!Material->CheckMaterialUsage(EMaterialUsage::MATUSAGE_SkeletalMesh))
					{
						Material->GetMaterial()->SetMaterialUsage(bNeedsRecompile,EMaterialUsage::MATUSAGE_SkeletalMesh);
					}

					if (bClothSection)
					{
						Material->GetMaterial()->SetMaterialUsage(bNeedsRecompile,EMaterialUsage::MATUSAGE_Clothing);
					}
				}
				
				MarkCachedMaterialParameterNameIndicesDirty();
				MarkRenderStateDirty();
			}
		}
	}
}
#endif

//UMaterialInterface* UMultiDrawSkeletalMeshComponent::GetMaterial(int32 MaterialIndex) const
//{
//	if (OverrideMaterials.IsValidIndex(MaterialIndex) && OverrideMaterials[MaterialIndex])
//	{
//		return OverrideMaterials[MaterialIndex];
//	}
//	else if (SkeletalMesh && SkeletalMesh->Materials.IsValidIndex(MaterialIndex) && SkeletalMesh->Materials[MaterialIndex].MaterialInterface)
//	{
//		return SkeletalMesh->Materials[MaterialIndex].MaterialInterface;
//	}
//	return nullptr;
//}

#undef LOCTEXT_NAMESPACE