#include "RomanceMinigameFactory.h"
#include "RomanceMinigame.h"

URomanceMinigameFactory::URomanceMinigameFactory(const FObjectInitializer& objectInitializer) : Super(objectInitializer) {
	SupportedClass = URomanceMinigame::StaticClass();
}

UObject* URomanceMinigameFactory::FactoryCreateNew(UClass* inClass, UObject* inParent, FName inName, EObjectFlags flags, UObject* context, FFeedbackContext* warn) {
	URomanceMinigame* asset = NewObject<URomanceMinigame>(inParent, inClass, inName, flags);
	return asset;
}
bool URomanceMinigameFactory::CanCreateNew() const {
	return true;
}