// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "MultiDrawCommon.h"
#include "Components/DynamicMeshComponent.h"
#include "MultiDrawDynamicMeshComponent.generated.h"

UCLASS(ClassGroup=(Rendering, Common), hidecategories=Object,  editinlinenew, meta=(BlueprintSpawnableComponent))
class UMultiDrawDynamicMeshComponent : public UDynamicMeshComponent
{
	GENERATED_UCLASS_BODY()

	//~ Begin UPrimitiveComponent Interface
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

	/** Accesses the scene relevance information for the materials applied to the mesh. Valid from game thread only. */
	virtual FMaterialRelevance GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const override;
	
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface
public:
	UFUNCTION(BlueprintCallable, Category = "MultiDraw")
	bool ChangeMaterialByIndex(UMaterialInterface* InMaterialInstance,int32 Index);

	UFUNCTION(BlueprintCallable, Category = "MultiDraw")
	bool ChangeMultiDrawSettingsByIndex(FMultiDrawSettings InSettings,int32 Index);
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDraw")
	TArray<FMultiDrawSettings> PerPassDrawSettingses;
};