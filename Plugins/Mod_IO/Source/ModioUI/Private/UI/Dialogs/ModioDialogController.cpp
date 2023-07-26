/*
 *  Copyright (C) 2021 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io UE4 Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue4/blob/main/LICENSE>)
 *
 */

#include "UI/Dialogs/ModioDialogController.h"
#include "UI/CommonComponents/ModioMenu.h"
#include "Core/ModioAuthenticationContextUI.h"
#include "Libraries/ModioErrorConditionLibrary.h"
#include "ModioUISubsystem.h"
#include "UI/BaseWidgets/Slate/SModioButtonBase.h"
#include "UI/Dialogs/ModioDialogBaseInternal.h"
#include "UI/Interfaces/IModioUITextValidator.h"
#include "UI/Interfaces/IModioUIDialogButtonWidget.h"
#include "UI/Styles/ModioButtonStyle.h"
#include "UI/Styles/ModioUIStyleRef.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBackgroundBlur.h"

void UModioDialogController::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MyBackgroundBlur)
	{
		MyBackgroundBlur->Invalidate(EInvalidateWidgetReason::PaintAndVolatility | EInvalidateWidgetReason::Visibility |
									 EInvalidateWidgetReason::Layout);
	}
	SetVisibility(bCurrentlyDisplayingDialog ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>())
	{
		Subsystem->OnSubscriptionStatusChanged.AddUObject(this, &UModioDialogController::HandleUnsubscribe);
	}
}

void UModioDialogController::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyBackgroundBlur.Reset();
	MyBackgroundBox.Reset();
	MyBackgroundButton.Reset();
	ClickDisableButton.Reset();
}

FReply UModioDialogController::OnBackgroundButtonClicked()
{
	FinalizeDialog();
	return FReply::Handled();
}

UModioDialogBaseInternal* UModioDialogController::GetActualDialog()
{
	if (ActualDialog)
	{
		return ActualDialog;
	}
	else
		return nullptr;
}

bool UModioDialogController::TrySetFocusToActiveDialog()
{
	if (DialogStack.Num() && ActualDialog && !ActualDialog->HasFocusedDescendants())
	{
		ActualDialog->SetDialogFocus();
		return true;
	}

	return DialogStack.Num() && ActualDialog && ActualDialog->HasFocusedDescendants();
}

float UModioDialogController::GetBlurStrength() const
{
	FSlateApplication::Get().GetDeltaTime();
	return 4;
}

FEventReply UModioDialogController::HandleBackgroundClick(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	return FEventReply(true);
}

TSharedRef<SWidget> UModioDialogController::RebuildWidget()
{
	if (!ActualDialog)
	{
		ActualDialog = NewObject<UModioDialogBaseInternal>();
		ActualDialog->SetDialogController(this);
	}
	// clang-format off
				
	MyBackgroundBox = SNew(SBox)
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SAssignNew(MyBackgroundButton, SModioButtonBase)
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.ContentPadding(0.0f)
				.IsFocusable(true)
				.ClickMethod(EButtonClickMethod::DownAndUp)
				.OnClicked_UObject(this, &UModioDialogController::OnBackgroundButtonClicked)
				[
					SAssignNew(MyBackgroundBlur, SBackgroundBlur)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(0.0f)
					.BlurStrength_UObject(this, &UModioDialogController::GetBlurStrength)
					[
						SAssignNew(ClickDisableButton, SModioButtonBase)
						.IsFocusable(false)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(880.0f)
							[
								ActualDialog->TakeWidget()
							]
						]
					]
				]
			];

	if (const FModioButtonStyle* ResolvedButtonStyle = InvisibleButtonStyleRef.FindStyle<FModioButtonStyle>())
	{
		MyBackgroundButton->SetButtonStyle(&ResolvedButtonStyle->ButtonStyle);
		ClickDisableButton->SetButtonStyle(&ResolvedButtonStyle->ButtonStyle);
	}

	return MyBackgroundBox.ToSharedRef();
	// clang-format on
}

