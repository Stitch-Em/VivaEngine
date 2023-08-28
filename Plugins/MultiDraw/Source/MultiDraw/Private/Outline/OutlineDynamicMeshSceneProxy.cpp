// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#include "Outline/OutlineDynamicMeshSceneProxy.h"

FOutlineDynamicMeshSceneProxy::FOutlineDynamicMeshSceneProxy(UOutlineDynamicMeshComponent* Component, EMultiDrawCullingMode CullingMode,bool bCastShadow)
	:FBaseDynamicMeshSceneProxy(Component)
	,CullingMode(CullingMode)
	,bCastShadow(bCastShadow)
	,MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
{
	ParentComponent = Component;
}

void FOutlineDynamicMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views,
	const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_BaseDynamicMeshSceneProxy_GetDynamicMeshElements);

	bool bWireframe = (AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe)
		|| ParentBaseComponent->GetEnableWireframeRenderPass();

	// Get wireframe material proxy if requested and available, otherwise disable wireframe
	FMaterialRenderProxy* WireframeMaterialProxy = nullptr;
	if (bWireframe)
	{
		UMaterialInterface* WireframeMaterial = GEngine->WireframeMaterial;
		if (WireframeMaterial != nullptr)
		{
			FColoredMaterialRenderProxy* WireframeMaterialInstance = new FColoredMaterialRenderProxy(
				WireframeMaterial->GetRenderProxy(), ParentBaseComponent->WireframeColor);
			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
			WireframeMaterialProxy = WireframeMaterialInstance;
		}
		else
		{
			bWireframe = false;
		}
	}

	// will use this material instead of any others below, if it becomes non-null
	UMaterialInterface* ForceOverrideMaterial = nullptr;

	// Note: there is an engine show flags for vertex colors. However if we enable that, then
	// we will fail the used-materials checks in the Editor, because UBaseDynamicMeshComponent::GetUsedMaterials()
	// cannot check the active show flags. One option would be to always add the vertex color material there...
	const bool bVertexColor = ParentBaseComponent->ColorMode == EDynamicMeshComponentColorOverrideMode::VertexColors ||
								ParentBaseComponent->ColorMode == EDynamicMeshComponentColorOverrideMode::Polygroups ||
								ParentBaseComponent->ColorMode == EDynamicMeshComponentColorOverrideMode::Constant;
	if (bVertexColor)
	{
		//ForceOverrideMaterial = UBaseDynamicMeshComponent::GetDefaultVertexColorMaterial_RenderThread();
		ForceOverrideMaterial = GEngine->VertexColorViewModeMaterial_ColorOnly;
	}

	ESceneDepthPriorityGroup DepthPriority = SDPG_World;

	TArray<FMeshRenderBufferSet*> Buffers;
	GetActiveRenderBufferSets(Buffers);

	UMaterialInterface* UseSecondaryMaterial = ForceOverrideMaterial;
	if (ParentBaseComponent->HasSecondaryRenderMaterial() && ForceOverrideMaterial == nullptr)
	{
		UseSecondaryMaterial = ParentBaseComponent->GetSecondaryRenderMaterial();
	}
	bool bDrawSecondaryBuffers = ParentBaseComponent->GetSecondaryBuffersVisibility();

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];

			bool bHasPrecomputedVolumetricLightmap;
			FMatrix PreviousLocalToWorld;
			int32 SingleCaptureIndex;
			bool bOutputVelocity;
			GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);
			bOutputVelocity |= AlwaysHasVelocity();

			// Draw the mesh.
			for (FMeshRenderBufferSet* BufferSet : Buffers)
			{
				UMaterialInterface* UseMaterial = BufferSet->Material;
				if (ParentBaseComponent->HasOverrideRenderMaterial(0))
				{
					UseMaterial = ParentBaseComponent->GetOverrideRenderMaterial(0);
				}
				if (ForceOverrideMaterial)
				{
					UseMaterial = ForceOverrideMaterial;
				}
				FMaterialRenderProxy* MaterialProxy = UseMaterial->GetRenderProxy();

				if (BufferSet->TriangleCount == 0)
				{
					continue;
				}

				// lock buffers so that they aren't modified while we are submitting them
				FScopeLock BuffersLock(&BufferSet->BuffersLock);

				// do we need separate one of these for each MeshRenderBufferSet?
				FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
				DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, bOutputVelocity, GetCustomPrimitiveData());

				if (BufferSet->IndexBuffer.Indices.Num() > 0)
				{
					// Unlike most meshes, which just use the wireframe material in wireframe mode, we draw the wireframe on top of the normal material if needed,
					// as this is easier to interpret. However, we do not do this in ortho viewports, where it frequently causes the our edit gizmo to be hidden 
					// beneath the material. So, only draw the base material if we are in perspective mode, or we're in ortho but not in wireframe.
					if (View->IsPerspectiveProjection() || !(AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe))
					{
						DrawBatch(Collector, *BufferSet, BufferSet->IndexBuffer, MaterialProxy, false, DepthPriority, ViewIndex, DynamicPrimitiveUniformBuffer);
					}
					if (bWireframe)
					{
						DrawBatch(Collector, *BufferSet, BufferSet->IndexBuffer, WireframeMaterialProxy, true, DepthPriority, ViewIndex, DynamicPrimitiveUniformBuffer);
					}
				}

				// draw secondary buffer if we have it, falling back to base material if we don't have the Secondary material
				FMaterialRenderProxy* UseSecondaryMaterialProxy = (UseSecondaryMaterial != nullptr) ? UseSecondaryMaterial->GetRenderProxy() : MaterialProxy;
				if (bDrawSecondaryBuffers && BufferSet->SecondaryIndexBuffer.Indices.Num() > 0 && UseSecondaryMaterialProxy != nullptr)
				{
					DrawBatch(Collector, *BufferSet, BufferSet->SecondaryIndexBuffer, UseSecondaryMaterialProxy, false, DepthPriority, ViewIndex, DynamicPrimitiveUniformBuffer);
					if (bWireframe)
					{
						DrawBatch(Collector, *BufferSet, BufferSet->SecondaryIndexBuffer, UseSecondaryMaterialProxy, true, DepthPriority, ViewIndex, DynamicPrimitiveUniformBuffer);
					}
				}

				//Outline
				auto OutlinePassMaterialSet=ParentComponent->OutlinePassMaterialSet;
				const int32 MaterialIndex = Buffers.IndexOfByKey(BufferSet);
				if(BufferSet->IndexBuffer.Indices.Num() <= 0 ||  MaterialIndex > OutlinePassMaterialSet.Num()-1)
					continue;
				
				const UMaterialInterface* CurrentOutlineMaterial=OutlinePassMaterialSet[MaterialIndex];
				if(CurrentOutlineMaterial == nullptr)
					continue;
				
				if (View->IsPerspectiveProjection() || !(AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe))
				{
					DrawOutlineBatch(Collector, *BufferSet, BufferSet->IndexBuffer, CurrentOutlineMaterial->GetRenderProxy(), false, DepthPriority, ViewIndex, DynamicPrimitiveUniformBuffer);
				}
				
			}
		}
	}
}

