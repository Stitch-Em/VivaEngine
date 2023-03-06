// Copyright 2022 BlueRose, Inc. All Rights Reserved.

#pragma once

#include "Engine/Private/SkeletalRenderGPUSkin.h"
#include "GPUSkinCache.h"
#include "MultiDrawCommon.generated.h"

UENUM()
enum class EMultiDrawCullingMode : uint8
{
	FrontfaceCulling,
	BackfaceCulling,
	DoubleSide
};

USTRUCT(BlueprintType)
struct FMultiDrawSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDraw")
	class UMaterialInterface* MaterialInterface = nullptr;

	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDraw")
	// uint8 ElementIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDraw")
	bool bDraw = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDraw")
	bool bCastShadow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDraw")
	EMultiDrawCullingMode CullingMode=EMultiDrawCullingMode::BackfaceCulling;
};

namespace MultiDraw
{
	static bool GUseShadowIndexBuffer = true;
	static int32 GForceRecomputeTangents = 0;
	static int32 GSkinCacheRecomputeTangents = 2;
	static int32 GBlendUsingVertexColorForRecomputeTangents = 0;

	inline void SetupMeshBatchForRuntimeVirtualTexture(FMeshBatch& MeshBatch)
	{
		MeshBatch.CastShadow = 0;
		MeshBatch.bUseAsOccluder = 0;
		MeshBatch.bUseForDepthPass = 0;
		MeshBatch.bUseForMaterial = 0;
		MeshBatch.bDitheredLODTransition = 0;
		MeshBatch.bRenderToVirtualTexture = 1;
	}
}

class FSkeletalMeshSectionIter
{
public:
	FSkeletalMeshSectionIter(const int32 InLODIdx, const FSkeletalMeshObject& InMeshObject, const FSkeletalMeshLODRenderData& InLODData, const FSkeletalMeshSceneProxy::FLODSectionElements& InLODSectionElements)
		: SectionIndex(0)
		, MeshObject(InMeshObject)
		, LODSectionElements(InLODSectionElements)
		, Sections(InLODData.RenderSections)
#if WITH_EDITORONLY_DATA
		, SectionIndexPreview(InMeshObject.SectionIndexPreview)
		, MaterialIndexPreview(InMeshObject.MaterialIndexPreview)
#endif
	{
		while (NotValidPreviewSection())
		{
			SectionIndex++;
		}
	}
	FORCEINLINE FSkeletalMeshSectionIter& operator++()
	{
		do 
		{
		SectionIndex++;
		} while (NotValidPreviewSection());
		return *this;
	}
	FORCEINLINE explicit operator bool() const
	{
		return ((SectionIndex < Sections.Num()) && LODSectionElements.SectionElements.IsValidIndex(GetSectionElementIndex()));
	}
	FORCEINLINE const FSkelMeshRenderSection& GetSection() const
	{
		return Sections[SectionIndex];
	}
	FORCEINLINE const int32 GetSectionElementIndex() const
	{
		return SectionIndex;
	}
	FORCEINLINE const FSkeletalMeshSceneProxy::FSectionElementInfo& GetSectionElementInfo() const
	{
		int32 SectionElementInfoIndex = GetSectionElementIndex();
		return LODSectionElements.SectionElements[SectionElementInfoIndex];
	}
	FORCEINLINE bool NotValidPreviewSection()
	{
#if WITH_EDITORONLY_DATA
		if (MaterialIndexPreview == INDEX_NONE)
		{
			int32 ActualPreviewSectionIdx = SectionIndexPreview;

			return	(SectionIndex < Sections.Num()) &&
				((ActualPreviewSectionIdx >= 0) && (ActualPreviewSectionIdx != SectionIndex));
		}
		else
		{
			int32 ActualPreviewMaterialIdx = MaterialIndexPreview;
			int32 ActualPreviewSectionIdx = INDEX_NONE;
			if (ActualPreviewMaterialIdx != INDEX_NONE && Sections.IsValidIndex(SectionIndex))
			{
				const FSkeletalMeshSceneProxy::FSectionElementInfo& SectionInfo = LODSectionElements.SectionElements[SectionIndex];
				if (SectionInfo.UseMaterialIndex == ActualPreviewMaterialIdx)
				{
					ActualPreviewSectionIdx = SectionIndex;
				}
			}

			return	(SectionIndex < Sections.Num()) &&
				((ActualPreviewMaterialIdx >= 0) && (ActualPreviewSectionIdx != SectionIndex));
		}
#else
		return false;
#endif
	}
private:
	int32 SectionIndex;
	const FSkeletalMeshObject& MeshObject;
	const FSkeletalMeshSceneProxy::FLODSectionElements& LODSectionElements;
	const TArray<FSkelMeshRenderSection>& Sections;
#if WITH_EDITORONLY_DATA
	const int32 SectionIndexPreview;
	const int32 MaterialIndexPreview;
#endif
};

