#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class TerrainSaveAction : public FAssetTypeActions_Base {
public:
	TerrainSaveAction(EAssetTypeCategories::Type category);

public:
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
    virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	

private:
	EAssetTypeCategories::Type _assetCategory;
};