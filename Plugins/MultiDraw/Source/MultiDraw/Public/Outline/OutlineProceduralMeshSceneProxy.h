// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#pragma once
#include "DynamicMeshBuilder.h"
#include "StaticMeshResources.h"
#include "MultiDrawCommon.h"
#include "MultiDrawCommonDeclare.h"
#include "Outline/OutlineProceduralMeshComponent.h"
#include "PrimitiveSceneProxy.h"

/** Procedural mesh scene proxy */
class FOutlineProceduralMeshSceneProxy final : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FOutlineProceduralMeshSceneProxy(UOutlineProceduralMeshComponent* Component,EMultiDrawCullingMode CullingMode,bool bCastShadow)
		: FPrimitiveSceneProxy(Component)
		, BodySetup(Component->GetBodySetup())
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
		, Component(Component)
		, CullingMode(CullingMode)
		, bCastShadow(bCastShadow)
	{
		// Copy each section
		const int32 NumSections = Component->GetNumMaterials();
		TArray<FProcMeshSection> ProcMeshSections;
		ProcMeshSections.SetNum(NumSections, false);
		
		for(int32 i = 0; i < NumSections;i++)
		{
			FProcMeshSection* MeshSeectionPtr=Component->GetProcMeshSection(i);
			FProcMeshSection& Section = ProcMeshSections[i];
			Section.Reset();

			Section.ProcVertexBuffer = MeshSeectionPtr->ProcVertexBuffer;
			Section.ProcIndexBuffer = MeshSeectionPtr->ProcIndexBuffer;
			Section.SectionLocalBox = MeshSeectionPtr->SectionLocalBox;
			Section.bEnableCollision = MeshSeectionPtr->bEnableCollision;
			Section.bSectionVisible = MeshSeectionPtr->bSectionVisible;
		}
		
		
		Sections.AddZeroed(NumSections);
		for (int SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
		{
			FProcMeshSection& SrcSection = ProcMeshSections[SectionIdx];
			if (SrcSection.ProcIndexBuffer.Num() > 0 && SrcSection.ProcVertexBuffer.Num() > 0)
			{
				FProcMeshProxySection* NewSection = new FProcMeshProxySection(GetScene().GetFeatureLevel());

				// Copy data from vertex buffer
				const int32 NumVerts = SrcSection.ProcVertexBuffer.Num();

				// Allocate verts

				TArray<FDynamicMeshVertex> Vertices;
				Vertices.SetNumUninitialized(NumVerts);
				// Copy verts
				for (int VertIdx = 0; VertIdx < NumVerts; VertIdx++)
				{
					const FProcMeshVertex& ProcVert = SrcSection.ProcVertexBuffer[VertIdx];
					FDynamicMeshVertex& Vert = Vertices[VertIdx];
					ConvertProcMeshToDynMeshVertex(Vert, ProcVert);
				}

				// Copy index buffer
				NewSection->IndexBuffer.Indices = SrcSection.ProcIndexBuffer;

				NewSection->VertexBuffers.InitFromDynamicVertex(&NewSection->VertexFactory, Vertices, 4);

				// Enqueue initialization of render resource
				BeginInitResource(&NewSection->VertexBuffers.PositionVertexBuffer);
				BeginInitResource(&NewSection->VertexBuffers.StaticMeshVertexBuffer);
				BeginInitResource(&NewSection->VertexBuffers.ColorVertexBuffer);
				BeginInitResource(&NewSection->IndexBuffer);
				BeginInitResource(&NewSection->VertexFactory);

				// Grab material
				NewSection->Material = Component->GetMaterial(SectionIdx);
				if (NewSection->Material == NULL)
				{
					NewSection->Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}

				// Copy visibility info
				NewSection->bSectionVisible = SrcSection.bSectionVisible;

				// Save ref to new section
				Sections[SectionIdx] = NewSection;

#if RHI_RAYTRACING
				if (IsRayTracingEnabled())
				{
					ENQUEUE_RENDER_COMMAND(InitProceduralMeshRayTracingGeometry)(
						[this, DebugName = Component->GetFName(), NewSection](FRHICommandListImmediate& RHICmdList)
					{
						FRayTracingGeometryInitializer Initializer;
						Initializer.DebugName = DebugName;
						Initializer.IndexBuffer = nullptr;
						Initializer.TotalPrimitiveCount = 0;
						Initializer.GeometryType = RTGT_Triangles;
						Initializer.bFastBuild = true;
						Initializer.bAllowUpdate = false;

						NewSection->RayTracingGeometry.SetInitializer(Initializer);
						NewSection->RayTracingGeometry.InitResource();

						NewSection->RayTracingGeometry.Initializer.IndexBuffer = NewSection->IndexBuffer.IndexBufferRHI;
						NewSection->RayTracingGeometry.Initializer.TotalPrimitiveCount = NewSection->IndexBuffer.Indices.Num() / 3;

						FRayTracingGeometrySegment Segment;
						Segment.VertexBuffer = NewSection->VertexBuffers.PositionVertexBuffer.VertexBufferRHI;
						Segment.NumPrimitives = NewSection->RayTracingGeometry.Initializer.TotalPrimitiveCount;
						Segment.MaxVertices = NewSection->VertexBuffers.PositionVertexBuffer.GetNumVertices();
						NewSection->RayTracingGeometry.Initializer.Segments.Add(Segment);

						//#dxr_todo: add support for segments?
						
						NewSection->RayTracingGeometry.UpdateRHI();
					});
				}
#endif
			}
		}
	}

	virtual ~FOutlineProceduralMeshSceneProxy()
	{
		for (FProcMeshProxySection* Section : Sections)
		{
			if (Section != nullptr)
			{
				Section->VertexBuffers.PositionVertexBuffer.ReleaseResource();
				Section->VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
				Section->VertexBuffers.ColorVertexBuffer.ReleaseResource();
				Section->IndexBuffer.ReleaseResource();
				Section->VertexFactory.ReleaseResource();

#if RHI_RAYTRACING
				if (IsRayTracingEnabled())
				{
					Section->RayTracingGeometry.ReleaseResource();
				}
#endif

				delete Section;
			}
		}
	}

	/** Called on render thread to assign new dynamic data */
	void UpdateSection_RenderThread(FProcMeshSectionUpdateData* SectionData)
	{
		//SCOPE_CYCLE_COUNTER(STAT_ProcMesh_UpdateSectionRT);

		check(IsInRenderingThread());

		// Check we have data 
		if(	SectionData != nullptr) 			
		{
			// Check it references a valid section
			if (SectionData->TargetSection < Sections.Num() &&
				Sections[SectionData->TargetSection] != nullptr)
			{
				FProcMeshProxySection* Section = Sections[SectionData->TargetSection];

				// Lock vertex buffer
				const int32 NumVerts = SectionData->NewVertexBuffer.Num();
			
				// Iterate through vertex data, copying in new info
				for(int32 i=0; i<NumVerts; i++)
				{
					const FProcMeshVertex& ProcVert = SectionData->NewVertexBuffer[i];
					FDynamicMeshVertex Vertex;
					ConvertProcMeshToDynMeshVertex(Vertex, ProcVert);

					Section->VertexBuffers.PositionVertexBuffer.VertexPosition(i) = Vertex.Position;
					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, Vertex.TangentX.ToFVector3f(), Vertex.GetTangentY(), Vertex.TangentZ.ToFVector3f());
					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 0, Vertex.TextureCoordinate[0]);
					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 1, Vertex.TextureCoordinate[1]);
					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 2, Vertex.TextureCoordinate[2]);
					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 3, Vertex.TextureCoordinate[3]);
					Section->VertexBuffers.ColorVertexBuffer.VertexColor(i) = Vertex.Color;
				}

				{
					auto& VertexBuffer = Section->VertexBuffers.PositionVertexBuffer;
					void* VertexBufferData = RHILockBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
					RHIUnlockBuffer(VertexBuffer.VertexBufferRHI);
				}

				{
					auto& VertexBuffer = Section->VertexBuffers.ColorVertexBuffer;
					void* VertexBufferData = RHILockBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
					RHIUnlockBuffer(VertexBuffer.VertexBufferRHI);
				}

				{
					auto& VertexBuffer = Section->VertexBuffers.StaticMeshVertexBuffer;
					void* VertexBufferData = RHILockBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTangentSize(), RLM_WriteOnly);
					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTangentData(), VertexBuffer.GetTangentSize());
					RHIUnlockBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI);
				}

				{
					auto& VertexBuffer = Section->VertexBuffers.StaticMeshVertexBuffer;
					void* VertexBufferData = RHILockBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTexCoordSize(), RLM_WriteOnly);
					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTexCoordData(), VertexBuffer.GetTexCoordSize());
					RHIUnlockBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI);
				}