void UModioDialogController::HandleEmailRequestSent(FModioErrorCode ec, UModioDialogInfo* DestinationOnSuccess)
{
	ActualDialog->HideLoadingSpinner();
	if (ec)
	{
		// If the dialog has an input widget, check if we can surface an error on it
		if (ActualDialog->InputWidget)
		{
			if (ActualDialog->InputWidget->Implements<UModioUITextValidator>())
			{
				// Initialize error message in case below ifs fail
				FText Error = FText::FromString("An unknown error occurred");

				if (UModioErrorConditionLibrary::ErrorCodeMatches(ec, EModioErrorCondition::InvalidArgsError))
				{
					if (UModioSubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioSubsystem>())
					{
						TArray<FModioValidationError> LastValidationError = Subsystem->GetLastValidationError();
						if (LastValidationError.Num())
						{
							// todo: @modio-ui support localization
							Error = FText::FromString(LastValidationError[0].ValidationFailureDescription);
						}
					}
				}
				else
				{
					Error = FText::FromString(ec.GetErrorMessage());
				}
				IModioUITextValidator::Execute_SetValidationError(ActualDialog->InputWidget, Error);
			}
		}
	}
	else
	{
		if (UModioSubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioSubsystem>())
		{
			Subsystem->FetchExternalUpdatesAsync(
				FOnErrorOnlyDelegateFast::CreateUObject(this, &UModioDialogController::OnFetchExternalCompleted));

			PushDialog(DestinationOnSuccess);
		}
	}
}

void UModioDialogController::HandleAuthCodeRequestSent(FModioErrorCode ec)
{
	ActualDialog->HideLoadingSpinner();
	if (ec)
	{
		// If the dialog has an input widget, check if we can surface an error on it
		if (ActualDialog->InputWidget)
		{
			if (ActualDialog->InputWidget->Implements<UModioUITextValidator>())
			{
				// Initialize error message in case below ifs fail
				FText Error = FText::FromString("An unknown error occurred");

				if (UModioErrorConditionLibrary::ErrorCodeMatches(ec, EModioErrorCondition::InvalidArgsError))
				{
					if (UModioSubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioSubsystem>())
					{
						TArray<FModioValidationError> LastValidationError = Subsystem->GetLastValidationError();
						if (LastValidationError.Num())
						{
							// todo: @modio-ui support localization
							Error = FText::FromString(LastValidationError[0].ValidationFailureDescription);
						}
					}
				}
				else
				{
					Error = FText::FromString(ec.GetErrorMessage());
				}
				IModioUITextValidator::Execute_SetValidationError(ActualDialog->InputWidget, Error);
			}
		}
	}
	else
	{
		if (UModioSubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioSubsystem>())
		{
			Subsystem->FetchExternalUpdatesAsync(
				FOnErrorOnlyDelegateFast::CreateUObject(this, &UModioDialogController::OnFetchExternalCompleted));
			FinalizeDialog();
		}
	}
}

void UModioDialogController::SendEmailCodeRequest(FString EmailAddress, UModioDialogInfo* DestinationOnSuccess)
{
	if (UModioSubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioSubsystem>())
	{
		Subsystem->RequestEmailAuthCodeAsync(
			FModioEmailAddress(EmailAddress),
			FOnErrorOnlyDelegateFast::CreateUObject(this, &UModioDialogController::HandleEmailRequestSent,
													DestinationOnSuccess));
	}
}

void UModioDialogController::SubmitEmailAuthCode(FString Code)
{
	// Proxy the request through the UI subsystem instead of hitting the UI subsystem directly so it broadcasts events
	// to the UI as well as notifying us
	if (UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>())
	{
		Subsystem->RequestEmailAuthentication(
			FModioEmailAuthCode(Code),
			FOnErrorOnlyDelegateFast::CreateUObject(this, &UModioDialogController::HandleAuthCodeRequestSent));
	}
}

void UModioDialogController::PopDialog()
{
	if (DialogStack.Num())
	{
		DialogStack.Pop();
		DialogInputValues.Pop();
	}

	// If there was an underlying dialog to show, display it again and restore the state;
	if (DialogStack.Num())
	{
		ActualDialog->InitializeFromDialogInfo(DialogStack.Top());
		ActualDialog->SetInputWidgetString(DialogInputValues.Top());
		ActualDialog->SetDialogFocus();
	}
	else
	{
		bCurrentlyDisplayingDialog = false;
	}

	SetVisibility(bCurrentlyDisplayingDialog ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bCurrentlyDisplayingDialog) 
	{
		return;
	}
	
	UModioUISubsystem* UISubsystem = Cast<UModioUISubsystem>(GEngine->GetEngineSubsystem<UModioUISubsystem>());
	if (UISubsystem && UISubsystem->GetCurrentFocusTarget() && UISubsystem->GetCurrentFocusTarget()->IsVisible()) 
	{
		UISubsystem->GetCurrentFocusTarget()->SetKeyboardFocus();
	}
	else if (UISubsystem && UISubsystem->ModBrowserInstance) 
	{
		UISubsystem->ModBrowserInstance->SetFocus();
	}
}

