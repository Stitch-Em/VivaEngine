#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class RomanceMinigameAction : public FAssetTypeActions_Base {
public:
	RomanceMinigameAction(EAssetTypeCategories::Type category);

public:
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
    virtual UClass* GetSupportedClass() const override;
	//virtual void OpenAssetEditor(const TArray<UObject*>& inObjects, TSharedPtr<class IToolkitHost> editWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override;
	

private:
	EAssetTypeCategories::Type _assetCategory;
};