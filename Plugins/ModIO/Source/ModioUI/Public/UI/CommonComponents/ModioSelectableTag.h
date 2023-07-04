/*
 *  Copyright (C) 2021 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io UE4 Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue4/blob/main/LICENSE>)
 *
 */

#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "CoreMinimal.h"
#include "UI/Styles/ModioCheckBoxStyle.h"
#include "UI/BaseWidgets/ModioCheckBox.h"
#include "UI/BaseWidgets/ModioRichTextBlock.h"
#include "UI/BaseWidgets/ModioUserWidgetBase.h"
#include "UI/Interfaces/IModioUIUserStringListEntry.h"

#include "ModioSelectableTag.generated.h"

/**
* Modio UI element that represents a selectable tag using a checkbox as
* visual indicator
**/
UCLASS()
class MODIOUI_API UModioSelectableTag : public UModioUserWidgetBase,
										public IUserObjectListEntry,
										public IModioUIUserStringListEntry
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Widgets", meta = (BindWidget))
	UModioCheckBox* TagSelectedCheckbox;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (StyleClass = "ModioCheckBoxStyle"), Category = "Widgets")
	FModioUIStyleRef NormalStyle;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (StyleClass = "ModioCheckBoxStyle"), Category = "Widgets")
	FModioUIStyleRef FocusedStyle;

	bool IsCheckboxChecked();
	FString GetTagString();

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	virtual void OnCheckboxStateChanged(bool bIsChecked);

	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

	virtual void NativeSetListItemString(const FString& InString) override
	{
		if (TagSelectedCheckbox)
		{
			TagSelectedCheckbox->SetLabelText(FText::FromString(InString));
		}
	};
};