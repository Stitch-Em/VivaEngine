// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "VE_ID_Registration_Subsystem.generated.h"

/**
 * 
 */


//Struct to store a FName and a int for a "VE_ID"
USTRUCT(BlueprintType)
struct FVE_ID
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name_ID;
};

UCLASS()
class VIVAENGINE_API UVE_ID_Registration_Subsystem : public UEngineSubsystem
{
	GENERATED_BODY()

private:
	TMap<FName, int> RegisteredIDs;
	TMap<UObject*, FVE_ID> IDMap;
	TArray<UObject*> SubscribedObjects;
	TMap<UObject*, FName> SubscribedObjectNames;
	
	FVE_ID GenerateID(FName ObjectName);

public:

	UFUNCTION(BlueprintCallable, Category = "VivaEngine", meta = (DefaultToSelf = "Object"))
	void Set_ID(UObject* Object, FVE_ID ID);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VivaEngine", meta = (DefaultToSelf = "Object"))
	FVE_ID GetUniqueID(UObject* Object);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VivaEngine", meta = (DefaultToSelf = "Object"))
	bool HasUniqueID(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "VivaEngine", meta = (DefaultToSelf = "Object"))
	FVE_ID Subscribe(UObject* Object, FName ObjectName);

	UFUNCTION(BlueprintCallable, Category = "VivaEngine", meta = (DefaultToSelf = "Object"))
	void Unsubscribe(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void UnsubscribeAll();

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void ResetAllIDs();
	
};
