#pragma once
#include "GameFramework/Actor.h"
#include "TerrainEditorStuff.h"
#include "TerrainSection.generated.h"

class URuntimeMeshComponent;
class UProceduralMeshComponent;
class ATerrainGenerator;

UCLASS()
class GARDENTERRAINRUNTIME_API ATerrainSection : public AActor
{
	GENERATED_BODY()
	
public:	
	ATerrainSection();
	void InitializeOnSpawn(int32 SectionIndex, ATerrainGenerator* Terrain);
	void CreateSection();
	void UpdateSection();

	FVector SectionCoordinates;
	FVector CenterLocation;

	UFUNCTION(BlueprintCallable, Category = "ProceduralMeshGeneration")
	void RequestSculpting(const FSculptSettings& Settings, const FSculptInputInfo& InputInfo);

private:
	virtual void BeginPlay() override;
	void SetVisibility();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UProceduralMeshComponent* ProceduralMeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UProceduralMeshComponent* ProceduralMeshComponentLOD1 = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UProceduralMeshComponent* ProceduralMeshComponentLOD2 = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UProceduralMeshComponent* ProceduralMeshComponentLOD3 = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	UProceduralMeshComponent* ProceduralMeshComponentLOD4 = nullptr;

private:
	TArray<UProceduralMeshComponent*> ProceduralMeshLODs;


	APlayerController* PlayerControllerReference = nullptr;

	int32 SectionIndexLocal = 0;

	TArray<int32> IndexBufferLocal;

	FTimerHandle VisibilityTimerHandle;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sculpting")
	ATerrainGenerator* OwningTerrain = nullptr;
};