void UModioDialogController::FinalizeDialog(EModioDialogReply Reply)
{
	bCurrentlyDisplayingDialog = false;
	DialogInputValues.Empty();
	DialogStack.Empty();
	SetVisibility(bCurrentlyDisplayingDialog ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	switch (Reply)
	{
		case EModioDialogReply::Confirm:
			// need to somehow bubble a confirm back
			break;
		case EModioDialogReply::Cancel:
			break;
	}

	UModioUISubsystem* UISubsystem = Cast<UModioUISubsystem>(GEngine->GetEngineSubsystem<UModioUISubsystem>());
	if (UISubsystem && UISubsystem->GetCurrentFocusTarget() && UISubsystem->GetCurrentFocusTarget()->IsVisible())
	{
		UISubsystem->GetCurrentFocusTarget()->SetKeyboardFocus();
	}
	else if (UISubsystem && UISubsystem->ModBrowserInstance)
	{
		UISubsystem->ModBrowserInstance->SetFocus();
	}

	OnDialogClosed.Broadcast();
}

void UModioDialogController::PushDialog(UModioDialogInfo* InitialDialog, UObject* DialogDataSource)
{
	if (ActualDialog && InitialDialog)
	{
		bCurrentlyDisplayingDialog = true;
		if (DialogStack.Num())
		{
			TOptional<FString> CurrentInput = ActualDialog->GetInputWidgetString();
			DialogInputValues.Top() = CurrentInput.Get("");
		}
		DialogStack.Push(InitialDialog);
		DialogInputValues.Push("");
		ActualDialog->InitializeFromDialogInfo(InitialDialog, false, DialogDataSource);
		ActualDialog->SetDialogFocus();

		MyBackgroundBlur->Invalidate(EInvalidateWidgetReason::PaintAndVolatility | EInvalidateWidgetReason::Visibility |
									 EInvalidateWidgetReason::Layout);
	}
	SetVisibility(bCurrentlyDisplayingDialog ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UModioDialogController::ShowAuthenticationChoiceDialog()
{
	if (!AuthenticationChoiceDialog.IsNull() && !DialogStack.Contains(AuthenticationChoiceDialog))
	{
		PushDialog(AuthenticationChoiceDialog.LoadSynchronous());
	}
}

void UModioDialogController::ShowModUnsubscribeDialog(UObject* DialogDataSource)
{
	if (!UnsubscribeConfirmationDialog.IsNull())
	{
		PushDialog(UnsubscribeConfirmationDialog.LoadSynchronous(), DialogDataSource);
	}
}

void UModioDialogController::ShowModReportDialog(UObject* DialogDataSource)
{
	if (!ReportContentDialog.IsNull())
	{
		PushDialog(ReportContentDialog.LoadSynchronous(), DialogDataSource);
	}
}

void UModioDialogController::ShowEmailAuthenticationDialog()
{
	if (!EmailAuthenticationDialog.IsNull())
	{
		PushDialog(EmailAuthenticationDialog.LoadSynchronous());
	}
}

void UModioDialogController::ShowLogoutDialog()
{
	if (!LogoutConfirmationDialog.IsNull())
	{
		PushDialog(LogoutConfirmationDialog.LoadSynchronous());
	}
}

void UModioDialogController::BeginExternalAuthentication(EModioAuthenticationProvider Provider)
{
	if (UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>())
	{
		Subsystem->RequestExternalAuthentication(
			Provider, FOnErrorOnlyDelegateFast::CreateUObject(
						  this, &UModioDialogController::HandleExternalAuthenticationComplete));
	}
}

void UModioDialogController::ShowReportContentDialog()
{
	if (!ReportContentDialog.IsNull())
	{
		PushDialog(ReportContentDialog.LoadSynchronous());
	}
}

void UModioDialogController::ShowUninstallConfirmationDialog(UObject* DialogDataSource)
{
	if (!UninstallConfirmationDialog.IsNull())
	{
		PushDialog(UninstallConfirmationDialog.LoadSynchronous(), DialogDataSource);
	}
}

void UModioDialogController::ShowErrorDialog(FModioErrorCode ec, bool bCloseDialogsOnOK /*= false*/)
{
	if (ErrorDisplayDialog.IsNull())
	{
		return;	
	}

	PushDialog(ErrorDisplayDialog.LoadSynchronous());
	
	ActualDialog->SetHeaderText(FText::FromString("Error " + FString::FromInt(ec.GetValue())));
	ActualDialog->SetContentText(FText::FromString(ec.GetErrorMessage()));
	// create a copy of the error widget
	// call the interface methods to set the error code
	// call the interface methods to set the close handler to either swap back or close all the dialogs altogether
}

void UModioDialogController::ShowLoadingDialog()
{
	ActualDialog->TakeWidget();
	ActualDialog->ShowLoadingSpinner();
	ActualDialog->SetDialogFocus();
}

void UModioDialogController::RequestUnsubscribe(const FModioModID& ModId, UModioDialogInfo* DestinationOnSuccess)
{
	if (UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>())
	{
		Subsystem->RequestRemoveSubscriptionForModID(
			ModId, FOnErrorOnlyDelegateFast::CreateUObject(this, &UModioDialogController::HandleUnsubscribeError));
	}
}

void UModioDialogController::RequestUninstall(const FModioModID& ModId, UModioDialogInfo* Destination)
{
	if (UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>())
	{
		Subsystem->RequestUninstallForModID(
			ModId,
			FOnErrorOnlyDelegateFast::CreateUObject(this, &UModioDialogController::HandleUninstallError, Destination));
	}
}

void UModioDialogController::ReportContentAsync(const FModioReportParams& ReportData, UModioDialogInfo* Destination)
{
	if (UModioSubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioSubsystem>())
	{
		Subsystem->ReportContentAsync(ReportData, FOnErrorOnlyDelegateFast::CreateUObject(
													  this, &UModioDialogController::HandleReportContent, Destination));
	}
}

void UModioDialogController::HandleReportContent(FModioErrorCode ec, UModioDialogInfo* ModioDialogInfo)
{
	if (!ec)
	{
		PushDialog(ModioDialogInfo);
	}
	else
	{
		ShowErrorDialog(ec);
	}
}

void UModioDialogController::HandleUnsubscribe(FModioModID ec, bool IsSubscribe)
{
	PopDialog();
}

void UModioDialogController::HandleExternalAuthenticationComplete(FModioErrorCode ec)
{
	if (ec)
	{
		ShowErrorDialog(ec);
	}
}

void UModioDialogController::HandleUnsubscribeError(FModioErrorCode ec)
{
	if (ec)
	{
		ShowErrorDialog(ec);
	}
}

void UModioDialogController::HandleUninstallError(FModioErrorCode ec, UModioDialogInfo* DestinationOnSuccess)
{
	if (!ec)
	{
		PushDialog(DestinationOnSuccess);
	}
	else
	{
		ShowErrorDialog(ec);
	}
}

void UModioDialogController::LogOut()
{
	if (UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>())
	{
		Subsystem->LogOut(FOnErrorOnlyDelegateFast::CreateUObject(this, &UModioDialogController::LogOutCallback));
	}
}

void UModioDialogController::LogOutCallback(FModioErrorCode ec)
{
	if (ec)
	{
		ShowErrorDialog(ec);
	}
	else
	{
		PopDialog();
	}
}

void UModioDialogController::ShowReportEmailDialog(UObject* DialogDataSource)
{
	if (!ReportEmailDialog.IsNull())
	{
		PushDialog(ReportEmailDialog.LoadSynchronous(), DialogDataSource);
	}
}

void UModioDialogController::ShowTermsOfUseDialog(TSharedPtr<FModioUIAuthenticationProviderInfo> ProviderInfo)
{
	if (UModioSubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioSubsystem>())
	{
		Subsystem->GetTermsOfUseAsync(ProviderInfo->ProviderID, EModioLanguage::English,
									  FOnGetTermsOfUseDelegateFast::CreateUObject(
										  this, &UModioDialogController::HandleTermsOfUseReceived, ProviderInfo));
	}
}

void UModioDialogController::HandleTermsOfUseReceived(FModioErrorCode ec, TOptional<FModioTerms> Terms,
													  TSharedPtr<FModioUIAuthenticationProviderInfo> ProviderInfo)
{
	UModioAuthenticationContextUI* AuthContext = NewObject<UModioAuthenticationContextUI>();
	AuthContext->ProviderInfo = *ProviderInfo;
	ActualDialog->HideLoadingSpinner();
	if (Terms && !ec)
	{
		AuthContext->Terms = *Terms;
		if (!TermsOfUseDialog.IsNull())
		{
			TermsOfUseDialog.LoadSynchronous();
			TermsOfUseDialog->DialogText = FText::FromString(AuthContext->Terms.TermsText);
			PushDialog(TermsOfUseDialog.Get(), AuthContext);
		}
	}
	else if (!TermsOfUseFailDialog.IsNull())
	{
		PushDialog(TermsOfUseFailDialog.LoadSynchronous(), AuthContext);
	}
}