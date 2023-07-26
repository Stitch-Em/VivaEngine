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

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "SlateBasics.h"

class UModioCreateModParamsObject;
struct FModioModInfo;

class MODIOEDITOR_API ModioCreateModParamsDetails : public IDetailCustomization
{

public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	void DrawCreateMod(IDetailLayoutBuilder& DetailBuider);

private:

	UModioCreateModParamsObject* Target;
	TSharedPtr<SEditableTextBox> PathToLogoFileEditableTextBox;
	TSharedPtr<SEditableTextBox> NameEditableTextBox;
	TSharedPtr<SMultiLineEditableTextBox> SummaryMultiLineEditableTextBox;
};