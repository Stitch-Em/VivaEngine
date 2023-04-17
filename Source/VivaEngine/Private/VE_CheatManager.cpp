// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "VE_CheatManager.h"

void UVE_CheatManager::TestDebug(FString DebugText) {

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("%s = %s"), *TEXT("DebugText"), *DebugText));
}