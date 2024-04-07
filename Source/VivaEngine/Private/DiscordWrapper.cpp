// Fill out your copyright notice in the Description page of Project Settings.

#include "DiscordWrapper.h"
#include "discord.h"

discord::Core* core{};

// Sets default values for this component's properties
UDiscordWrapper::UDiscordWrapper()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDiscordWrapper::SetDiscordActivity(FString State, FString Details, FString LargeImageName)
{
	// Make sure Discord is not required
	// We will need to hide this Token ID in something later
	auto result = discord::Core::Create(1030046546768711720, DiscordCreateFlags_NoRequireDiscord, &core);


	discord::Activity activity{};

	// Turn FString UE5 TCHAR into a Char array
	char* StateChar = TCHAR_TO_UTF8(*State);
	char* DetailsChar = TCHAR_TO_UTF8(*Details);
	char* LargeImageChar = TCHAR_TO_UTF8(*LargeImageName);

	// This will say: Playing Viva Pinata: RtP;\n In Lush Jungle
	activity.SetType(discord::ActivityType::Playing);
	activity.SetState(StateChar);
	activity.SetDetails(DetailsChar);
	activity.GetAssets().SetLargeImage(LargeImageChar);

	core->ActivityManager().UpdateActivity(activity, [](discord::Result result)
		{
			// Can do stuff here on error or success
		});
}

void UDiscordWrapper::ClearDiscordActivity()
{
	auto result = discord::Core::Create(1030046546768711720, DiscordCreateFlags_NoRequireDiscord, &core);

	core->ActivityManager().ClearActivity([](discord::Result result)
		{
			// Can do stuff here on error or success
		});
}


// Called when the game starts
void UDiscordWrapper::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UDiscordWrapper::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Required every tick (Per Discord SDK Docs)
	::core->RunCallbacks();
}

