#pragma once

#include "CoreMinimal.h"
#include "RomanceMinigameFactory.generated.h"

UCLASS()
class URomanceMinigameFactory : public UFactory {
	GENERATED_BODY()
	

public:
	URomanceMinigameFactory(const FObjectInitializer& objectInitializer);

public:
	virtual UObject* FactoryCreateNew(UClass* inClass, UObject* inParent, FName inName, EObjectFlags flags, UObject* context, FFeedbackContext* warn);
	virtual bool CanCreateNew() const override;
};