#if RHI_RAYTRACING
				if (IsRayTracingEnabled())
				{
					Section->RayTracingGeometry.ReleaseResource();

					FRayTracingGeometryInitializer Initializer;
					Initializer.IndexBuffer = Section->IndexBuffer.IndexBufferRHI;
					Initializer.TotalPrimitiveCount = Section->IndexBuffer.Indices.Num() / 3;
					Initializer.GeometryType = RTGT_Triangles;
					Initializer.bFastBuild = true;
					Initializer.bAllowUpdate = false;

					Section->RayTracingGeometry.SetInitializer(Initializer);
					Section->RayTracingGeometry.InitResource();

					FRayTracingGeometrySegment Segment;
					Segment.VertexBuffer = Section->VertexBuffers.PositionVertexBuffer.VertexBufferRHI;
					Segment.NumPrimitives = Section->RayTracingGeometry.Initializer.TotalPrimitiveCount;
					Segment.MaxVertices = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices();
					Section->RayTracingGeometry.Initializer.Segments.Add(Segment);

					Section->RayTracingGeometry.UpdateRHI();
				}
#endif
			}

			// Free data sent from game thread
			delete SectionData;
		}
	}

	void SetSectionVisibility_RenderThread(int32 SectionIndex, bool bNewVisibility)
	{
		check(IsInRenderingThread());

		if(	SectionIndex < Sections.Num() &&
			Sections[SectionIndex] != nullptr)
		{
			Sections[SectionIndex]->bSectionVisible = bNewVisibility;
		}
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		SCOPE_CYCLE_COUNTER(STAT_ProcMesh_GetMeshElements);


		// Set up wireframe material (if needed)
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		FColoredMaterialRenderProxy* WireframeMaterialInstance = NULL;
		if (bWireframe)
		{
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : NULL,
				FLinearColor(0, 0.5f, 1.f)
				);

			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}

		// Iterate over sections
		for (const FProcMeshProxySection* Section : Sections)
		{
			if (Section != nullptr && Section->bSectionVisible)
			{
				FMaterialRenderProxy* MaterialProxy = bWireframe ? WireframeMaterialInstance : Section->Material->GetRenderProxy();

				// For each view..
				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
				{
					if (VisibilityMap & (1 << ViewIndex))
					{
						const FSceneView* View = Views[ViewIndex];
						// Draw the mesh.
						FMeshBatch& Mesh = Collector.AllocateMesh();
						FMeshBatchElement& BatchElement = Mesh.Elements[0];
						BatchElement.IndexBuffer = &Section->IndexBuffer;
						Mesh.bWireframe = bWireframe;
						Mesh.VertexFactory = &Section->VertexFactory;
						Mesh.MaterialRenderProxy = MaterialProxy;

						bool bHasPrecomputedVolumetricLightmap;
						FMatrix PreviousLocalToWorld;
						int32 SingleCaptureIndex;
						bool bOutputVelocity;
						GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);
						bOutputVelocity |= AlwaysHasVelocity();

						FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
						DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, bOutputVelocity, GetCustomPrimitiveData());
						BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

						BatchElement.FirstIndex = 0;
						BatchElement.NumPrimitives = Section->IndexBuffer.Indices.Num() / 3;
						BatchElement.MinVertexIndex = 0;
						BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
						Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
						Mesh.Type = PT_TriangleList;
						Mesh.DepthPriorityGroup = SDPG_World;
						Mesh.bCanApplyViewModeOverrides = false;
						Collector.AddMesh(ViewIndex, Mesh);

						//Outline
						FMeshBatch& OutlineMesh = Collector.AllocateMesh();
						FMeshBatchElement& OutlineMeshBatchElement = OutlineMesh.Elements[0];
						OutlineMeshBatchElement.IndexBuffer = &Section->IndexBuffer;
						OutlineMesh.bWireframe = bWireframe;
						OutlineMesh.VertexFactory = &Section->VertexFactory;
						OutlineMesh.MaterialRenderProxy = MaterialProxy;
						
						GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);
						bOutputVelocity |= AlwaysHasVelocity();

						FDynamicPrimitiveUniformBuffer& OutlineMeshDynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
						OutlineMeshDynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, bOutputVelocity, GetCustomPrimitiveData());
						OutlineMeshBatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

						OutlineMeshBatchElement.FirstIndex = 0;
						OutlineMeshBatchElement.NumPrimitives = Section->IndexBuffer.Indices.Num() / 3;
						OutlineMeshBatchElement.MinVertexIndex = 0;
						OutlineMeshBatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
						OutlineMesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
						OutlineMesh.Type = PT_TriangleList;
						OutlineMesh.DepthPriorityGroup = SDPG_World;
						OutlineMesh.bCanApplyViewModeOverrides = false;
						
						auto OutlinePassMaterialSet=Component->OutlinePassMaterialSet;
						const int32 MaterialIndex = Sections.IndexOfByKey(Section);
						if(MaterialIndex > OutlinePassMaterialSet.Num()-1)
							continue;
						
						UMaterialInterface* CurrentOutlineMaterial=OutlinePassMaterialSet[MaterialIndex];
						if (CurrentOutlineMaterial == nullptr)
							continue;
						
						OutlineMesh.MaterialRenderProxy = CurrentOutlineMaterial->GetRenderProxy();
						OutlineMesh.CastShadow=bCastShadow;
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
				}
			}
		}

		// Draw bounds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{	 
				// Draw simple collision as wireframe if 'show collision', and collision is enabled, and we are not using the complex as the simple
				if (ViewFamily.EngineShowFlags.Collision && IsCollisionEnabled() && BodySetup->GetCollisionTraceFlag() != ECollisionTraceFlag::CTF_UseComplexAsSimple)
				{
					FTransform GeomTransform(GetLocalToWorld());
					BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(FColor(157, 149, 223, 255), IsSelected(), IsHovered()).ToFColor(true), NULL, false, false, AlwaysHasVelocity(), ViewIndex, Collector);
				}

				// Render bounds
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
			}
		}
