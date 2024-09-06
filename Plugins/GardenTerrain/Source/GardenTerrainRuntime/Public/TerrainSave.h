#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TerrainSave.generated.h"

USTRUCT(BlueprintType)
struct FDeconstructedTerrainSave {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<AActor*> Trash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UTextureRenderTarget*> RenderTargets;

};

USTRUCT(BlueprintType)
struct FDeconstructedTrash {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> TrashClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer Tags;

};

 
USTRUCT(BlueprintType)
struct FConstructedTerrainSave {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDeconstructedTrash> Trash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> RenderTargetPaths;

};

UCLASS(BlueprintType)
class GARDENTERRAINRUNTIME_API UTerrainSave : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FConstructedTerrainSave Save;
};