// Fill out your copyright notice in the Description page of Project Settings.


#include "VE_Event_Subsystem.h"

void UVE_Event_Subsystem::BindAll()
{
	OnAddedEvent.AddUniqueDynamic(this, &UVE_Event_Subsystem::OnAddedEventCalled);
	OnOnetimeEvent.AddUniqueDynamic(this, &UVE_Event_Subsystem::OnOnetimeEventCalled);
	OnCompletedTask.AddUniqueDynamic(this, &UVE_Event_Subsystem::OnCompletedTaskCalled);
	OnIncompletedTask.AddUniqueDynamic(this, &UVE_Event_Subsystem::OnIncompletedTaskCalled);
}

void UVE_Event_Subsystem::GetEventDetails(FVE_CEvent Event, FVE_EventMapDetails& Details, bool& IsValid)
{
	IsValid = (EventMap.Find(Event.Key) != nullptr);

	if (IsValid) {
		Details = *EventMap.Find(Event.Key);
	}
	else {
		Details = FVE_EventMapDetails();
	}
}

bool UVE_Event_Subsystem::IsTaskCompleted(FVE_CTask Task)
{
	return CompletedTasks.Contains(Task.TaskKey);
}

void UVE_Event_Subsystem::OnAddedEventCalled(FVE_CEvent Event)
{
	return;
}

void UVE_Event_Subsystem::OnOnetimeEventCalled(FVE_CEvent Event)
{
	return;
}



void UVE_Event_Subsystem::OnCompletedTaskCalled(FVE_CTask Task)
{
	return;
}

void UVE_Event_Subsystem::OnIncompletedTaskCalled(FVE_CTask Task)
{
	return;
}

void UVE_Event_Subsystem::CheckTask()
{
	TArray<FName> AllEventMapNames;
	EventMap.GenerateKeyArray(AllEventMapNames);

	//For each task in the task array
	for (FVE_CTask Task : Tasks) {

		bool addToCompletedTaskList = false;
		bool removeFromCompletedTaskList = false;

		//first, check to see if it's a task that can be completed with any of an item type
		switch (Task.TaskKey.ToString().Contains("anyitemoftype", ESearchCase::IgnoreCase, ESearchDir::FromStart)) {

			//For finding tasks that need a specific item
		case false:
			if (EventMap.Find(Task.EventKey) && EventMap.Find(Task.EventKey)->TimesTriggered >= Task.TriggerTimes) {
				//If the task is a one time task and it has been completed, don't add to completed task list 
				addToCompletedTaskList = !(Task.OneTime && CompletedTasks.Contains(Task.TaskKey));
				removeFromCompletedTaskList = false;
			}
			else {
				addToCompletedTaskList = false;
				removeFromCompletedTaskList = true;
			}
			break;

			//Check if the task can be completed with a nonspecific item
		case true:
			int NumberOfMatchingItemTypes = 0;

			for (FName eventKey : AllEventMapNames) {
				//To check whether it's any normal item or any ROTTING item
				bool rotStatusMatches = (eventKey.ToString().Contains("Rotten", ESearchCase::IgnoreCase, ESearchDir::FromStart) == Task.EventKey.ToString().Contains("Rotten", ESearchCase::IgnoreCase, ESearchDir::FromStart));
				
				if (EventMap.Find(eventKey)->ItemType == Task.ItemObjectType && rotStatusMatches) {
					for (FString TaskType : TaskTypes) {
						if (Task.TaskKey.ToString().Contains(TaskType) && eventKey.ToString().Contains(TaskType)) {
							NumberOfMatchingItemTypes = NumberOfMatchingItemTypes + EventMap.Find(eventKey)->TimesTriggered;
							break;
						}
					}
				}
			}

			if (NumberOfMatchingItemTypes >= Task.TriggerTimes) {
				addToCompletedTaskList = !(Task.OneTime && CompletedTasks.Contains(Task.TaskKey));
				removeFromCompletedTaskList = false;
			}
			else {
				addToCompletedTaskList = false;
				removeFromCompletedTaskList = true;
			}
			break;
		}

		if (addToCompletedTaskList) {
			CompletedTasks.Add(Task.TaskKey);

			OnCompletedTask.Broadcast(Task);
		}

		if (removeFromCompletedTaskList) {
			//removes task from completed list if it becomes incomplete
			if (CompletedTasks.Find(Task.TaskKey)) {
				CompletedTasks.Remove(Task.TaskKey);
				OnIncompletedTask.Broadcast(Task);
			}
		}
	}
	return;
}