void FOutlineDynamicMeshSceneProxy::DrawBatch(FMeshElementCollector& Collector,
	const FMeshRenderBufferSet& RenderBuffers, const FDynamicMeshIndexBuffer32& IndexBuffer,
	FMaterialRenderProxy* UseMaterial, bool bWireframe, ESceneDepthPriorityGroup DepthPriority, int ViewIndex,
	FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer) const
{
	FMeshBatch& Mesh = Collector.AllocateMesh();
	FMeshBatchElement& BatchElement = Mesh.Elements[0];
	BatchElement.IndexBuffer = &IndexBuffer;
	Mesh.bWireframe = bWireframe;
	Mesh.VertexFactory = &RenderBuffers.VertexFactory;
	Mesh.MaterialRenderProxy = UseMaterial;

	BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = RenderBuffers.PositionVertexBuffer.GetNumVertices() - 1;
	Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
	Mesh.Type = PT_TriangleList;
	Mesh.DepthPriorityGroup = DepthPriority;
	Mesh.bCanApplyViewModeOverrides = this->bEnableViewModeOverrides;
	Collector.AddMesh(ViewIndex, Mesh);
}

void FOutlineDynamicMeshSceneProxy::DrawOutlineBatch(FMeshElementCollector& Collector,
	const FMeshRenderBufferSet& RenderBuffers, const FDynamicMeshIndexBuffer32& IndexBuffer,
	FMaterialRenderProxy* UseMaterial, bool bWireframe, ESceneDepthPriorityGroup DepthPriority, int ViewIndex,
	FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer) const
{
	FMeshBatch& Mesh = Collector.AllocateMesh();
	FMeshBatchElement& BatchElement = Mesh.Elements[0];
	BatchElement.IndexBuffer = &IndexBuffer;
	Mesh.bWireframe = bWireframe;
	Mesh.VertexFactory = &RenderBuffers.VertexFactory;
	Mesh.MaterialRenderProxy = UseMaterial;

	BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = RenderBuffers.PositionVertexBuffer.GetNumVertices() - 1;
	
	Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();

	FMeshBatch& OutlineMesh = Collector.AllocateMesh();
	FMeshBatchElement& OutlineMeshBatchElement = OutlineMesh.Elements[0];
	OutlineMeshBatchElement.IndexBuffer = &IndexBuffer;
	OutlineMesh.bWireframe = bWireframe;
	OutlineMesh.VertexFactory = &RenderBuffers.VertexFactory;
	OutlineMesh.MaterialRenderProxy = UseMaterial;

	OutlineMeshBatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

	OutlineMeshBatchElement.FirstIndex = 0;
	OutlineMeshBatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
	OutlineMeshBatchElement.MinVertexIndex = 0;
	OutlineMeshBatchElement.MaxVertexIndex = RenderBuffers.PositionVertexBuffer.GetNumVertices() - 1;
	
	OutlineMesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
	OutlineMesh.CastShadow=bCastShadow;
	OutlineMesh.Type = PT_TriangleList;
	OutlineMesh.DepthPriorityGroup = DepthPriority;
	OutlineMesh.bCanApplyViewModeOverrides = this->bEnableViewModeOverrides;

	switch (CullingMode)
	{
	case EMultiDrawCullingMode::FrontfaceCulling:
		OutlineMesh.ReverseCulling = !OutlineMesh.ReverseCulling;
		break;
	case  EMultiDrawCullingMode::BackfaceCulling:
		break;
	case  EMultiDrawCullingMode::DoubleSide:
		OutlineMesh.bDisableBackfaceCulling=true;
		break;
	}
	Collector.AddMesh(ViewIndex, OutlineMesh);
}