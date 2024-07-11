#include "TerrainSave/TerrainSaveAction.h"
#include "TerrainSave.h"

TerrainSaveAction::TerrainSaveAction(EAssetTypeCategories::Type category) {
	_assetCategory = category;
}

FText TerrainSaveAction::GetName() const {
	return FText::FromString("Terrain Save");
}

FColor TerrainSaveAction::GetTypeColor() const {
	return FColor::FromHex("228B22");
}

UClass* TerrainSaveAction::GetSupportedClass() const {
	return UTerrainSave::StaticClass();
}

uint32 TerrainSaveAction::GetCategories() {
	return _assetCategory;
}