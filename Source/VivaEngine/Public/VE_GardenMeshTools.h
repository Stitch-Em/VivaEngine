// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
//#include "DynamicMeshBuilder.h"
//#include "DynamicMeshComponent.h"
#include "VE_GardenMeshTools.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VIVAENGINE_API UVE_GardenMeshTools : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVE_GardenMeshTools();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

//	UFUNCTION(BlueprintCallable, Category = "Dynamic Mesh Utils")
//		void EditGardenMeshHeightClosestToPlayerAsync(UDynamicMeshComponent* DynamicMesh, AActor* PlayerActor, float HeightOffset, float MaxDistance, float FalloffDistance, UDynamicMeshComponent*& OutputMesh);
		
};
