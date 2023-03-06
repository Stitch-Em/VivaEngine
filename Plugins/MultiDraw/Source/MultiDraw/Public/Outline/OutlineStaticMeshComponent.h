// Copyright 2022 BlueRose, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "MultiDrawCommon.h"
#include "Engine/Classes/Components/StaticMeshComponent.h"
#include "OutlineStaticMeshComponent.generated.h"

UCLASS(ClassGroup=(Rendering, Common), hidecategories=Object,  editinlinenew, meta=(BlueprintSpawnableComponent))
class UOutlineStaticMeshComponent : public UStaticMeshComponent
{
	GENERATED_UCLASS_BODY()

	//~ Begin UPrimitiveComponent Interface
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

	/** Accesses the scene relevance information for the materials applied to the mesh. Valid from game thread only. */
	virtual FMaterialRelevance GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const override;
	
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface
public:
	// Change outline material by index,return false if index error or MaterialInstance is not valid.
	UFUNCTION(BlueprintCallable, Category = "MultiDraw")
	bool ChangeMaterialByIndex(UMaterialInterface* InMaterialInstance,int32 Index);
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OutlinePass")
	EMultiDrawCullingMode CullingMode;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OutlinePass")
	bool bCastShadow;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OutlinePass")
	TArray<UMaterialInterface*> OutlinePassMaterialSet;
};