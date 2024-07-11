#include "TerrainSave/TerrainSaveFactory.h"
#include "TerrainSave.h"

UTerrainSaveFactory::UTerrainSaveFactory(const FObjectInitializer& objectInitializer) : Super(objectInitializer) {
	SupportedClass = UTerrainSave::StaticClass();
}

UObject* UTerrainSaveFactory::FactoryCreateNew(UClass* inClass, UObject* inParent, FName inName, EObjectFlags flags, UObject* context, FFeedbackContext* warn) {
	UTerrainSave* asset = NewObject<UTerrainSave>(inParent, inClass, inName, flags);
	return asset;
}
bool UTerrainSaveFactory::CanCreateNew() const {
	return true;
}