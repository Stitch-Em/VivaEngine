// Fill out your copyright notice in the Description page of Project Settings.

#include "DiamondSquare.h"
//include the header file for UKismetProceduralMeshLibrary
#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"

ADiamondSquare::ADiamondSquare()
{
	PrimaryActorTick.bCanEverTick = false;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMesh");
	ProceduralMesh->SetupAttachment(GetRootComponent());

}

void ADiamondSquare::BeginPlay()
{
	Super::BeginPlay();
	
	Regenerate();

}

void ADiamondSquare::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADiamondSquare::CreateVertices()
{
	for (int X = 0; X <= XSize; ++X)
	{
		for (int Y = 0; Y <= YSize; ++Y)
		{
			float Z = FMath::PerlinNoise2D(FVector2D(X * NoiseScale + 0.1, Y * NoiseScale + 0.1)) * ZMultiplier;
			Vertices.Add(FVector(X * Scale, Y * Scale, Z));
			UV0.Add(FVector2D(X * UVScale, Y * UVScale));
			
			
		}
	}
}

void ADiamondSquare::CreateTriangles()
{
	int Vertex = 0;

	for (int X = 0; X < XSize; ++X)
	{
		for (int Y = 0; Y < YSize; ++Y)
		{
			Triangles.Add(Vertex);//Bottom left corner
			Triangles.Add(Vertex + 1);//Bottom right corner
			Triangles.Add(Vertex + YSize + 1);//Top left corner
			Triangles.Add(Vertex + 1);//Bottom right corner
			Triangles.Add(Vertex + YSize + 2);//Top right corner
			Triangles.Add(Vertex + YSize + 1);//Top left corner

			++Vertex;
		}
		++Vertex;
	}
}



//a function that deletes the mesh and creates a new one
void ADiamondSquare::Regenerate()
{
	FDateTime StartTime = FDateTime::UtcNow();

	ProceduralMesh->ClearAllMeshSections();
	Vertices.Empty();
	Triangles.Empty();
	UV0.Empty();

	CreateVertices();
	CreateTriangles();

	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, TArray<FVector>(), UV0, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
	ProceduralMesh->SetMaterial(0, Material);

	float TimeElapsedInMs = (FDateTime::UtcNow() - StartTime).GetTotalMilliseconds();
	UE_LOG(LogTemp, Display, TEXT("%f"), TimeElapsedInMs);
	//log to UE_Screen 
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Regenerate: %f"), TimeElapsedInMs));

	CalculateNormals();
}


//a funtion that changes the hight of the mesh based off of x,y location Height multiplier and falloff
void ADiamondSquare::ChangeHeight(float X, float Y, float HeightMultiplier, float DistanceMulti, float Falloff)
{
	FDateTime StartTime = FDateTime::UtcNow();

	for (int i = 0; i < Vertices.Num(); ++i)
	{
		

		float VX = GetActorLocation().X + Vertices[i].X;
		float VY = GetActorLocation().Y + Vertices[i].Y;

		float Distance = FVector2D::Distance(FVector2D(X, Y), FVector2D(VX, VY));

		//Use FMath to remap the distance using 0 to the distancemulti and 1 to 0
		float RemapedDistance = FMath::GetMappedRangeValueClamped(FVector2D(0, DistanceMulti), FVector2D(1, 0), Distance);

		float RemapedDistanceClamped = FMath::Clamp(RemapedDistance, 0.0f, 1.0f);
		float CurvedDistance = HeightCurve->GetFloatValue(RemapedDistanceClamped);


		float Height = 0;

		if (HeightCurve)
		{
			 Height = HeightMultiplier * CurvedDistance;
		}
		else
		{
			 Height = HeightMultiplier * 1 - RemapedDistanceClamped;
		}
		
		Vertices[i].Z += Height;
	}
	
	
    


	ProceduralMesh->UpdateMeshSection(0, Vertices, TArray<FVector>(), UV0, TArray<FColor>(), TArray<FProcMeshTangent>());

	float TimeElapsedInMs = (FDateTime::UtcNow() - StartTime).GetTotalMilliseconds();
	UE_LOG(LogTemp, Display, TEXT("%f"), TimeElapsedInMs);
	//log to UE_Screen 
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Mesh: %f"), TimeElapsedInMs));

	CalculateNormals();
	
}




//make a replacement for calculatinng the normals that dosnt requare the use of CalculateTangentsForMesh and is faster
void ADiamondSquare::CalculateNormals()
{
	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;
	Normals.Init(FVector(0, 0, 0), Vertices.Num());
	Tangents.Init(FProcMeshTangent(0, 0, 0), Vertices.Num());
	for (int i = 0; i < Triangles.Num(); i += 3)
	{
		int32 i0 = Triangles[i];
		int32 i1 = Triangles[i + 1];
		int32 i2 = Triangles[i + 2];

		FVector v1 = Vertices[i1] - Vertices[i0];
		FVector v2 = Vertices[i2] - Vertices[i0];
		FVector faceNormal = (v1 ^ v2).GetSafeNormal();

		Normals[i0] += faceNormal;
		Normals[i1] += faceNormal;
		Normals[i2] += faceNormal;
	}

	for (int i = 0; i < Normals.Num(); ++i)
	{
		Normals[i].Normalize();
	}
	//invert normals
	for (int i = 0; i < Normals.Num(); ++i)
	{
		Normals[i] = -Normals[i];
	}

	ProceduralMesh->UpdateMeshSection(0, Vertices, Normals, UV0, TArray<FColor>(), TArray<FProcMeshTangent>());
}