void UVE_Event_Subsystem::AddEvent(FVE_CEvent Event)
{
	
	//If we find the event in the map we will increment the number of times the event has been called
	if (EventMap.Find(Event.Key)) {
		int number = EventMap.Find(Event.Key)->TimesTriggered;
		number++;
		EventMap.Add(Event.Key, FVE_EventMapDetails(number, Event.ObjectType));
	}
	else {
		EventMap.Add(Event.Key, FVE_EventMapDetails(1, Event.ObjectType)); // Else we will add the event to the map and set the number of times called to 1.
	}
	
	//We will also add the event storage to the map overriting any previous storage for that event
	EventStorageMap.Add(Event.Key, Event.Storage);

	//We want to call the On added event dispatcher
	OnAddedEvent.Broadcast(Event);

	//Then we check to see if a Task has been completed.

	CheckTask();

	return;
}

void UVE_Event_Subsystem::OnetimeEvent(FVE_CEvent Event)
{
	//We want to call the Onetime event dispatcher once added here.
	OnOnetimeEvent.Broadcast(Event);
	return;
}



void UVE_Event_Subsystem::RemoveEvent(FVE_CEvent Event, bool All)
{
//If we want to remove all events of the same key
	if (All) {
		//We will remove all events of the same key
		EventMap.Remove(Event.Key);
		EventStorageMap.Remove(Event.Key);
	}
	else {
		//We will remove only one event of the same key and if that is 0 then we will remove the key from the map
		if (EventMap.Find(Event.Key)) {

			int number = EventMap.Find(Event.Key)->TimesTriggered;
			number--;

			if (number <= 0) {
				EventMap.Remove(Event.Key);
				EventStorageMap.Remove(Event.Key);
			}
			else {
				EventMap.Add(Event.Key, FVE_EventMapDetails(number, Event.ObjectType));
			}
		}
	}

	//We now want to check if changing the event map causes any tasks to become incomplete.

	CheckTask();

	return;

}

void UVE_Event_Subsystem::RemoveAllEventsWithEventKeySubstring(FString Substring)
{
	
	TArray<FName> Keys;
	EventMap.GetKeys(Keys);

	for (FName key : Keys) {
		FString EventKeyString = key.ToString();

		if (EventKeyString.Contains(Substring)) {
			EventMap.Remove(key);
			EventStorageMap.Remove(key);
		}
	}
	return;
}

void UVE_Event_Subsystem::AddTask(FVE_CTask Task)
{
	//Check to see if the task being added already exists.


	//This If Statement breaks UE5.4 support
	//'==': no operator found ... 'const ComparisonType' (or there is no acceptable conversion)
	// The task array is of structs not FNames, not sure why it worked in 5.3
	
	//if(!Tasks.Contains(Task.TaskKey)){ 

	//UE 5.4 fix
	bool Contains = false;
	for (FVE_CTask Taskfor : Tasks)
	{
		FName forTaskKey = Taskfor.TaskKey;
		FName normTaskKey = Task.TaskKey;

		if (forTaskKey == Task.TaskKey)
		{
			Contains = true;
		}
	}

	if (!Contains) {
		Tasks.Add(Task);
		CheckTask();
	}
	
}

void UVE_Event_Subsystem::RemoveAllTaskWithEventKeySubstring(FString Substring)
{
	int num = 0;
	TArray <FVE_CTask> TasksCopy = Tasks;
	for (FVE_CTask Task : TasksCopy)
	{
		FName EventKey = Task.EventKey;
		FString EventKeyString = EventKey.ToString();

		if (EventKeyString.Contains(Substring)) {
			//RemoveFromCompletedTasks
			CompletedTasks.Remove(Task.TaskKey);
			//RemoveFromTasks
			Tasks.RemoveAt(num);
		}
		num++;
	}
	TasksCopy.Empty();
	return;
}
