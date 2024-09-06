// Fill out your copyright notice in the Description page of Project Settings.


#include "VE_Notification_Subsystem.h"


void UVE_Notification_Subsystem::BindAll()
{
	OnAlert.AddUniqueDynamic(this, &UVE_Notification_Subsystem::SendAlertCalled);
}

void UVE_Notification_Subsystem::SendAlertCalled(FVE_CAlert Alert)
{
	return;
}

void UVE_Notification_Subsystem::SendAlert(FVE_CAlert Alert)
{
	OnAlert.Broadcast(Alert);
	return;
}