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
	;


};

USTRUCT(BlueprintType)
struct FVE_CEvent {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Key;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVE_CEvent_Storage Storage;
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
	bool OneTime;
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

	//Update the Compleated Task
	void CheckTask();


public:

	//Used to Store The Number of times a event has been called By The Event Key
	UPROPERTY(BlueprintReadOnly, Category = "VivaEngine")
	TMap<FName, int> EventMap;

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
	int GetEvent(FVE_CEvent Event);

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	bool IsTaskCompleted(FVE_CTask Task);

	UFUNCTION(BlueprintCallable, Category = "VivaEngine")
	void AddTask(FVE_CTask Task) { Tasks.Add(Task); };

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
