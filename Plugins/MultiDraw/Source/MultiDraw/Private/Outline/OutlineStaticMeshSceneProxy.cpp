// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#include "Outline/OutlineStaticMeshSceneProxy.h"
#include "Outline/OutlineStaticMeshComponent.h"
#include "Engine/Public/MaterialShared.h"
#define LOCTEXT_NAMESPACE "OutlineStaticMeshSceneProxy"

FOutlineStaticMeshSceneProxy::FOutlineStaticMeshSceneProxy(UStaticMeshComponent* Component, bool bForceLODsShareStaticLighting,EMultiDrawCullingMode CullingMode,bool bCastShadow)
	:FStaticMeshSceneProxy(Component,bForceLODsShareStaticLighting)
	,ComponentPtr(Component)
	,CullingMode(CullingMode)
	,bCastShadow(bCastShadow)
	,OverlayMaterial(Component->OverlayMaterial)
	,OverlayMaterialMaxDrawDistance(Component->OverlayMaterialMaxDrawDistance)
{
}

void FOutlineStaticMeshSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
	checkSlow(IsInParallelRenderingThread());
	if (!HasViewDependentDPG())
	{
		// Determine the DPG the primitive should be drawn in.
		ESceneDepthPriorityGroup PrimitiveDPG = GetStaticDepthPriorityGroup();
		int32 NumLODs = RenderData->LODResources.Num();
		//Never use the dynamic path in this path, because only unselected elements will use DrawStaticElements
		bool bIsMeshElementSelected = false;
		const auto FeatureLevel = GetScene().GetFeatureLevel();
		const bool IsMobile = IsMobilePlatform(GetScene().GetShaderPlatform());
		const int32 NumRuntimeVirtualTextureTypes = RuntimeVirtualTextureMaterialTypes.Num();

		const UOutlineStaticMeshComponent* OutlineStaticMeshComponent = dynamic_cast<const UOutlineStaticMeshComponent *>(ComponentPtr);
		auto OutlinePassMaterialSet=OutlineStaticMeshComponent->OutlinePassMaterialSet;

		//check if a LOD is being forced
		if (ForcedLodModel > 0) 
		{
			int32 LODIndex = FMath::Clamp(ForcedLodModel, ClampedMinLOD + 1, NumLODs) - 1;

			const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
			
			// Draw the static mesh elements.
			for(int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
			{
			#if WITH_EDITOR
				if (GIsEditor)
				{
					const FLODInfo::FSectionInfo& Section = LODs[LODIndex].Sections[SectionIndex];
					bIsMeshElementSelected = Section.bSelected;
					PDI->SetHitProxy(Section.HitProxy);
				}
			#endif

				const int32 NumBatches = GetNumMeshBatches();
				PDI->ReserveMemoryForMeshes(NumBatches * (1 + NumRuntimeVirtualTextureTypes));

				for (int32 BatchIndex = 0; BatchIndex < NumBatches; BatchIndex++)
				{
					FMeshBatch BaseMeshBatch;

					if (GetMeshElement(LODIndex, BatchIndex, SectionIndex, PrimitiveDPG, bIsMeshElementSelected, true, BaseMeshBatch))
					{
						if (NumRuntimeVirtualTextureTypes > 0)
						{
							// Runtime virtual texture mesh elements.
							FMeshBatch MeshBatch(BaseMeshBatch);
							MultiDraw::SetupMeshBatchForRuntimeVirtualTexture(MeshBatch);
							for (ERuntimeVirtualTextureMaterialType MaterialType : RuntimeVirtualTextureMaterialTypes)
							{
								MeshBatch.RuntimeVirtualTextureMaterialType = (uint32)MaterialType;
								PDI->DrawMesh(MeshBatch, FLT_MAX);

								if(OutlinePassMaterialSet.Num()<=0)
									continue;
								UMaterialInterface* CurrentOutlineMaterial=nullptr;
								if(SectionIndex < OutlinePassMaterialSet.Num())
								{
									CurrentOutlineMaterial=OutlinePassMaterialSet[SectionIndex];
								}
    
								if (CurrentOutlineMaterial == nullptr)
									continue;
								
								const EBlendMode& BlendMode=CurrentOutlineMaterial->GetBlendMode();
								const bool IsTranslucent=BlendMode != BLEND_Opaque && BlendMode != BLEND_Masked;
								if(IsTranslucent)
									MeshBatch.bOverlayMaterial=true;
								
								MeshBatch.MaterialRenderProxy = CurrentOutlineMaterial->GetRenderProxy();
								MeshBatch.CastShadow=bCastShadow;
								switch (CullingMode)
								{
								case EMultiDrawCullingMode::FrontfaceCulling:
									MeshBatch.ReverseCulling = !MeshBatch.ReverseCulling;
									break;
								case  EMultiDrawCullingMode::BackfaceCulling:
									break;
								case  EMultiDrawCullingMode::DoubleSide:
									MeshBatch.bDisableBackfaceCulling=true;
									break;
								}
    
								PDI->DrawMesh(MeshBatch, FLT_MAX);
							}
						}
						{
							PDI->DrawMesh(BaseMeshBatch, FLT_MAX);

							if(OutlinePassMaterialSet.Num()<=0)
								continue;
							UMaterialInterface* CurrentOutlineMaterial=nullptr;
							if(SectionIndex < OutlinePassMaterialSet.Num())
							{
								CurrentOutlineMaterial=OutlinePassMaterialSet[SectionIndex];
							}
    
							if (CurrentOutlineMaterial == nullptr)
								continue;
							
							const EBlendMode& BlendMode=CurrentOutlineMaterial->GetBlendMode();
							const bool IsTranslucent=BlendMode != BLEND_Opaque && BlendMode != BLEND_Masked;
							if(IsTranslucent)
								BaseMeshBatch.bOverlayMaterial=true;

							BaseMeshBatch.MaterialRenderProxy = CurrentOutlineMaterial->GetRenderProxy();
							BaseMeshBatch.CastShadow=bCastShadow;
							switch (CullingMode)
							{
							case EMultiDrawCullingMode::FrontfaceCulling:
								BaseMeshBatch.ReverseCulling = !BaseMeshBatch.ReverseCulling;
								break;
							case  EMultiDrawCullingMode::BackfaceCulling:
								break;
							case  EMultiDrawCullingMode::DoubleSide:
								BaseMeshBatch.bDisableBackfaceCulling=true;
								break;
							}
    
							PDI->DrawMesh(BaseMeshBatch, FLT_MAX);
						}
						if (OverlayMaterial != nullptr)
						{
							FMeshBatch OverlayMeshBatch(BaseMeshBatch);
							OverlayMeshBatch.bOverlayMaterial = true;
							OverlayMeshBatch.CastShadow = false;
							OverlayMeshBatch.bSelectable = false;
							OverlayMeshBatch.MaterialRenderProxy = OverlayMaterial->GetRenderProxy();
							// make sure overlay is always rendered on top of base mesh
							OverlayMeshBatch.MeshIdInPrimitive += LODModel.Sections.Num();
							PDI->DrawMesh(OverlayMeshBatch, FLT_MAX);
						}
					}
				}
			}
		} 
		else //no LOD is being forced, submit them all with appropriate cull distances
		{
			for (int32 LODIndex = ClampedMinLOD; LODIndex < NumLODs; ++LODIndex)
			{
				const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
				float ScreenSize = GetScreenSize(LODIndex);

				bool bUseUnifiedMeshForShadow = false;
				bool bUseUnifiedMeshForDepth = false;

				if (MultiDraw::GUseShadowIndexBuffer && LODModel.bHasDepthOnlyIndices)
				{
					const FLODInfo& ProxyLODInfo = LODs[LODIndex];

					// The shadow-only mesh can be used only if all elements cast shadows and use opaque materials with no vertex modification.
					bool bSafeToUseUnifiedMesh = true;

					bool bAnySectionUsesDitheredLODTransition = false;
					bool bAllSectionsUseDitheredLODTransition = true;
					bool bIsMovable = IsMovable();
					bool bAllSectionsCastShadow = bCastShadow;

					for (int32 SectionIndex = 0; bSafeToUseUnifiedMesh && SectionIndex < LODModel.Sections.Num(); SectionIndex++)
					{
						const FMaterial& Material = ProxyLODInfo.Sections[SectionIndex].Material->GetRenderProxy()->GetIncompleteMaterialWithFallback(FeatureLevel);
						// no support for stateless dithered LOD transitions for movable meshes
						bAnySectionUsesDitheredLODTransition = bAnySectionUsesDitheredLODTransition || (!bIsMovable && Material.IsDitheredLODTransition());
						bAllSectionsUseDitheredLODTransition = bAllSectionsUseDitheredLODTransition && (!bIsMovable && Material.IsDitheredLODTransition());
						const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];

						bSafeToUseUnifiedMesh =
							!(bAnySectionUsesDitheredLODTransition && !bAllSectionsUseDitheredLODTransition) // can't use a single section if they are not homogeneous
							&& Material.WritesEveryPixel()
							&& !Material.IsTwoSided()
							&& !IsTranslucentBlendMode(Material.GetBlendMode())
							&& !Material.MaterialModifiesMeshPosition_RenderThread()
							&& Material.GetMaterialDomain() == MD_Surface
							&& !Material.IsSky()
							&& !Material.GetShadingModels().HasShadingModel(MSM_SingleLayerWater);

						bAllSectionsCastShadow &= Section.bCastShadow;
					}

					if (bSafeToUseUnifiedMesh)
					{
						bUseUnifiedMeshForShadow = bAllSectionsCastShadow;

						// Depth pass is only used for deferred renderer. The other conditions are meant to match the logic in FDepthPassMeshProcessor::AddMeshBatch.
						bUseUnifiedMeshForDepth = ShouldUseAsOccluder() && GetScene().GetShadingPath() == EShadingPath::Deferred && !IsMovable();

						if (bUseUnifiedMeshForShadow || bUseUnifiedMeshForDepth)
						{
							const int32 NumBatches = GetNumMeshBatches();

							PDI->ReserveMemoryForMeshes(NumBatches);

							for (int32 BatchIndex = 0; BatchIndex < NumBatches; BatchIndex++)
							{
								FMeshBatch MeshBatch;

								if (GetShadowMeshElement(LODIndex, BatchIndex, PrimitiveDPG, MeshBatch, bAllSectionsUseDitheredLODTransition))
								{
									bUseUnifiedMeshForShadow = bAllSectionsCastShadow;

									MeshBatch.CastShadow = bUseUnifiedMeshForShadow;
									MeshBatch.bUseForDepthPass = bUseUnifiedMeshForDepth;
									MeshBatch.bUseAsOccluder = bUseUnifiedMeshForDepth;
									MeshBatch.bUseForMaterial = false;

									PDI->DrawMesh(MeshBatch, ScreenSize);
								}
							}
						}
					}
				}

				// Draw the static mesh elements.
				for(int32 SectionIndex = 0;SectionIndex < LODModel.Sections.Num();SectionIndex++)
				{
#if WITH_EDITOR
					if( GIsEditor )
					{
						const FLODInfo::FSectionInfo& Section = LODs[LODIndex].Sections[SectionIndex];

						bIsMeshElementSelected = Section.bSelected;
						PDI->SetHitProxy(Section.HitProxy);
					}
#endif // WITH_EDITOR

					const int32 NumBatches = GetNumMeshBatches();
					PDI->ReserveMemoryForMeshes(NumBatches * (1 + NumRuntimeVirtualTextureTypes));

					for (int32 BatchIndex = 0; BatchIndex < NumBatches; BatchIndex++)
					{
						FMeshBatch BaseMeshBatch;
						if (GetMeshElement(LODIndex, BatchIndex, SectionIndex, PrimitiveDPG, bIsMeshElementSelected, true, BaseMeshBatch))
						{
							if (NumRuntimeVirtualTextureTypes > 0)
							{
								// Runtime virtual texture mesh elements.
								FMeshBatch MeshBatch(BaseMeshBatch);
								MultiDraw::SetupMeshBatchForRuntimeVirtualTexture(MeshBatch);

								for (ERuntimeVirtualTextureMaterialType MaterialType : RuntimeVirtualTextureMaterialTypes)
								{
									MeshBatch.RuntimeVirtualTextureMaterialType = (uint32)MaterialType;
									PDI->DrawMesh(MeshBatch, ScreenSize);

									if(OutlinePassMaterialSet.Num()<=0)
										continue;
									UMaterialInterface* CurrentOutlineMaterial=nullptr;
									if(SectionIndex < OutlinePassMaterialSet.Num())
									{
										CurrentOutlineMaterial=OutlinePassMaterialSet[SectionIndex];
									}
    
									if (CurrentOutlineMaterial == nullptr)
										continue;
									
									const EBlendMode& BlendMode=CurrentOutlineMaterial->GetBlendMode();
									const bool IsTranslucent=BlendMode != BLEND_Opaque && BlendMode != BLEND_Masked;
									if(IsTranslucent)
										MeshBatch.bOverlayMaterial=true;
									
									MeshBatch.MaterialRenderProxy = CurrentOutlineMaterial->GetRenderProxy();
									MeshBatch.CastShadow=bCastShadow;
									switch (CullingMode)
									{
									case EMultiDrawCullingMode::FrontfaceCulling:
										MeshBatch.ReverseCulling = !MeshBatch.ReverseCulling;
										break;
									case  EMultiDrawCullingMode::BackfaceCulling:
										break;
									case  EMultiDrawCullingMode::DoubleSide:
										MeshBatch.bDisableBackfaceCulling=true;
										break;
									}
    
									PDI->DrawMesh(MeshBatch, FLT_MAX);
								}
							}

							{
								// Standard mesh elements.
								// If we have submitted an optimized shadow-only mesh, remaining mesh elements must not cast shadows.
								FMeshBatch MeshBatch(BaseMeshBatch);
								MeshBatch.CastShadow &= !bUseUnifiedMeshForShadow;
								MeshBatch.bUseAsOccluder &= !bUseUnifiedMeshForDepth;
								MeshBatch.bUseForDepthPass &= !bUseUnifiedMeshForDepth;
								PDI->DrawMesh(MeshBatch, ScreenSize);

								if(SectionIndex >= OutlinePassMaterialSet.Num())
									continue;
								
								UMaterialInterface* CurrentOutlineMaterial=OutlinePassMaterialSet[SectionIndex];
								if (CurrentOutlineMaterial == nullptr)
									continue;

								const EBlendMode& BlendMode=CurrentOutlineMaterial->GetBlendMode();
								const bool IsTranslucent=BlendMode != BLEND_Opaque && BlendMode != BLEND_Masked;
								if(IsTranslucent)
									MeshBatch.bOverlayMaterial=true;
								
								MeshBatch.MaterialRenderProxy = CurrentOutlineMaterial->GetRenderProxy();
								MeshBatch.CastShadow=bCastShadow;
								switch (CullingMode)
								{
								case EMultiDrawCullingMode::FrontfaceCulling:
									MeshBatch.ReverseCulling = !MeshBatch.ReverseCulling;
									break;
								case  EMultiDrawCullingMode::BackfaceCulling:
									break;
								case  EMultiDrawCullingMode::DoubleSide:
									MeshBatch.bDisableBackfaceCulling=true;
									break;
								}
    
								PDI->DrawMesh(MeshBatch, FLT_MAX);
							}

							if (OverlayMaterial != nullptr)
							{
								FMeshBatch OverlayMeshBatch(BaseMeshBatch);
								OverlayMeshBatch.bOverlayMaterial = true;
								OverlayMeshBatch.CastShadow = false;
								OverlayMeshBatch.bSelectable = false;
								OverlayMeshBatch.MaterialRenderProxy = OverlayMaterial->GetRenderProxy();
								// make sure overlay is always rendered on top of base mesh
								OverlayMeshBatch.MeshIdInPrimitive += LODModel.Sections.Num();
								// Reuse mesh ScreenSize as cull distance for an overlay. Overlay does not need to compute LOD so we can avoid adding new members into MeshBatch or MeshRelevance
								float OverlayMeshScreenSize = OverlayMaterialMaxDrawDistance;
								PDI->DrawMesh(OverlayMeshBatch, OverlayMeshScreenSize);
							}
						}
					}
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE


