// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#include "Outline/OutlineSkeletalMeshComponent.h"
#include "Outline/OutlineSkeletalMeshSceneProxy.h"
#include "MaterialDomain.h"
#include "Engine/Public/Rendering/SkeletalMeshRenderData.h"
#include "Engine/Public/SkeletalRenderPublic.h"
#include "Materials/MaterialInstanceDynamic.h"
#define LOCTEXT_NAMESPACE "OutlineSkeletalMeshComponent"

UOutlineSkeletalMeshComponent::UOutlineSkeletalMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),CullingMode(EMultiDrawCullingMode::FrontfaceCulling),bCastShadow(false)
{

}

void UOutlineSkeletalMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
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

		for(UMaterialInterface* Material : OutlinePassMaterialSet)
		{
			OutMaterials.Add(Material);
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

FMaterialRelevance UOutlineSkeletalMeshComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	FMaterialRelevance Result=Super::GetMaterialRelevance(InFeatureLevel);
	for(UMaterialInterface* Material : OutlinePassMaterialSet)
	{
		UMaterialInterface* MaterialInterface = Material ? Material :  UMaterial::GetDefaultMaterial(MD_Surface);
		Result |= MaterialInterface->GetRelevance_Concurrent(InFeatureLevel);
	}
	
	return Result;
}

FPrimitiveSceneProxy* UOutlineSkeletalMeshComponent::CreateSceneProxy()
{
	ERHIFeatureLevel::Type SceneFeatureLevel = GetWorld()->FeatureLevel;

	FOutlineSkeletalMeshSceneProxy* Result = nullptr;
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
		int32 MaxSupportedNumBones = MeshObject->IsCPUSkinned() ? MAX_int32 : FGPUBaseSkinVertexFactory::GetMaxGPUSkinBones();
		if (MaxBonesPerChunk <= MaxSupportedNumBones)
		{
			Result = ::new FOutlineSkeletalMeshSceneProxy(this, SkelMeshRenderData,CullingMode,bCastShadow);
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	SendRenderDebugPhysics(Result);
#endif

	return Result;
}

bool UOutlineSkeletalMeshComponent::ChangeMaterialByIndex(UMaterialInterface* InMaterialInstance, int32 Index)
{
	if(OutlinePassMaterialSet.Num() < Index+1 || !IsValid(InMaterialInstance))
		return false;
	
	UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(InMaterialInstance);
	if (!((DynamicMaterial != nullptr && DynamicMaterial->Parent == OutlinePassMaterialSet[Index]) || OutlinePassMaterialSet[Index] == nullptr))
	{
		// Mark cached material parameter names dirty
		MarkCachedMaterialParameterNameIndicesDirty();
	}
	
	// Set the material and invalidate things
	OutlinePassMaterialSet[Index]=InMaterialInstance;
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
#if WITH_EDITOR
void UOutlineSkeletalMeshComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bool bNeedsRecompile = true;
	FProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if ( PropertyThatChanged != nullptr )
	{
		if(PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED( UOutlineSkeletalMeshComponent, OutlinePassMaterialSet))
		{
			if (OutlinePassMaterialSet.Num())
			{
				//Check bUsedWithSkeletalMesh Is True
				for(int i=0;i<OutlinePassMaterialSet.Num();i++)
				{
					if(OutlinePassMaterialSet[i]==nullptr)
						continue;
					
					if(!OutlinePassMaterialSet[i]->CheckMaterialUsage(EMaterialUsage::MATUSAGE_SkeletalMesh))
					{
						
						OutlinePassMaterialSet[i]->GetMaterial()->SetMaterialUsage(bNeedsRecompile,EMaterialUsage::MATUSAGE_SkeletalMesh);
					}
				}
				
				USkeletalMesh* SkeletalMeshPtr=GetSkeletalMeshAsset();
				//Check Has Cloth Data
				if(SkeletalMeshPtr)
				{
					FSkeletalMeshRenderData* SkelMeshRenderData = GetSkeletalMeshRenderData();
					const FSkeletalMeshLODRenderData& LODData = SkelMeshRenderData->LODRenderData[0];
					for(int32 SectionIndex = 0;SectionIndex < LODData.RenderSections.Num();SectionIndex++)
					{
						if(OutlinePassMaterialSet.Num() < (SectionIndex+1))
							break;
						if(OutlinePassMaterialSet[SectionIndex] == nullptr)
							continue;
						const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];
						bool bClothSection = Section.HasClothingData();
						if (bClothSection)
						{
							OutlinePassMaterialSet[SectionIndex]->GetMaterial()->SetMaterialUsage(bNeedsRecompile,EMaterialUsage::MATUSAGE_Clothing);
						}
					}
				}
				
				MarkCachedMaterialParameterNameIndicesDirty();
				MarkRenderStateDirty();
			}
		}
	}
}
#endif
#undef LOCTEXT_NAMESPACE