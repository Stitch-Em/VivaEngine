// Fill out your copyright notice in the Description page of Project Settings.

#include "DiamondSquare.h"
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
	ProceduralMesh->ClearAllMeshSections();
	Vertices.Empty();
	Triangles.Empty();
	UV0.Empty();

	CreateVertices();
	CreateTriangles();

	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, TArray<FVector>(), UV0, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
	ProceduralMesh->SetMaterial(0, Material);
}


//a funtion that changes the hight of the mesh based off of x,y location Height multiplier and falloff
void ADiamondSquare::ChangeHeight(float X, float Y, float HeightMultiplier, float DistanceMulti, float Falloff)
{
	for (int i = 0; i < Vertices.Num(); ++i)
	{
		float Distance = FVector2D::Distance(FVector2D(X, Y), FVector2D(Vertices[i].X, Vertices[i].Y) * DistanceMulti);
		Distance = FMath::Clamp(Distance, 0.0f, 1.0f);
		Distance = 1 - Distance;
		float Height = 0;

		if (HeightCurve)
		{
			 Height = HeightMultiplier * HeightCurve->GetFloatValue(Distance);
		}
		else
		{
			 Height = HeightMultiplier * 1 - Distance;
		}
		
		Vertices[i].Z += Height;
	}
	
	
    


	ProceduralMesh->UpdateMeshSection(0, Vertices, TArray<FVector>(), UV0, TArray<FColor>(), TArray<FProcMeshTangent>());
}

