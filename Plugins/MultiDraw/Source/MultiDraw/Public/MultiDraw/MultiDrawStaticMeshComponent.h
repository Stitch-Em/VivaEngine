// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Engine/Classes/Components/StaticMeshComponent.h"
#include "MultiDrawCommon.h"
#include "MultiDrawStaticMeshComponent.generated.h"

UCLASS(ClassGroup = (Rendering, Common), hidecategories = Object, editinlinenew, meta = (BlueprintSpawnableComponent))
class UMultiDrawStaticMeshComponent : public UStaticMeshComponent
{
	GENERATED_UCLASS_BODY()

	//~ Begin UPrimitiveComponent Interface
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

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