class FGPUSkinCacheEntry
{
public:
	FGPUSkinCacheEntry(FGPUSkinCache* InSkinCache, FSkeletalMeshObjectGPUSkin* InGPUSkin, FGPUSkinCache::FRWBuffersAllocation* InPositionAllocation, int32 InLOD, EGPUSkinCacheEntryMode InMode)
		: Mode(InMode)
		, PositionAllocation(InPositionAllocation)
		, SkinCache(InSkinCache)
		, GPUSkin(InGPUSkin)
		, MorphBuffer(0)
		, LOD(InLOD)
	{
		
		const TArray<FSkelMeshRenderSection>& Sections = InGPUSkin->GetRenderSections(LOD);
		DispatchData.AddDefaulted(Sections.Num());
		BatchElementsUserData.AddZeroed(Sections.Num());
		for (int32 Index = 0; Index < Sections.Num(); ++Index)
		{
			BatchElementsUserData[Index].Entry = this;
			BatchElementsUserData[Index].Section = Index;
		}

		UpdateSkinWeightBuffer();
	}

	~FGPUSkinCacheEntry()
	{
		check(!PositionAllocation);
	}

	struct FSectionDispatchData
	{
		FGPUSkinCache::FRWBufferTracker PositionTracker;

		FGPUBaseSkinVertexFactory* SourceVertexFactory = nullptr;
		FGPUSkinPassthroughVertexFactory* TargetVertexFactory = nullptr;

		// triangle index buffer (input for the RecomputeSkinTangents, might need special index buffer unique to position and normal, not considering UV/vertex color)
		FRHIShaderResourceView* IndexBuffer = nullptr;

		const FSkelMeshRenderSection* Section = nullptr;

		// for debugging / draw events, -1 if not set
		uint32 SectionIndex = -1;

		// 0:normal, 1:with morph target, 2:with APEX cloth (not yet implemented)
		uint16 SkinType = 0;

		// See EGPUSkinCacheDispatchFlags
		uint16 DispatchFlags = 0;

		//
		uint32 NumBoneInfluences = 0;

		// in floats (4 bytes)
		uint32 OutputStreamStart = 0;
		uint32 NumVertices = 0;

		// in vertices
		uint32 InputStreamStart = 0;
		uint32 NumTexCoords = 1;
		uint32 SelectedTexCoord = 0;

		FShaderResourceViewRHIRef TangentBufferSRV = nullptr;
		FShaderResourceViewRHIRef UVsBufferSRV = nullptr;
		FShaderResourceViewRHIRef ColorBufferSRV = nullptr;
		FShaderResourceViewRHIRef PositionBufferSRV = nullptr;
		FShaderResourceViewRHIRef ClothPositionsAndNormalsBuffer = nullptr;

		// skin weight input
		uint32 InputWeightStart = 0;

		// morph input
		uint32 MorphBufferOffset = 0;

        // cloth input
		uint32 ClothBufferOffset = 0;
        float ClothBlendWeight = 0.0f;

        FMatrix44f ClothToLocal = FMatrix44f::Identity;

		// triangle index buffer (input for the RecomputeSkinTangents, might need special index buffer unique to position and normal, not considering UV/vertex color)
		uint32 IndexBufferOffsetValue = 0;
		uint32 NumTriangles = 0;

		FGPUSkinCache::FSkinCacheRWBuffer* TangentBuffer = nullptr;
		FGPUSkinCache::FSkinCacheRWBuffer* IntermediateTangentBuffer = nullptr;
		FGPUSkinCache::FSkinCacheRWBuffer* IntermediateAccumulatedTangentBuffer = nullptr;
		uint32 IntermediateAccumulatedTangentBufferOffset = 0;
		FGPUSkinCache::FSkinCacheRWBuffer* PositionBuffer = nullptr;
		FGPUSkinCache::FSkinCacheRWBuffer* PreviousPositionBuffer = nullptr;

