// Fill out your copyright notice in the Description page of Project Settings.


#include "VE_Event_Subsystem.h"

void UVE_Event_Subsystem::BindAll()
{
	OnAddedEvent.AddUniqueDynamic(this, &UVE_Event_Subsystem::OnAddedEventCalled);
	OnOnetimeEvent.AddUniqueDynamic(this, &UVE_Event_Subsystem::OnOnetimeEventCalled);
	OnCompletedTask.AddUniqueDynamic(this, &UVE_Event_Subsystem::OnCompletedTaskCalled);
	OnIncompletedTask.AddUniqueDynamic(this, &UVE_Event_Subsystem::OnIncompletedTaskCalled);
}

int UVE_Event_Subsystem::GetEvent(FVE_CEvent Event)
{
	if (EventMap.Find(Event.Key)) {
		return *EventMap.Find(Event.Key);
	}
	else {
		return 0;
	}
}

bool UVE_Event_Subsystem::IsTaskCompleted(FVE_CTask Task)
{
	if(CompletedTasks.Find(Task.TaskKey)){
		return true;
	}
	else {
		return false;
	}
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
	//For each task in the task array
	for (FVE_CTask Task : Tasks) {
		if (EventMap.Find(Task.EventKey) && *EventMap.Find(Task.EventKey) >= Task.TriggerTimes) {
			//If the task is a one time task and it has been completed 
			if (Task.OneTime && CompletedTasks.Find(Task.TaskKey)) {

				return;
				
			}
			else {

				CompletedTasks.Add(Task.TaskKey);

				OnCompletedTask.Broadcast(Task);
			}
		}
		else {
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
		int number = *EventMap.Find(Event.Key);
		number++;
		EventMap.Add(Event.Key, number);
	}
	else {
		EventMap.Add(Event.Key, 1); // Else we will add the event to the map and set the number of times called to 1.
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

			int number = *EventMap.Find(Event.Key);
			number--;

			if (number <= 0) {
				EventMap.Remove(Event.Key);
				EventStorageMap.Remove(Event.Key);
			}
			else {
				EventMap.Add(Event.Key, number);
			}
		}
	}

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

void UVE_Event_Subsystem::RemoveAllTaskWithEventKeySubstring(FString Substring)
{
	int num = 0;
	TArray <FVE_CTask> TasksCopy = Tasks;
	for (FVE_CTask Task : TasksCopy)
	{
		num++;
		FName EventKey = Task.EventKey;
		FString EventKeyString = EventKey.ToString();

		if (EventKeyString.Contains(Substring)) {
			//RemoveFromCOmpleatedTasks
			CompletedTasks.Remove(Task.TaskKey);
			//RemoveFromTasks
			Tasks.RemoveAt(num);
		}
	}
	TasksCopy.Empty();
	return;
}
