// Fill out your copyright notice in the Description page of Project Settings.


#include "GardenMesh.h"
#include "CoreMinimal.h"
#include "Engine.h"
//#include "Landscape.h"
//#include "LandscapeEdit.h"


// Sets default values
AGardenMesh::AGardenMesh()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGardenMesh::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGardenMesh::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

