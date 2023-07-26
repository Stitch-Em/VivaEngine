/*
 *  Copyright (C) 2021 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io UE4 Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue4/blob/main/LICENSE>)
 *
 */

#include "UI/Views/ModDetails/ModioModDetailsView.h"
#include "Engine/Engine.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameFramework/InputSettings.h"
#include "Loc/BeginModioLocNamespace.h"
#include "ModioSubsystem.h"
#include "Types/ModioRating.h"
#include "Settings/ModioUISettings.h"
#include "UI/CommonComponents/ModioErrorRetryWidget.h"
#include "UI/CommonComponents/ModioMenu.h"
#include "UI/Commands/ModioCommonUICommands.h"
#include "UI/EventHandlers/IModioUIAuthenticationChangedReceiver.h"
#include "UI/Interfaces/IModioUIAsyncHandlerWidget.h"
#include "UI/Views/ModDetails/ModioModDetailsOpProgress.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "TimerManager.h"

void UModioModDetailsView::NativeConstruct()
{
	Super::NativeConstruct();
	IModioUIModDetailsDisplay::Register<UModioModDetailsView>();
}

void UModioModDetailsView::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (DetailsLoader)
	{
		TScriptInterface<IModioUIAsyncOperationWidget> InterfaceWrapper =
			TScriptInterface<IModioUIAsyncOperationWidget>(this);
		IModioUIAsyncHandlerWidget::Execute_LinkAsyncOperationWidget(DetailsLoader, InterfaceWrapper);
	}

	if (RateUpButton)
	{
		RateUpButton->OnClicked.AddDynamic(this, &UModioModDetailsView::RateUpClicked);
	}

	if (RateDownButton)
	{
		RateDownButton->OnClicked.AddDynamic(this, &UModioModDetailsView::RateDownClicked);
	}

	if (ReportButton)
	{
		ReportButton->OnClicked.AddDynamic(this, &UModioModDetailsView::ReportClicked);
	}

	if (DetailsViewContent)
	{
		DetailsViewContent->SetActualAsyncOperationWidget(TScriptInterface<IModioUIAsyncOperationWidget>(this));
	}

	IModioUIModInfoReceiver::Register<UModioModDetailsView>(EModioUIModInfoEventType::GetModInfo);

	bCachedSubscriptionState = false;

	if (SubscribeButton)
	{
		SubscribeButton->OnClicked.AddDynamic(this, &UModioModDetailsView::NativeSubscribeClicked);
	}

	UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>();
	if (Subsystem)
	{
		Subsystem->OnSubscriptionStatusChanged.AddUObject(this, &UModioModDetailsView::OnModSubscriptionStatusChanged);
		Subsystem->ModBrowserInstance->OnDownloadQueueClosed.AddDynamic(this, &UModioModDetailsView::OnDownloadQueueClosed);
		Subsystem->ModBrowserInstance->GetDialogController()->OnDialogClosed.AddDynamic(this, &UModioModDetailsView::OnDialogClosed);
	}
}

void UModioModDetailsView::OnModSubscriptionStatusChanged(FModioModID ID, bool Subscribed)
{
	if (DataSource)
	{
		UModioModInfoUI* Data = Cast<UModioModInfoUI>(DataSource);
		if (Data)
		{
			if (Data->Underlying.ModId == ID)
			{
				if (SubscribeButton)
				{
					GWorld->GetTimerManager().SetTimer(SetFocusTimerHandle, this, &UModioModDetailsView::EnableSubscribeButton, 0.6, false);
				}

				if (SubscriptionBadge)
				{
					SubscriptionBadge->SetVisibility(Subscribed ? ESlateVisibility::HitTestInvisible
																: ESlateVisibility::Collapsed);
				}
				bCachedSubscriptionState = Subscribed;
			}
		}
	}
}

void UModioModDetailsView::NativeSubscribeClicked()
{
	SubscribeButton->SetIsEnabled(false);
	GWorld->GetTimerManager().SetTimer(SetFocusTimerHandle, this, &UModioModDetailsView::EnableSubscribeButton, 0.6, false);

	if (!bCachedSubscriptionState)
	{
		UModioModInfoUI* Data = Cast<UModioModInfoUI>(DataSource);
		if (Data)
		{
			UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>();
			if (Subsystem)
			{
				Subsystem->RequestSubscriptionForModID(Data->Underlying.ModId);
			}
		}
	}
	else
	{
		UModioModInfoUI* Data = Cast<UModioModInfoUI>(DataSource);
		if (Data)
		{
			UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>();
			if (Subsystem)
			{
				Subsystem->RequestRemoveSubscriptionForModID(Data->Underlying.ModId);
			}
		}
	}
}

