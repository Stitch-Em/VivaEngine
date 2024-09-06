#pragma once

#include "CoreMinimal.h"
#include "TerrainSaveFactory.generated.h"

UCLASS()
class UTerrainSaveFactory : public UFactory {
	GENERATED_BODY()
	

public:
	UTerrainSaveFactory(const FObjectInitializer& objectInitializer);

public:
	virtual UObject* FactoryCreateNew(UClass* inClass, UObject* inParent, FName inName, EObjectFlags flags, UObject* context, FFeedbackContext* warn);
	virtual bool CanCreateNew() const override;
};