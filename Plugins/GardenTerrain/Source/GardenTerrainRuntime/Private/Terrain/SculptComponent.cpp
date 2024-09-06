#include "Terrain/SculptComponent.h"
#include "Terrain/TerrainSection.h"


USculptComponent::USculptComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void USculptComponent::SculptStart()
{
	InputInfo.SculptInput = ESculptInput::ST_Started;
	InputInfo.StartLocation = HitResultOwner.Location;
	GetWorld()->GetTimerManager().SetTimer(SculptTimerHandle, this, &USculptComponent::Sculpt, GetWorld()->DeltaTimeSeconds, true);
}


void USculptComponent::SculptStop()
{
	InputInfo.SculptInput = ESculptInput::ST_Stopped;
	GetWorld()->GetTimerManager().PauseTimer(SculptTimerHandle);
	Sculpt();
}


bool USculptComponent::SculptSingle(FSculptInputInfo _InputInfo)
{
	// Cast to owner of hit section
	ATerrainSection* HitSection = dynamic_cast<ATerrainSection*>(HitResultOwner.GetActor());
	if (!HitSection) { return false; }

	HitSection->RequestSculpting(SculptSettings, _InputInfo);
	return true;
}


void USculptComponent::Sculpt()
{
	InputInfo.CurrentLocation = HitResultOwner.Location;
	if (InSleepDistance()) { return; }
	if (InputInfo.SculptInput == ESculptInput::ST_Stopped) { return; }

	// Cast to owner of hit section
	ATerrainSection* HitSection = dynamic_cast<ATerrainSection*>(HitResultOwner.GetActor());
	if (!HitSection) { return; }

	HitSection->RequestSculpting(SculptSettings, InputInfo);
	if (InputInfo.SculptInput == ESculptInput::ST_Started) { InputInfo.SculptInput = ESculptInput::ST_Ongoing; }
}


bool USculptComponent::InSleepDistance()
{
	if (FVector::Dist(InputInfo.LastLocation, InputInfo.CurrentLocation) < SleepDistance) { return true; }
	InputInfo.LastLocation = InputInfo.CurrentLocation;
	return false;
}