#endif
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
		return Result;
	}

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint(void) const
	{
		return(sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const
	{
		return(FPrimitiveSceneProxy::GetAllocatedSize());
	}


#if RHI_RAYTRACING
	virtual bool IsRayTracingRelevant() const override { return true; }

	virtual bool HasRayTracingRepresentation() const override { return true; }

	virtual void GetDynamicRayTracingInstances(FRayTracingMaterialGatheringContext& Context, TArray<FRayTracingInstance>& OutRayTracingInstances) override final
	{
		return;
		/*
		for (int32 SegmentIndex = 0; SegmentIndex < Sections.Num(); ++SegmentIndex)
		{
			const FProcMeshProxySection* Section = Sections[SegmentIndex];
			if (Section != nullptr && Section->bSectionVisible)
			{
				FMaterialRenderProxy* MaterialProxy = Section->Material->GetRenderProxy();
				
				if (Section->RayTracingGeometry.RayTracingGeometryRHI.IsValid())
				{
					check(Section->RayTracingGeometry.Initializer.IndexBuffer.IsValid());

					FRayTracingInstance RayTracingInstance;
					RayTracingInstance.Geometry = &Section->RayTracingGeometry;
					RayTracingInstance.InstanceTransforms.Add(GetLocalToWorld());

					uint32 SectionIdx = 0;
					FMeshBatch MeshBatch;

					MeshBatch.VertexFactory = &Section->VertexFactory;
					MeshBatch.SegmentIndex = 0;
					MeshBatch.MaterialRenderProxy = Section->Material->GetRenderProxy();
					MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
					MeshBatch.Type = PT_TriangleList;
					MeshBatch.DepthPriorityGroup = SDPG_World;
					MeshBatch.bCanApplyViewModeOverrides = false;
					MeshBatch.CastRayTracedShadow = IsShadowCast(Context.ReferenceView);

					FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
					BatchElement.IndexBuffer = &Section->IndexBuffer;

					bool bHasPrecomputedVolumetricLightmap;
					FMatrix PreviousLocalToWorld;
					int32 SingleCaptureIndex;
					bool bOutputVelocity;
					GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);
					bOutputVelocity |= AlwaysHasVelocity();

					FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Context.RayTracingMeshResourceCollector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
					DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, bOutputVelocity, GetCustomPrimitiveData());
					BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

					BatchElement.FirstIndex = 0;
					BatchElement.NumPrimitives = Section->IndexBuffer.Indices.Num() / 3;
					BatchElement.MinVertexIndex = 0;
					BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;

					RayTracingInstance.Materials.Add(MeshBatch);

					RayTracingInstance.BuildInstanceMaskAndFlags(GetScene().GetFeatureLevel());
					OutRayTracingInstances.Add(RayTracingInstance);
				}
			}
		}*/
	}
	
#endif

private:
	/** Array of sections */
	TArray<FProcMeshProxySection*> Sections;

	UBodySetup* BodySetup;

	FMaterialRelevance MaterialRelevance;

	UOutlineProceduralMeshComponent* Component;

	EMultiDrawCullingMode CullingMode;
	bool bCastShadow;
};