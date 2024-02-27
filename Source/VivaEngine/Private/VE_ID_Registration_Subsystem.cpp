// Fill out your copyright notice in the Description page of Project Settings.


#include "VE_ID_Registration_Subsystem.h"

FVE_ID UVE_ID_Registration_Subsystem::GenerateID(FName ObjectName)
{
	if (RegisteredIDs.Find(ObjectName))
	{
		int number = *RegisteredIDs.Find(ObjectName);
		number++;
		RegisteredIDs.Add(ObjectName, number);
	}
	else
	{
		RegisteredIDs.Add(ObjectName, 1);
	}

	//append number to name using _ as separator
	FString IDString = ObjectName.ToString() + "_" + FString::FromInt(RegisteredIDs[ObjectName]);

	return FVE_ID(ObjectName, RegisteredIDs[ObjectName], FName(*IDString));
	
}

void UVE_ID_Registration_Subsystem::Set_ID(UObject* Object, FVE_ID ID)
{
	if (IDMap.Find(Object))
	{
		IDMap[Object] = ID;
	}
	else
	{
		IDMap.Add(Object, ID);
	}
	RegisteredIDs.Add(ID.Name, ID.ID);
	if (!SubscribedObjects.Contains(Object))
	{
		SubscribedObjects.Add(Object);
		SubscribedObjectNames.Add(Object, ID.Name);
	}
}

FVE_ID UVE_ID_Registration_Subsystem::GetUniqueID(UObject* Object)
{
	if (IDMap.Find(Object))
	{
		return *IDMap.Find(Object);
	}
	else
	{
		return FVE_ID();
	}
}

bool UVE_ID_Registration_Subsystem::HasUniqueID(UObject* Object)
{
	if (IDMap.Find(Object))
	{
		return true;
	}
	else
	{
		return false;
	}
}

FVE_ID UVE_ID_Registration_Subsystem::Subscribe(UObject* Object, FName ObjectName)
{
	FVE_ID ID = GenerateID(ObjectName);
	IDMap.Add(Object, ID);
	SubscribedObjects.Add(Object);
	SubscribedObjectNames.Add(Object, ObjectName);
	return ID;
	
}

void UVE_ID_Registration_Subsystem::Unsubscribe(UObject* Object)
{
	if (IDMap.Find(Object))
	{
		IDMap.Remove(Object);
	}
	if (SubscribedObjects.Contains(Object))
	{
		SubscribedObjects.Remove(Object);
	}
	if (SubscribedObjectNames.Contains(Object))
	{
		SubscribedObjectNames.Remove(Object);
	}
}

void UVE_ID_Registration_Subsystem::UnsubscribeAll()
{
	RegisteredIDs.Empty();
	IDMap.Empty();
	SubscribedObjects.Empty();
	SubscribedObjectNames.Empty();
}

void UVE_ID_Registration_Subsystem::ResetAllIDs()
{
	RegisteredIDs.Empty();
	IDMap.Empty();
	for (UObject* Object : SubscribedObjects)
	{
		FName Name = SubscribedObjectNames[Object];
		FVE_ID ID = GenerateID(Name);
		IDMap.Add(Object, ID);

	}
}


