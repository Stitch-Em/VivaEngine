// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Delegates/DelegateCombinations.h"
#include "VE_Event_Subsystem.h"
#include "Blueprint/UserWidget.h"
#include "VE_Notification_Subsystem.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class FVE_EAlert_Ribbon : uint8
{
	None = 0,
	Visit = 1,
	Resident = 2,
	Romance = 3,
	Master_Romance = 4,
	Variant = 5,
	Party = 6,
	Plant = 7,
	Fertilized = 8,
	Custom = 9
};

USTRUCT(BlueprintType)
struct FVE_CAlert {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SmallText;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVE_EAlert_Ribbon RibbonType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CustomRibbon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVE_CEvent Action;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Dialog;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName DialogName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDSendAlert, FVE_CAlert, Alert);

UCLASS()
class VIVAENGINE_API UVE_Notification_Subsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Leaf;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* VisitRibbon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* ResidentRibbon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* RomanceRibbon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Master_RomanceRibbon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* VariantRibbon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* PartyRibbon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* PlantRibbon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* FertilizedRibbon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UUserWidget* Notification_UMG;

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void BindAll();

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void SendAlert(FVE_CAlert Alert);

	UPROPERTY(BlueprintAssignable, Category = "VivaEngine")
	FDSendAlert OnAlert;

	UFUNCTION()
	void SendAlertCalled(FVE_CAlert Alert);

};
