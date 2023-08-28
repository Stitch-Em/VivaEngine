// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MultiDrawCommon.h"
#include "Components/SkeletalMeshComponent.h"
#include "MultiDrawSkeletalMeshComponent.generated.h"

UCLASS(ClassGroup = (Rendering, Common), hidecategories = Object, editinlinenew, meta = (BlueprintSpawnableComponent))
class UMultiDrawSkeletalMeshComponent : public USkeletalMeshComponent
{
	GENERATED_UCLASS_BODY()

	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

	virtual FMaterialRelevance GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const override;
	
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
public:
	UFUNCTION(BlueprintCallable, Category = "MultiDraw")
	bool ChangeMaterialByIndex(UMaterialInterface* InMaterialInstance,int32 Index);

	UFUNCTION(BlueprintCallable, Category = "MultiDraw")
	bool ChangeMultiDrawSettingsByIndex(FMultiDrawSettings InSettings,int32 Index);
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDraw")
	// TArray<FMultiDrawSettings> MultiDrawMaterialSet;
	TArray<FMultiDrawSettings> PerPassDrawSettingses;
};