void UModioModDetailsView::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (SubscribeButton)
	{
		SubscribeButton->SetLabel(SubscribeButtonFormat);
	}
	// this will need to pull values from the data source when we have one set

	if (RateUpButton)
	{
		RateUpButton->SetLabel(RateUpTextFormat);
	}
	if (RateDownButton)
	{
		RateDownButton->SetLabel(RateDownTextFormat);
	}
	if (ReportButton)
	{
		ReportButton->SetLabel(ReportTextFormat);
	}

#if WITH_EDITOR
	// This could likely be moved elsewhere, but as it is editor-only I'm not too worried
	if (EditorPreviewDataSource && IsDesignTime())
	{
		SetDataSource(EditorPreviewDataSource);
	}
#endif
}

void UModioModDetailsView::NativeDisplayModDetails(const TScriptInterface<class IModioModInfoUIDetails>& Details)
{
	// SetDataSource(NewDataSource);
}

void UModioModDetailsView::NativeDisplayModDetailsForId(const FModioModID& ModID)
{
	ShowDetailsForMod(ModID);
}

void UModioModDetailsView::NativeRequestOperationRetry()
{
	ShowDetailsForMod(CurrentModID);
}

FReply UModioModDetailsView::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FillScrollableWidgetsArray();

	if (ProcessCommandForEvent(InKeyEvent))
	{
		return FReply::Handled();
	}

	UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>();
	if (ImageGallery->bIsFocused)
	{
		if (GetCommandKeyForEvent(InKeyEvent) == FModioInputKeys::Down)
		{
			if (ScrollableWidgets.IsValidIndex(CurrentIndex + 1))
			{
				CurrentIndex += 1;
				ScrollBox->ScrollWidgetIntoView(ScrollableWidgets[CurrentIndex], true,
												EDescendantScrollDestination::Center);
			}
			return FReply::Handled();
		}

		if (GetCommandKeyForEvent(InKeyEvent) == FModioInputKeys::Up)
		{
			if (ScrollableWidgets.IsValidIndex(CurrentIndex - 1))
			{
				CurrentIndex -= 1;
				ScrollBox->ScrollWidgetIntoView(ScrollableWidgets[CurrentIndex], true,
												EDescendantScrollDestination::Center);
			}
			return FReply::Handled();
		}
	}

	if (GetCommandKeyForEvent(InKeyEvent) == FModioInputKeys::Next)
	{
		ImageGallery->ChangeImage(FModioInputKeys::Next);
		return FReply::Handled();
	}

	if (GetCommandKeyForEvent(InKeyEvent) == FModioInputKeys::Previous)
	{
		ImageGallery->ChangeImage(FModioInputKeys::Previous);
		return FReply::Handled();
	}

	if (GetCommandKeyForEvent(InKeyEvent) == FModioInputKeys::Subscribe && !InKeyEvent.IsRepeat() && SubscribeButton && SubscribeButton->GetIsEnabled())
	{
		NativeSubscribeClicked();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UModioModDetailsView::NativeOnAuthenticationChanged(TOptional<FModioUser> User)
{
	bIsUserAuthenticated = User.IsSet();
}

// These methods potentially need us to know about the user's ratings
void UModioModDetailsView::RateUpClicked()
{
	// to disable button spamming
	if (bRateUpProcess)
	{
		return;
	}

	UModioModInfoUI* ModInfo = Cast<UModioModInfoUI>(DataSource);
	if (ModInfo)
	{
		UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>();
		if (Subsystem)
		{
			// Needs additional payload param so we know which type of operation was completed
			Subsystem->RequestRateUpForModId(
				ModInfo->Underlying.ModId,
				FOnErrorOnlyDelegateFast::CreateUObject(this, &UModioModDetailsView::OnRatingSubmissionComplete,
														EModioRating::Positive));
			if (bIsUserAuthenticated)
			{
				bRateUpProcess = true;
				RateUpButton->SetIsEnabled(false);
			}
		}
	}
}

void UModioModDetailsView::RateDownClicked()
{
	if (bRateDownProcess)
	{
		return;
	}

	UModioModInfoUI* ModInfo = Cast<UModioModInfoUI>(DataSource);
	if (ModInfo)
	{
		UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>();
		if (Subsystem)
		{
			// Needs additional payload param so we know which type of operation was completed
			Subsystem->RequestRateDownForModId(
				ModInfo->Underlying.ModId,
				FOnErrorOnlyDelegateFast::CreateUObject(this, &UModioModDetailsView::OnRatingSubmissionComplete,
														EModioRating::Negative));
			if (bIsUserAuthenticated)
			{
				bRateDownProcess = true;
				RateDownButton->SetIsEnabled(false);
			}
		}
	}
}

void UModioModDetailsView::ReportClicked()
{
	UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>();
	if (Subsystem)
	{
		Subsystem->ShowModReportDialog(DataSource);
	}
}

void UModioModDetailsView::OnRatingSubmissionComplete(FModioErrorCode ec, EModioRating SubmittedRating)
{
	if (UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>())
	{
		Subsystem->DisplayErrorNotification(UModioNotificationParamsLibrary::CreateRatingNotification(ec, DataSource));
	}

	UModioModInfoUI* ModInfo = Cast<UModioModInfoUI>(DataSource);
	FFormatNamedArguments FormatArgs;

	if (!ec)
	{
		switch (SubmittedRating)
		{
			case EModioRating::Positive:
				RateUpButton->SetIsEnabled(true);
				bRateUpProcess = false;
				if (ModInfo)
				{
					FormatArgs.Add("RatingPositive", ModInfo->Underlying.Stats.RatingPositive);
					RateUpButton->SetLabel(FText::Format(RateUpTextFormat, FormatArgs));
				}
				break;
			case EModioRating::Negative:
				RateDownButton->SetIsEnabled(true);
				bRateDownProcess = false;
				if (ModInfo)
				{
					FormatArgs.Add("RatingNegative", ModInfo->Underlying.Stats.RatingNegative);
					RateDownButton->SetLabel(FText::Format(RateDownTextFormat, FormatArgs));
				}
				break;
			break;
			case EModioRating::Neutral:
				// ensure that neither button is highlighted
				break;
			default:
				break;
		}
	}
	else
	{
		GEngine->GetEngineSubsystem<UModioUISubsystem>()->DisplayErrorDialog(ec);
	}
}

void UModioModDetailsView::NativeOnSetDataSource()
{
	Super::NativeOnSetDataSource();
	UModioModInfoUI* ModInfo = Cast<UModioModInfoUI>(DataSource);
	if (ModInfo)
	{
		bCachedSubscriptionState = false;
		if (UModioSubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioSubsystem>())
		{
			// Can hit this function directly because the primary subsystem caches the subscriptions, so we dont
			// have to do it manually in the UI subsystem
			if (Subsystem->QueryUserSubscriptions().Contains(ModInfo->Underlying.ModId))
			{
				bCachedSubscriptionState = true;
			}
		}
		if (ModTitleTextBlock)
		{
			ModTitleTextBlock->SetText(FText::FromString(ModInfo->Underlying.ProfileName));
		}

		if (ModSummaryTextBlock)
		{
			ModSummaryTextBlock->SetText(FText::FromString(ModInfo->Underlying.ProfileSummary));
		}
		if (ModChangelog)
		{
			ModChangelog->SetText(FText::FromString(ModInfo->Underlying.FileInfo.Changelog));
		}

		if (SubscriptionBadge)
		{
			SubscriptionBadge->SetDataSource(DataSource);
		}

		if (ImageGallery)
		{
			ImageGallery->SetDataSource(DataSource);
			ImageGallery->SetVisibility(ESlateVisibility::Visible);
		}

		if (ModFullDescriptionTextBlock)
		{
			ModFullDescriptionTextBlock->SetText(FText::FromString(ModInfo->Underlying.ProfileDescriptionPlaintext));
		}

		if (ModTags)
		{
			ModTags->SetTags(ModInfo->Underlying.Tags);
		}

		FFormatNamedArguments FormatArgs;
		FormatArgs.Add("RatingPositive", ModInfo->Underlying.Stats.RatingPositive);
		FormatArgs.Add("RatingNegative", ModInfo->Underlying.Stats.RatingNegative);
		if (RateUpButton)
		{
			RateUpButton->SetLabel(FText::Format(RateUpTextFormat, FormatArgs));
		}
		if (RateDownButton)
		{
			RateDownButton->SetLabel(FText::Format(RateDownTextFormat, FormatArgs));
		}

		if (SubscribeButton)
		{
			if (bCachedSubscriptionState)
			{
				SubscribeButton->SetLabel(UnsubscribeLabel);
			}
			else
			{
				SubscribeButton->SetLabel(SubscribeLabel);
			}
		}

		if (ModPropertiesInspector)
		{
			ModPropertiesInspector->SetDataSource(DataSource);
		}
		if (ProgressWidget)
		{
			ProgressWidget->SetDataSource(DataSource);
		}
	}
}

void UModioModDetailsView::NativeOnModInfoRequestCompleted(FModioModID ModID, FModioErrorCode ec,
														   TOptional<FModioModInfo> Info)
{
	IModioUIModInfoReceiver::NativeOnModInfoRequestCompleted(ModID, ec, Info);

	if (CurrentModID == ModID)
	{
		if (!ec)
		{
			if (UModioModInfoUI* NewDataSource = NewObject<UModioModInfoUI>())
			{
				NewDataSource->Underlying = Info.GetValue();
				SetDataSource(NewDataSource);

				GWorld->GetTimerManager().SetTimer(SetFocusTimerHandle, this, &UModioModDetailsView::SetInitialFocus, 0.6, false);
			}
			IModioUIAsyncOperationWidget::Execute_NotifyOperationState(this,
																	   EModioUIAsyncOperationWidgetState::Success);
		}
		else
		{
			IModioUIAsyncOperationWidget::Execute_NotifyOperationState(this, EModioUIAsyncOperationWidgetState::Error);
			ModioErrorWithRetryWidget->RetryButton->SetKeyboardFocus();
		}
	}
}

void UModioModDetailsView::FillScrollableWidgetsArray()
{
	// The contents here can be easily adjusted for smoother scrolling - these seemed work fine for now
	ScrollableWidgets.Empty();
	ScrollableWidgets.Add(ImageGallery);
	ScrollableWidgets.Add(ModFullDescriptionTextBlock);
	ScrollableWidgets.Add(ModChangelog);
}

void UModioModDetailsView::SetInitialFocus()
{
	if (SubscribeButton->GetIsEnabled())
	{
		FSlateApplication::Get().SetUserFocus(0, SubscribeButton->TakeWidget(), EFocusCause::SetDirectly);
	}
	else
	{
		ImageGallery->SetKeyboardFocus();
	}
}

void UModioModDetailsView::OnDialogClosed()
{
	if (ModioErrorWithRetryWidget->IsVisible())
	{
		ModioErrorWithRetryWidget->SetKeyboardFocus();
		return;
	}

	UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>();
	if (IsValid(Subsystem))
	{
		if (Subsystem->GetCurrentFocusTarget() != SubscribeButton)
		{
			FSlateApplication::Get().SetUserFocus(0, SubscribeButton->TakeWidget(), EFocusCause::SetDirectly);
		}
	}
}

void UModioModDetailsView::OnDownloadQueueClosed()
{
	FSlateApplication::Get().SetUserFocus(0, SubscribeButton->TakeWidget(), EFocusCause::SetDirectly);
}

void UModioModDetailsView::EnableSubscribeButton()
{
	SubscribeButton->SetIsEnabled(true);
	SubscribeButton->SetKeyboardFocus();

	(SubscribeButton->GetLabel() == SubscribeLabel.ToString()) ? SubscribeButton->SetLabel(UnsubscribeLabel)
																   : SubscribeButton->SetLabel(SubscribeLabel);
}

void UModioModDetailsView::ShowDetailsForMod(FModioModID ID)
{
	CurrentModID = ID;
	if (UModioUISubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioUISubsystem>())
	{
		Subsystem->RequestModInfoForModIDs({ID});
		IModioUIAsyncOperationWidget::Execute_NotifyOperationState(this, EModioUIAsyncOperationWidgetState::InProgress);
	}
	if (ImageGallery)
	{
		ImageGallery->SetVisibility(ESlateVisibility::Collapsed);
	}

	UModioSubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioSubsystem>();
	if (IsValid(Subsystem))
	{
		TOptional<FModioModProgressInfo> CurrentProgress = Subsystem->QueryCurrentModUpdate();

		if (CurrentProgress)
		{
			(CurrentProgress->ID == CurrentModID)
				? ProgressWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible)
				: ProgressWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
			ProgressWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FReply UModioModDetailsView::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	SubscribeButton->SetKeyboardFocus();
	if (ModioErrorWithRetryWidget->IsVisible())
	{
		ModioErrorWithRetryWidget->SetKeyboardFocus();
	}
	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

#include "Loc/EndModioLocNamespace.h"