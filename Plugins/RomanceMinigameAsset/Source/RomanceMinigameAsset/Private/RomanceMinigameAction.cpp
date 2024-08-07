#include "RomanceMinigameAction.h"
#include "RomanceMinigame.h"

RomanceMinigameAction::RomanceMinigameAction(EAssetTypeCategories::Type category) {
	_assetCategory = category;
}

FText RomanceMinigameAction::GetName() const {
	return FText::FromString("Romance Minigame");
}

FColor RomanceMinigameAction::GetTypeColor() const {
	return FColor::FromHex("FF28B9");
}

UClass* RomanceMinigameAction::GetSupportedClass() const {
	return URomanceMinigame::StaticClass();
}

//void RomanceMinigameAction::OpenAssetEditor(const TArray<UObject*>& inObjects, TSharedPtr<class IToolkitHost> editWithinLevelEditor = TSharedPtr<IToolkitHost>()) {

	//ToDo: Open the romance minigame editor
//}

uint32 RomanceMinigameAction::GetCategories() {
	return _assetCategory;
}