        // Handle duplicates
        FShaderResourceViewRHIRef DuplicatedIndicesIndices = nullptr;
        FShaderResourceViewRHIRef DuplicatedIndices = nullptr;

		FSectionDispatchData() = default;

		inline FGPUSkinCache::FSkinCacheRWBuffer* GetPreviousPositionRWBuffer() const
		{
			check(PreviousPositionBuffer);
			return PreviousPositionBuffer;
		}

		inline FGPUSkinCache::FSkinCacheRWBuffer* GetPositionRWBuffer() const
		{
			check(PositionBuffer);
			return PositionBuffer;
		}

		inline FGPUSkinCache::FSkinCacheRWBuffer* GetTangentRWBuffer() const
		{
			return TangentBuffer;
		}

		FGPUSkinCache::FSkinCacheRWBuffer* GetActiveTangentRWBuffer() const
		{
			// This is the buffer containing tangent results from the skinning CS pass
			return (IndexBuffer && IntermediateTangentBuffer) ? IntermediateTangentBuffer : TangentBuffer;
		}

		inline FGPUSkinCache::FSkinCacheRWBuffer* GetIntermediateAccumulatedTangentBuffer() const
		{
			check(IntermediateAccumulatedTangentBuffer);
			return IntermediateAccumulatedTangentBuffer;
		}

		void UpdateVertexFactoryDeclaration()
		{
			TargetVertexFactory->UpdateVertexDeclaration(SourceVertexFactory, &GetPositionRWBuffer()->Buffer, &GetTangentRWBuffer()->Buffer);
		}
	};

	void UpdateVertexFactoryDeclaration(int32 Section)
	{
		DispatchData[Section].UpdateVertexFactoryDeclaration();
	}

	inline FCachedGeometry::Section GetCachedGeometry(int32 SectionIndex) const
	{
		FCachedGeometry::Section MeshSection;
		if (SectionIndex >= 0 && SectionIndex < DispatchData.Num())
		{
			const FSkelMeshRenderSection& Section = *DispatchData[SectionIndex].Section;
			MeshSection.PositionBuffer = DispatchData[SectionIndex].PositionBuffer->Buffer.SRV;
			MeshSection.UVsBuffer = DispatchData[SectionIndex].UVsBufferSRV;
			MeshSection.TotalVertexCount = DispatchData[SectionIndex].PositionBuffer->Buffer.NumBytes / (sizeof(float) * 3);
			MeshSection.NumPrimitives = Section.NumTriangles;
			MeshSection.NumVertices = Section.NumVertices;
			MeshSection.IndexBaseIndex = Section.BaseIndex;
			MeshSection.VertexBaseIndex = Section.BaseVertexIndex;
			MeshSection.IndexBuffer = nullptr;
			MeshSection.TotalIndexCount = 0;
			MeshSection.LODIndex = 0;
			MeshSection.SectionIndex = SectionIndex;
		}
		return MeshSection;
	}

	bool IsSectionValid(int32 Section) const
	{
		const FSectionDispatchData& SectionData = DispatchData[Section];
		return SectionData.SectionIndex == Section;
	}

	bool IsSourceFactoryValid(int32 Section, FGPUBaseSkinVertexFactory* SourceVertexFactory) const
	{
		const FSectionDispatchData& SectionData = DispatchData[Section];
		return SectionData.SourceVertexFactory == SourceVertexFactory;
	}

	bool IsValid(FSkeletalMeshObjectGPUSkin* InSkin, int32 InLOD) const
	{
		return GPUSkin == InSkin && LOD == InLOD;
	}

	void UpdateSkinWeightBuffer()
	{
		FSkinWeightVertexBuffer* WeightBuffer = GPUSkin->GetSkinWeightVertexBuffer(LOD);
		bUse16BitBoneIndex = WeightBuffer->Use16BitBoneIndex();
		InputWeightIndexSize = WeightBuffer->GetBoneIndexByteSize();
		InputWeightStride = WeightBuffer->GetConstantInfluencesVertexStride();
		InputWeightStreamSRV = WeightBuffer->GetDataVertexBuffer()->GetSRV();
		InputWeightLookupStreamSRV = WeightBuffer->GetLookupVertexBuffer()->GetSRV();
				
		if (WeightBuffer->GetBoneInfluenceType() == GPUSkinBoneInfluenceType::DefaultBoneInfluence)
		{
			int32 MaxBoneInfluences = WeightBuffer->GetMaxBoneInfluences();
			BoneInfluenceType = MaxBoneInfluences > MAX_INFLUENCES_PER_STREAM ? 1 : 0;
		}
		else
		{
			BoneInfluenceType = 2;
		}
	}

