// Fill out your copyright notice in the Description page of Project Settings.


#include "VE_GardenMeshTools.h"
//#include "DynamicMeshBuilder.h"
//#include "DynamicMeshComponent.h"
#include "Async/Async.h"

// Sets default values for this component's properties
UVE_GardenMeshTools::UVE_GardenMeshTools()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UVE_GardenMeshTools::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UVE_GardenMeshTools::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

/*void EditGardenMeshHeightClosestToPlayerAsync(UDynamicMeshComponent* DynamicMesh, AActor* PlayerActor, float HeightOffset, float MaxDistance, float FalloffDistance, UDynamicMeshComponent*& OutputMesh)
    {
        if (!DynamicMesh || !PlayerActor)
        {
            OutputMesh = nullptr;
            return;
        }

        // Clone the original mesh so we can edit it asynchronously
        OutputMesh = DuplicateObject<UDynamicMeshComponent>(DynamicMesh, DynamicMesh->GetOuter());

        // Begin the asynchronous mesh editing operation
        FDynamicMeshAsyncOp* AsyncOp = new FDynamicMeshAsyncOp(OutputMesh);
        AsyncOp->Begin([=]() {

            TArray<FVector> Vertices;
            TArray<int32> Triangles;

            OutputMesh->GetSectionMesh(0, Vertices, Triangles);

            // Get the player location
            FVector PlayerLocation = PlayerActor->GetActorLocation();

            // Iterate through each vertex and adjust its height based on its proximity to the player
            for (int32 i = 0; i < Vertices.Num(); ++i)
            {
                FVector WorldVertex = OutputMesh->GetComponentTransform().TransformPosition(Vertices[i]);
                float DistanceToPlayer = FVector::Dist(WorldVertex, PlayerLocation);

                if (DistanceToPlayer <= MaxDistance)
                {
                    // Calculate the falloff based on the distance to the player
                    float Falloff = 1.0f;
                    if (DistanceToPlayer > FalloffDistance)
                    {
                        Falloff = FMath::Lerp(1.0f, 0.0f, (DistanceToPlayer - FalloffDistance) / (MaxDistance - FalloffDistance));
                    }

                    // Adjust the height of the vertex by the specified offset and falloff
                    WorldVertex.Z += HeightOffset * Falloff;

                    // Update the vertex in the mesh
                    Vertices[i] = OutputMesh->GetComponentTransform().InverseTransformPosition(WorldVertex);
                }
            }

            // Update the mesh section with the modified vertices
            OutputMesh->UpdateMeshSection(0, Vertices, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>());

            });

        AsyncOp->Finalize();
    }
    */