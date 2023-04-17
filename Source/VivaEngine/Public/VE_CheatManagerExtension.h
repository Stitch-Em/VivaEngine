// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "VE_CheatManagerExtension.generated.h"

/**
 * 
 */
UCLASS()
class VIVAENGINE_API UVE_CheatManagerExtension : public UCheatManagerExtension
{
	GENERATED_BODY()
public:

	UFUNCTION(exec)
		void TestDebug(FString DebugText);
};