	void SetupSection(
		int32 SectionIndex,
		FGPUSkinCache::FRWBuffersAllocation* InPositionAllocation,
		FSkelMeshRenderSection* Section,
		const FMorphVertexBuffer* MorphVertexBuffer,
		const FSkeletalMeshVertexClothBuffer* ClothVertexBuffer,
		uint32 NumVertices,
		uint32 InputStreamStart,
		FGPUBaseSkinVertexFactory* InSourceVertexFactory,
		FGPUSkinPassthroughVertexFactory* InTargetVertexFactory,
		uint32 InIntermediateAccumulatedTangentBufferOffset,
		const FClothSimulData* SimData)
	{
		//UE_LOG(LogSkinCache, Warning, TEXT("*** SetupSection E %p Alloc %p Sec %d(%p) LOD %d"), this, InAllocation, SectionIndex, Section, LOD);
		FSectionDispatchData& Data = DispatchData[SectionIndex];
		check(!Data.PositionTracker.Allocation || Data.PositionTracker.Allocation == InPositionAllocation);

		Data.PositionTracker.Allocation = InPositionAllocation;

		Data.SectionIndex = SectionIndex;
		Data.Section = Section;

		FSkeletalMeshRenderData& SkelMeshRenderData = GPUSkin->GetSkeletalMeshRenderData();
		FSkeletalMeshLODRenderData& LodData = SkelMeshRenderData.LODRenderData[LOD];
		check(Data.SectionIndex == LodData.FindSectionIndex(*Section));

		Data.NumVertices = NumVertices;
		const bool bMorph = MorphVertexBuffer && MorphVertexBuffer->SectionIds.Contains(SectionIndex);
		if (bMorph)
		{
			// in bytes
			const uint32 MorphStride = sizeof(FMorphGPUSkinVertex);

			// see GPU code "check(MorphStride == sizeof(float) * 6);"
			check(MorphStride == sizeof(float) * 6);

			Data.MorphBufferOffset = Section->BaseVertexIndex;
		}

		if (ClothVertexBuffer && ClothVertexBuffer->GetClothIndexMapping().Num() > SectionIndex)
		{
			const FClothBufferIndexMapping& ClothBufferIndexMapping = ClothVertexBuffer->GetClothIndexMapping()[SectionIndex];

			check(SimData->LODIndex != INDEX_NONE && SimData->LODIndex <= LOD);
			const uint32 ClothLODBias = (uint32)(LOD - SimData->LODIndex);

			const uint32 ClothBufferOffset = ClothBufferIndexMapping.MappingOffset + ClothBufferIndexMapping.LODBiasStride * ClothLODBias;

			// Set the buffer offset depending on whether enough deformer mapping data exists (RaytracingMinLOD/RaytracingLODBias/ClothLODBiasMode settings)
			const uint32 NumInfluences = NumVertices ? ClothBufferIndexMapping.LODBiasStride / NumVertices : 1;
			Data.ClothBufferOffset = (ClothBufferOffset + NumVertices * NumInfluences <= ClothVertexBuffer->GetNumVertices()) ?
				ClothBufferOffset :                     // If the offset is valid, set the calculated LODBias offset
				ClothBufferIndexMapping.MappingOffset;  // Otherwise fallback to a 0 ClothLODBias to prevent from reading pass the buffer (but still raytrace broken shadows/reflections/etc.)
		}

		//INC_DWORD_STAT(STAT_GPUSkinCache_TotalNumChunks);

		// SkinType 0:normal, 1:with morph target, 2:with cloth
		Data.SkinType = ClothVertexBuffer ? 2 : (bMorph ? 1 : 0);
		Data.InputStreamStart = InputStreamStart;
		Data.OutputStreamStart = Section->BaseVertexIndex;

		Data.TangentBufferSRV = InSourceVertexFactory->GetTangentsSRV();
		Data.UVsBufferSRV = InSourceVertexFactory->GetTextureCoordinatesSRV();
		Data.ColorBufferSRV = InSourceVertexFactory->GetColorComponentsSRV();
		Data.NumTexCoords = InSourceVertexFactory->GetNumTexCoords();
		Data.PositionBufferSRV = InSourceVertexFactory->GetPositionsSRV();

		Data.NumBoneInfluences = InSourceVertexFactory->GetNumBoneInfluences();
		check(Data.TangentBufferSRV && Data.PositionBufferSRV);

		// weight buffer
		Data.InputWeightStart = (InputWeightStride * Section->BaseVertexIndex) / sizeof(float);
		Data.SourceVertexFactory = InSourceVertexFactory;
		Data.TargetVertexFactory = InTargetVertexFactory;

		InTargetVertexFactory->InvalidateStreams();

		int32 RecomputeTangentsMode = GSkinCacheRecomputeTangents;
		if (RecomputeTangentsMode > 0)
		{
			if (Section->bRecomputeTangent || RecomputeTangentsMode == 1)
			{
				FRawStaticIndexBuffer16or32Interface* IndexBuffer = LodData.MultiSizeIndexContainer.GetIndexBuffer();
				Data.IndexBuffer = IndexBuffer->GetSRV();
				if (Data.IndexBuffer)
				{
					Data.NumTriangles = Section->NumTriangles;
					Data.IndexBufferOffsetValue = Section->BaseIndex;
					Data.IntermediateAccumulatedTangentBufferOffset = InIntermediateAccumulatedTangentBufferOffset;
				}
			}
		}
	}

#if RHI_RAYTRACING
	void GetRayTracingSegmentVertexBuffers(TArray<FBufferRHIRef>& OutVertexBuffers) const
	{
		OutVertexBuffers.SetNum(DispatchData.Num());
		for (int32 SectionIdx = 0; SectionIdx < DispatchData.Num(); SectionIdx++)
		{
			OutVertexBuffers[SectionIdx] = DispatchData[SectionIdx].PositionBuffer->Buffer.Buffer;
		}
	}
#endif // RHI_RAYTRACING

