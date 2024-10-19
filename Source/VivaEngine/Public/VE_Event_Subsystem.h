// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "VE_Event_Subsystem.generated.h"


/**
 * 
 */

UENUM(BlueprintType)
enum EVE_ObjectType
{
	None,
	Seed UMETA(ToolTip = "Something that inpegnates the ground"),
	Fruit UMETA(ToolTip = "Useless Gay Rotting piece of shit"),
	Rubbish UMETA(ToolTip = "The rarest and bestest item in this shitty game"),
	Veggie UMETA(ToolTip = "Usless Non-Gay Rotting piece of shit"),
	FlowerHead UMETA(ToolTip = "Otherwise known as your mom!"),
	Produce UMETA(ToolTip = "The transgenders of consumable items"),
	Coinage UMETA(ToolTip = "Something that you will never have..."),
	Tile UMETA(ToolTip = "Step on me harder daddy =P"),
	Candy UMETA(ToolTip = "The mouth candy, not the nose one"),
	Decor UMETA(ToolTip = "Do I REALLY need to explain what this is?"),
	Building UMETA(ToolTip = "Come inside me daddy =P"),
	Fence UMETA(ToolTip = "Only borderline ofFENCEive"),
	Egg UMETA(ToolTip = "Kakyoin, did you lay this egg?"),
	Toy UMETA(ToolTip = "0_O that's kinky"),
	Plant UMETA(ToolTip = "The lovechild the impregnated ground gives birth to"),
	Animal UMETA(ToolTip = "A Papery fleshed animal that has sex"),
};


USTRUCT(BlueprintType)
struct FVE_CEvent_Storage {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Bool;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Int;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Float;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString String;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UUserWidget> WidgetClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UObject> Object;
};

USTRUCT(BlueprintType)
struct FVE_CEvent {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Key;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVE_CEvent_Storage Storage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EVE_ObjectType> ObjectType;
};

USTRUCT(BlueprintType)
struct FVE_CTask {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName EventKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int TriggerTimes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName TaskKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EVE_ObjectType> ItemObjectType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool OneTime;
};

USTRUCT(BlueprintType)
struct FVE_EventMapDetails {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int TimesTriggered;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TEnumAsByte<EVE_ObjectType> ItemType;

	FVE_EventMapDetails() {
		TimesTriggered = 0;
		ItemType = None;
	}

	FVE_EventMapDetails(int timesTriggered, EVE_ObjectType itemType) {
		TimesTriggered = timesTriggered;
		ItemType = itemType;
	}
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAddedEvent, FVE_CEvent, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDOnetimeEvent, FVE_CEvent, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCompletedTask, FVE_CTask, Task);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIncompletedTask, FVE_CTask, Task);

UCLASS()
class VIVAENGINE_API UVE_Event_Subsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	
	
private:

	//Update the Completed Task
	void CheckTask();

	TArray<FString> TaskTypes = TArray<FString>{ "In Garden", "NOT In Garden", 
		"Eaten", "Won Fight", "Lost Fight", "Wearing Acessory", 
		"Pinometer Count", "Joy Status", "Garden Value"};

public:

	//Used to Store The Number of times a event has been called By The Event Key
	UPROPERTY(BlueprintReadOnly, Category = "VivaEngine")
	TMap<FName, FVE_EventMapDetails> EventMap;

	//Used to Store The Event Storage By The Event Key
	UPROPERTY(BlueprintReadOnly, Category = "VivaEngine")
	TMap<FName, FVE_CEvent_Storage> EventStorageMap;

	//Completed Task Key Array
	UPROPERTY(BlueprintReadOnly, Category = "VivaEngine")
	TArray<FName> CompletedTasks;

	//Array of Tasks
	UPROPERTY(BlueprintReadOnly, Category = "VivaEngine")
	TArray<FVE_CTask> Tasks;
	
	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void AddEvent(FVE_CEvent Event);

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void OnetimeEvent(FVE_CEvent Event);

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void RemoveEvent(FVE_CEvent Event, bool All);

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void RemoveAllEventsWithEventKeySubstring(FString Substring);

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void RemoveAllTaskWithEventKeySubstring(FString Substring);

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void BindAll();

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void GetEventDetails(FVE_CEvent Event, FVE_EventMapDetails& Details, bool& IsValid);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VivaEngine")
	bool IsTaskCompleted(FVE_CTask Task);

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void AddTask(FVE_CTask Task);

	UPROPERTY(BlueprintAssignable, Category = "VivaEngine")
	FAddedEvent OnAddedEvent;

	UPROPERTY(BlueprintAssignable, Category = "VivaEngine")
	FDOnetimeEvent OnOnetimeEvent;

	UPROPERTY(BlueprintAssignable, Category = "VivaEngine")
	FCompletedTask OnCompletedTask;

	UPROPERTY(BlueprintAssignable, Category = "VivaEngine")
	FIncompletedTask OnIncompletedTask;

	UFUNCTION()
	void OnAddedEventCalled(FVE_CEvent Event);

	UFUNCTION()
	void OnOnetimeEventCalled(FVE_CEvent Event);

	UFUNCTION()
	void OnCompletedTaskCalled(FVE_CTask Task);

	UFUNCTION()
	void OnIncompletedTaskCalled(FVE_CTask Task);

};