	TArray<FSectionDispatchData>& GetDispatchData() { return DispatchData; }

public:
	EGPUSkinCacheEntryMode Mode;
	FGPUSkinCache::FRWBuffersAllocation* PositionAllocation;
	FGPUSkinCache* SkinCache;
	TArray<FGPUSkinBatchElementUserData> BatchElementsUserData;
	TArray<FSectionDispatchData> DispatchData;
	FSkeletalMeshObjectGPUSkin* GPUSkin;
	int BoneInfluenceType;
	bool bUse16BitBoneIndex;
	uint32 InputWeightIndexSize;
	uint32 InputWeightStride;
	FShaderResourceViewRHIRef InputWeightStreamSRV;
	FShaderResourceViewRHIRef InputWeightLookupStreamSRV;
	FRHIShaderResourceView* MorphBuffer;
	FShaderResourceViewRHIRef ClothBuffer;
	int32 LOD;

	friend class FGPUSkinCache;
	friend class FBaseGPUSkinCacheCS;
	friend class FBaseRecomputeTangentsPerTriangleShader;
};

class FOutlineSkeletalMeshObjectGPUSkin : public FSkeletalMeshObjectGPUSkin
{
public:
	FOutlineSkeletalMeshObjectGPUSkin(USkinnedMeshComponent* InMeshComponent, FSkeletalMeshRenderData* InSkelMeshRenderData, ERHIFeatureLevel::Type InFeatureLevel):FSkeletalMeshObjectGPUSkin(InMeshComponent,InSkelMeshRenderData,InFeatureLevel){}
	
	FGPUSkinCacheEntry* GetGPUSkinCacheEntry(){return SkinCacheEntry;}
};

class FOutlineGPUSkinPassthroughVertexFactory : public FGPUSkinPassthroughVertexFactory
{
public:
	FOutlineGPUSkinPassthroughVertexFactory(ERHIFeatureLevel::Type InFeatureLevel):FGPUSkinPassthroughVertexFactory(InFeatureLevel)
	{}

	TRefCountPtr<FRDGPooledBuffer> GetPrevPositionRDG()
	{
		return PrevPositionRDG;
	}

	TUniformBufferRef<FLocalVertexFactoryLooseParameters> GetLooseParametersUniformBuffer()
	{
		return LooseParametersUniformBuffer;	
	}
};