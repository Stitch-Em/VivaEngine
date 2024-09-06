#include "Terrain/TerrainGenerator.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetProceduralMeshLibrary.h"
#include "SimplexNoiseBPLibrary.h"
#include "Terrain/TerrainSection.h"


ATerrainGenerator::ATerrainGenerator()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	LODProperties.Add(&SectionProperties);
	LODProperties.Add(&SectionPropertiesLOD1);
	LODProperties.Add(&SectionPropertiesLOD2);
	LODProperties.Add(&SectionPropertiesLOD3);
	LODProperties.Add(&SectionPropertiesLOD4);
}


void ATerrainGenerator::BeginPlay()
{
	Super::BeginPlay();
	USimplexNoiseBPLibrary::setNoiseSeed(Seed);
	SetLODVisibility();
	bUseTimerforGeneration ? GenerateMeshTimed() : GenerateMesh();
}

void ATerrainGenerator::SetLODVisibility()
{
	float SectionLength = (SectionXY - 1) * QuadSize;
	//VisibilityLOD0 = SectionLength * 1.5;
	//VisibilityLOD1 = SectionLength * 2.5;
	//VisibilityLOD2 = SectionLength * 3.5;
	//VisibilityLOD3 = SectionLength * 4.5;
	//VisibilityLOD4 = SectionLength * 10.5;
}


void ATerrainGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Warning, TEXT("ENDPLAY Allocated: %i"), GlobalVertexData.GetAllocatedSize()/1000000);

	for (FSectionProperties* Iter : LODProperties)
	{
		Iter->Vertices.Empty();
		Iter->UV.Empty();
		Iter->Normals.Empty();
		Iter->VertexColors.Empty();
		Iter->SectionPosition.Empty();
		Iter->Triangles.Empty();
	}
	GlobalVertexData.Empty();
	UE_LOG(LogTemp, Warning, TEXT("ENDPLAY Allocated: %i"), GlobalVertexData.GetAllocatedSize() / 1000000);

}



void ATerrainGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// check if any sections request to be updated via queue
	if (SectionUpdateQueue.Num() > 0 && bAllowedToUpdateSection)
	{
		bAllowedToUpdateSection = false;
		if (!SectionActors.IsValidIndex(SectionUpdateQueue[0])) { return; }
		SectionActors[SectionUpdateQueue[0]]->UpdateSection();
	}
}


void ATerrainGenerator::GenerateMesh()
{
	// Main Function (freezes gamethread with big terrains)
	InitializeProperties();
	FillGlobalVertexData();
	FillIndexBuffer();
	SpawnSectionActors();
}


void ATerrainGenerator::GenerateMeshTimed()
{
	// Main Function alternative (no gamethread freezing but takes some time too)
	InitializeProperties();
	FillGlobalVertexDataTimed(); // TODO rename
}


void ATerrainGenerator::InitializeProperties()
{
	int32 ArraySizeGlobal = SectionXY * SectionXY * ComponentXY * ComponentXY;
	IndexBuffer.SetNum(ArraySizeGlobal, true);

	int32 NumOfSections = ComponentXY * ComponentXY;
	SectionActors.SetNum(NumOfSections, true);

	// Init GlobalProperties
	int32 MeshVertsPerSide = SectionXY * ComponentXY - (ComponentXY - 1);
	int32 TotalNumOfVerts = MeshVertsPerSide * MeshVertsPerSide;
	GlobalVertexData.SetNum(TotalNumOfVerts, true);

	// Init SectionProperties
	int32 SectionNumVerts = SectionXY * SectionXY;
	SectionProperties.Vertices.SetNum(SectionNumVerts, true);
	SectionProperties.UV.SetNum(SectionNumVerts, true);
	SectionProperties.Normals.SetNum(SectionNumVerts, true);
	SectionProperties.VertexColors.SetNum(SectionNumVerts, true);
	SectionProperties.SectionPosition.SetNum(SectionNumVerts, true);
	UKismetProceduralMeshLibrary::CreateGridMeshTriangles(SectionXY, SectionXY, false, OUT SectionProperties.Triangles);
	AddBorderVerticesToSectionProperties();

	// Init SectionPropertiesLOD1
	int32 SectionXYLOD1 = ((SectionXY - 1) / FactorLOD1) + 1;
	int32 LOD1NumVerts = SectionXYLOD1 * SectionXYLOD1;
	
	SectionPropertiesLOD1.Vertices.SetNum(LOD1NumVerts, true);
	SectionPropertiesLOD1.UV.SetNum(LOD1NumVerts, true);
	SectionPropertiesLOD1.Normals.SetNum(LOD1NumVerts, true);
	SectionPropertiesLOD1.VertexColors.SetNum(LOD1NumVerts, true);
	UKismetProceduralMeshLibrary::CreateGridMeshTriangles(SectionXYLOD1, SectionXYLOD1, false, OUT SectionPropertiesLOD1.Triangles);

	// Init SectionPropertiesLOD2
	int32 SectionXYLOD2 = ((SectionXY - 1) / FactorLOD2) + 1;
	int32 LOD2NumVerts = SectionXYLOD2 * SectionXYLOD2;

	SectionPropertiesLOD2.Vertices.SetNum(LOD2NumVerts, true);
	SectionPropertiesLOD2.UV.SetNum(LOD2NumVerts, true);
	SectionPropertiesLOD2.Normals.SetNum(LOD2NumVerts, true);
	SectionPropertiesLOD2.VertexColors.SetNum(LOD2NumVerts, true);
	UKismetProceduralMeshLibrary::CreateGridMeshTriangles(SectionXYLOD2, SectionXYLOD2, false, OUT SectionPropertiesLOD2.Triangles);

	// Init SectionPropertiesLOD3
	int32 SectionXYLOD3 = ((SectionXY - 1) / FactorLOD3) + 1;
	int32 LOD3NumVerts = SectionXYLOD3 * SectionXYLOD3;

	SectionPropertiesLOD3.Vertices.SetNum(LOD3NumVerts, true);
	SectionPropertiesLOD3.UV.SetNum(LOD3NumVerts, true);
	SectionPropertiesLOD3.Normals.SetNum(LOD3NumVerts, true);
	SectionPropertiesLOD3.VertexColors.SetNum(LOD3NumVerts, true);
	UKismetProceduralMeshLibrary::CreateGridMeshTriangles(SectionXYLOD3, SectionXYLOD3, false, OUT SectionPropertiesLOD3.Triangles);

	// Init SectionPropertiesLOD4
	int32 SectionXYLOD4 = ((SectionXY - 1) / FactorLOD4) + 1;
	int32 LOD4NumVerts = SectionXYLOD4 * SectionXYLOD4;

	SectionPropertiesLOD4.Vertices.SetNum(LOD4NumVerts, true);
	SectionPropertiesLOD4.UV.SetNum(LOD4NumVerts, true);
	SectionPropertiesLOD4.Normals.SetNum(LOD4NumVerts, true);
	SectionPropertiesLOD4.VertexColors.SetNum(LOD4NumVerts, true);
	UKismetProceduralMeshLibrary::CreateGridMeshTriangles(SectionXYLOD4, SectionXYLOD4, false, OUT SectionPropertiesLOD4.Triangles);

	UE_LOG(LogTemp, Warning, TEXT(" %i %i %i %i %i"), SectionXY, SectionXYLOD1, SectionXYLOD2, SectionXYLOD3, SectionXYLOD4);
}


void ATerrainGenerator::FillIndexBuffer()
{
	int32 ArraySizeGlobal = SectionXY * SectionXY * ComponentXY * ComponentXY;
	IndexBuffer.SetNum(ArraySizeGlobal, true);

	int32 Iterator = 0;
	int32 QuadsPerSide = SectionXY - 1;
	int32 GlobalXYVerts = QuadsPerSide * ComponentXY + 1;
	for (int XComp = 0; XComp < ComponentXY; XComp++)
	{
		for (int YComp = 0; YComp < ComponentXY; YComp++)
		{
			int32 SectionRoot = ((GlobalXYVerts * QuadsPerSide) * XComp) + (QuadsPerSide * YComp);
			for (int XSection = 0; XSection < SectionXY; XSection++)
			{
				for (int YSection = 0; YSection < SectionXY; YSection++)
				{
					int32 IndexToAdd = GlobalXYVerts * XSection + YSection;
					int32 IndexTotal = SectionRoot + IndexToAdd;
					IndexBuffer[Iterator] = IndexTotal;
					Iterator++;
				}
			}
		}
	}
}


void ATerrainGenerator::FillIndexBufferTimed()
{
	// Fill Index Buffer (Partially, then spawn coresponding section)
	int32 XComp = SectionIndexIter / ComponentXY;
	int32 YComp = SectionIndexIter % ComponentXY;

	int32 QuadsPerSide = SectionXY - 1;
	int32 GlobalXYVerts = QuadsPerSide * ComponentXY + 1;
	int32 SectionRoot = ((GlobalXYVerts * QuadsPerSide) * XComp) + (QuadsPerSide * YComp);
	for (int XSection = 0; XSection < SectionXY; XSection++)
	{
		for (int YSection = 0; YSection < SectionXY; YSection++)
		{
			int32 IndexToAdd = GlobalXYVerts * XSection + YSection;
			int32 IndexTotal = SectionRoot + IndexToAdd;
			IndexBuffer[IndexBufferIter] = IndexTotal;
			IndexBufferIter++;
		}
	}

	// Spawn TerrainSectionActor
	SectionActors[SectionIndexIter] = GetWorld()->SpawnActor<ATerrainSection>(
		ClassToSpawnAsSection,
		GetActorLocation(),
		GetActorRotation());
	SectionActors[SectionIndexIter]->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	SectionActors[SectionIndexIter]->InitializeOnSpawn(SectionIndexIter, this);

	// Create Section
	SectionActors[SectionIndexIter]->CreateSection();

	// Recursive function call with a timer to prevent freezing of the gamethread
	SectionIndexIter++;
	if (SectionIndexIter >= ComponentXY * ComponentXY) { return; }
	GetWorld()->GetTimerManager().SetTimer(SectionCreateTimerHandle, this, &ATerrainGenerator::FillIndexBufferTimed, CreateSectionTimerDelay, false);
}


void ATerrainGenerator::AddBorderVerticesToSectionProperties()
{
	for (int32 X = 0; X < SectionXY; X++)
	{
		for (int32 Y = 0; Y < SectionXY; Y++)
		{
			int32 i = X * SectionXY + Y;
			FVector2D Ratio = FVector2D(X, Y) / FVector2D(SectionXY - 1, SectionXY - 1);
			ESectionPosition VertPositionInsideSection = ESectionPosition::SB_NotOnBorder;

			if (Ratio.Equals(FVector2D(0, 0))) { VertPositionInsideSection = ESectionPosition::SB_EdgeBottomLeft; }
			if (Ratio.Equals(FVector2D(1, 0))) { VertPositionInsideSection = ESectionPosition::SB_EdgeTopLeft; }
			if (Ratio.Equals(FVector2D(0, 1))) { VertPositionInsideSection = ESectionPosition::SB_EdgeBottomRight; }
			if (Ratio.Equals(FVector2D(1, 1))) { VertPositionInsideSection = ESectionPosition::SB_EdgeTopRight; }

			if (VertPositionInsideSection == ESectionPosition::SB_NotOnBorder)
			{
				if (FMath::IsNearlyEqual(Ratio.X, 0)) { VertPositionInsideSection = ESectionPosition::SB_BorderBottom; }
				if (FMath::IsNearlyEqual(Ratio.X, 1)) { VertPositionInsideSection = ESectionPosition::SB_BorderTop; }
				if (FMath::IsNearlyEqual(Ratio.Y, 0)) { VertPositionInsideSection = ESectionPosition::SB_BorderLeft; }
				if (FMath::IsNearlyEqual(Ratio.Y, 1)) { VertPositionInsideSection = ESectionPosition::SB_BorderRight; }
			}

			SectionProperties.SectionPosition[i] = VertPositionInsideSection;
		}
	}
}


void ATerrainGenerator::FillGlobalVertexData()
{
	int32 i = 0;
	int32 VertsPerSide = (ComponentXY * SectionXY - (ComponentXY - 1));
	for (int32 X = 0; X < VertsPerSide; X++)
	{
		for (int32 Y = 0; Y < VertsPerSide; Y++)
		{
			FVector VertCoords;
			FVector Normal = FVector(0.f, 0.f, 1.f);
			switch (TerrainZ)
			{
			case ETerrainGeneration::TG_Flat:
				VertCoords = FVector(X * QuadSize, Y * QuadSize, 0.f);
				Normal = FVector(0.f, 0.f, 1.f);
				break;

			case ETerrainGeneration::TG_Noise:
				VertCoords = FVector(X * QuadSize, Y * QuadSize, GetHeightByNoise(X, Y));
				Normal = CalculateVertexNormalByNoise(VertCoords / QuadSize);
				break;

			case ETerrainGeneration::TG_LineTrace:
				VertCoords = FVector(X * QuadSize, Y * QuadSize, 0.f);
				CopyLandscapeHeightBelow(OUT VertCoords, OUT Normal);
				break;
			}

			// Set Vertex, UV, Normal
			GlobalVertexData[i].Vertices = VertCoords;
			GlobalVertexData[i].UV = FVector2D(X, Y);
			GlobalVertexData[i].Normals = Normal;
			i++;
		}
	}
	if (TerrainZ == ETerrainGeneration::TG_LineTrace)
	{
		FillGlobalNormals();
	}
}


void ATerrainGenerator::FillGlobalNormals()
{
	for (int32 i = 0; i < GlobalVertexData.Num(); i++)
	{
		GlobalVertexData[i].Normals = CalculateVertexNormal(i);
	}
}


void ATerrainGenerator::FillGlobalVertexDataTimed()
{
	int32 MeshVertsPerSide = SectionXY * ComponentXY - (ComponentXY - 1);
	int32 TotalNumOfVerts = MeshVertsPerSide * MeshVertsPerSide;

	// Get GlobalProperties Vertex & UV Coordinates
	for (int i = 0; i < MeshVertsPerSide; i++)
	{
		int32 X = GlobalXIter;
		int32 Y = i;
		int32 Index = X * MeshVertsPerSide + Y;
		if (!GlobalVertexData.IsValidIndex(Index)) { UE_LOG(LogTemp, Error, TEXT("IndexNotValid")); }

		FVector VertCoords;
		FVector Normal = FVector(0.f, 0.f, 1.f);
		switch (TerrainZ)
		{
		case ETerrainGeneration::TG_Flat:
			VertCoords = FVector(X * QuadSize, Y * QuadSize, 0.f);
			Normal = FVector(0.f, 0.f, 1.f);
			break;

		case ETerrainGeneration::TG_Noise:
			VertCoords = FVector(X * QuadSize, Y * QuadSize, GetHeightByNoise(X, Y));
			Normal = CalculateVertexNormalByNoise(VertCoords / QuadSize);
			break;

		case ETerrainGeneration::TG_LineTrace:
			VertCoords = FVector(X * QuadSize, Y * QuadSize, 0.f);
			CopyLandscapeHeightBelow(OUT VertCoords, OUT Normal);
			break;
		}

		// Set Vertex, UV, Normal
		GlobalVertexData[Index].Vertices = VertCoords;
		GlobalVertexData[Index].UV = FVector2D(X, Y);
		GlobalVertexData[Index].Normals = Normal;
	}

	UE_LOG(LogTemp, Warning, TEXT("GlobalPropertiesPercentFilled: %f % "), ((float)GlobalXIter/ (float)MeshVertsPerSide) * 100.f);
	GlobalXIter++;

	// execute following when GlobalVertexData is filled
	if (GlobalXIter + 1 > MeshVertsPerSide)
	{
		//if (TerrainZ == ETerrainGeneration::TG_LineTrace) { FillGlobalNormals(); }
		FillIndexBufferTimed(); return;
	}

	// Recursive function call with a timer to prevent freezing of the gamethread
	GetWorld()->GetTimerManager().SetTimer(SectionCreateTimerHandle, this, &ATerrainGenerator::FillGlobalVertexDataTimed, FillVertexDataTimerDelay,false);
}


void ATerrainGenerator::CopyLandscapeHeightBelow(FVector &Coordinates, FVector& Normal)
{
	FHitResult Hit;
	FVector Start = Coordinates + GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, LineTraceLength);
	
	GetWorld()->LineTraceSingleByChannel(OUT Hit, Start, End, ECollisionChannel::ECC_WorldStatic);

	float LineTraceHeight = Hit.Location.Z - GetActorLocation().Z + LineTraceHeightOffset;
	Coordinates = FVector(Coordinates.X, Coordinates.Y, LineTraceHeight);
	Normal = (Hit.bBlockingHit) ? Normal = Hit.Normal : Normal = FVector(0, 0, 1);
}


void ATerrainGenerator::SpawnSectionActors()
{
	if (!ClassToSpawnAsSection) { UE_LOG(LogTemp, Error, TEXT("ClassToSpawnAsSection Not Set in TerrainGenerator BP")); return; }

	// Iterate through amount of Components/Sections
	for (int32 X = 0; X < ComponentXY; X++)
	{
		for (int32 Y = 0; Y < ComponentXY; Y++)
		{
			// Spawn the SectionActor & Attach to this
			int32 SectionIndex = X * ComponentXY + Y;
			SectionActors[SectionIndex] = GetWorld()->SpawnActor<ATerrainSection>(
				ClassToSpawnAsSection,
				GetActorLocation(),
				GetActorRotation());
			SectionActors[SectionIndex]->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
			SectionActors[SectionIndex]->InitializeOnSpawn(SectionIndex, this);

			// Fill SectionProperties and create section inside SectionActor
			SectionActors[SectionIndex]->CreateSection();
		}
	}
}


void ATerrainGenerator::FillSectionVertStructLOD(int32 SectionIndex)
{
	int32 IndexStart = SectionXY * SectionXY * SectionIndex;
	int32 IndexEnd = IndexStart + (SectionXY * SectionXY);

	int32 L1 = 0;
	int32 L2 = 0;
	int32 L3 = 0;
	int32 L4 = 0;

	for (int i = 0; i + IndexStart < IndexEnd; i++)
	{
		if (!SectionProperties.Vertices.IsValidIndex(i)) { return; }
		if (!IndexBuffer.IsValidIndex(i + IndexStart)) { return; }

		int32 Index = IndexBuffer[i + IndexStart];

		FVertexData IterVertexData;
		IterVertexData.Vertices		= GlobalVertexData[Index].Vertices;
		IterVertexData.Normals		= GlobalVertexData[Index].Normals;
		IterVertexData.UV			= GlobalVertexData[Index].UV;
		IterVertexData.VertexColor	= GlobalVertexData[Index].VertexColor;

		// Set Vertex Data of all LODs
		SetLODVertexData(0, i, i, 1, IterVertexData);
		if (SetLODVertexData(1, i, L1, FactorLOD1, IterVertexData)) { L1++; }
		if (SetLODVertexData(2, i, L2, FactorLOD2, IterVertexData)) { L2++; }
		if (SetLODVertexData(3, i, L3, FactorLOD3, IterVertexData)) { L3++; }
		if (SetLODVertexData(4, i, L4, FactorLOD4, IterVertexData)) { L4++; }
	}
}



bool ATerrainGenerator::SetLODVertexData(int32 LOD, int32 LoopIter, int32 Index, int32 DivideFactor, FVertexData VertexData)
{
	if (LoopIter % DivideFactor == 0 && (LoopIter / SectionXY) % DivideFactor == 0)
	{
		if (!LODProperties[LOD]->Vertices.IsValidIndex(Index)) { return false; }
		LODProperties[LOD]->Vertices[Index]		= VertexData.Vertices;
		LODProperties[LOD]->Normals[Index]		= VertexData.Normals;
		LODProperties[LOD]->UV[Index]			= VertexData.UV;
		LODProperties[LOD]->VertexColors[Index]	= VertexData.VertexColor;
		return true;
	}
	return false;
}


void ATerrainGenerator::SectionRequestsUpdate(int32 SectionIndex, const FSculptSettings& Settings, const FSculptInputInfo& InputInfo)
{
	ModifyTerrain(SectionIndex, Settings, InputInfo);
}


void ATerrainGenerator::ModifyTerrain(int32 SectionIndex, const FSculptSettings& Settings, const FSculptInputInfo& InputInfo)
{	
	// TODO move all code for sculpting/ updating in seperate class and only keep what's necessary to create section
	TArray<int32> AffectedSections;
	TArray<int32> AffectedVertNormals;
	FVector RelativeHitLocation = (InputInfo.CurrentLocation - GetActorLocation());
	FVector CenterCoordinates = FVector(FMath::RoundToInt(RelativeHitLocation.X / QuadSize), FMath::RoundToInt(RelativeHitLocation.Y / QuadSize), 0);
	int32 VertsPerSide = ((SectionXY - 1) * ComponentXY + 1);
	int32 CenterIndex = CenterCoordinates.X * VertsPerSide + CenterCoordinates.Y;

	// Modify Verts around given radius
	int32 RadiusInVerts = Settings.SculptRadius / QuadSize;
	int32 RadiusExtended = RadiusInVerts + 1;

	for (int32 X = -RadiusExtended; X <= RadiusExtended; X++)
	{
		for (int32 Y = -RadiusExtended; Y <= RadiusExtended; Y++)
		{
			// Continue loop if Vert doesn't exist
			int32 CurrentIndex = CenterIndex + (X * VertsPerSide) + Y;
			if (!GlobalVertexData.IsValidIndex(CurrentIndex)) { continue; }

			FVector CurrentVertCoords = FVector(
				FMath::RoundToInt(GlobalVertexData[CurrentIndex].Vertices.X / QuadSize),
				FMath::RoundToInt(GlobalVertexData[CurrentIndex].Vertices.Y / QuadSize),
				0);
			float DistanceFromCenter = FVector::Dist(CenterCoordinates, CurrentVertCoords);

			// affected normals are added to array, and calculated after loop
			if (DistanceFromCenter > RadiusExtended) { /*CalculateVertexNormal(CurrentIndex);*/ continue; }
			AffectedVertNormals.Add(CurrentIndex);

			// Check real radius
			if (DistanceFromCenter > RadiusInVerts) { /*CalculateVertexNormal(CurrentIndex);*/ continue; }


			// execute selected sculpt function for each vertex in radius
			if (InputInfo.SculptInput != ESculptInput::ST_Stopped)
			{
				float DistanceFraction = DistanceFromCenter / RadiusInVerts;
				switch (Settings.SculptMode)
				{
				case ESculptMode::ST_Sculpt:
					VertexChangeHeight(CurrentIndex, DistanceFraction, Settings);
					break;

				case ESculptMode::ST_Flatten:
					VertexFlatten(CurrentIndex, DistanceFraction, Settings, InputInfo);
					break;

				case ESculptMode::ST_Paint:
					VertexPaint(CurrentIndex, DistanceFraction, Settings);
					break;

				case ESculptMode::ST_Noise:
					VertexAddNoise(CurrentIndex, DistanceFraction, Settings);
					break;

				case ESculptMode::ST_Smooth:
					VertexSmooth(CurrentIndex, DistanceFraction, Settings);
					break;
				}
			}

			FVector SectionCoordinates = FVector(SectionIndex / (ComponentXY), SectionIndex % (ComponentXY), 0);

			FVector SectionVertCoords = CurrentVertCoords - (SectionCoordinates * SectionXY - SectionCoordinates);
			int32 SectionVertIndex = SectionVertCoords.X * SectionXY + SectionVertCoords.Y;

			if (SectionVertCoords.X > SectionXY - 1 || SectionVertCoords.X < 0 || SectionVertCoords.Y > SectionXY - 1 || SectionVertCoords.Y < 0) { continue; }
			if (!SectionProperties.SectionPosition.IsValidIndex(SectionVertIndex)) { continue; }

			AddAffectedSections(SectionIndex, SectionVertIndex, OUT AffectedSections);
		}
	}

	// Update affected Normals
	for (int32 Vert : AffectedVertNormals)
	{
		GlobalVertexData[Vert].Normals = CalculateVertexNormal(Vert);
	}

	// Update section directly or via Queue
	for (int32 Iter : AffectedSections)
	{
		if (!SectionActors.IsValidIndex(Iter)) { 
			continue; 
		}
		if (Settings.bUseUpdateQueue && !SectionUpdateQueue.Contains(Iter)) { 
			SectionUpdateQueue.Add(Iter);
			SectionActors[Iter]->UpdateSection();
		}
		else { 
			//SectionActors[Iter]->UpdateSection();
		};
	}
}


void ATerrainGenerator::AddAffectedSections(int32 SectionIndex, int32 VertexIndex, OUT TArray<int32> &AffectedSections)
{
	// TODO find better way to detect border vertices (modulo?) and find a way to know which sectionindex to add 
	int32 NeighbourSection;
	switch (SectionProperties.SectionPosition[VertexIndex])
	{
	case ESectionPosition::SB_NotOnBorder:
		if (!AffectedSections.Contains(SectionIndex)) { AffectedSections.Add(SectionIndex); }
		break;

	case ESectionPosition::SB_BorderRight:
		NeighbourSection = SectionIndex + 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_BorderLeft:
		NeighbourSection = SectionIndex - 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_BorderTop:
		NeighbourSection = SectionIndex + ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_BorderBottom:
		NeighbourSection = SectionIndex - ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_EdgeBottomLeft:
		NeighbourSection = SectionIndex - 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex - ComponentXY - 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex - ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_EdgeBottomRight:
		NeighbourSection = SectionIndex + 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex - ComponentXY + 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex - ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_EdgeTopLeft:
		NeighbourSection = SectionIndex - 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex + ComponentXY - 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex + ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;

	case ESectionPosition::SB_EdgeTopRight:
		NeighbourSection = SectionIndex + 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex + ComponentXY + 1;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		NeighbourSection = SectionIndex + ComponentXY;
		if (!AffectedSections.Contains(NeighbourSection)) { AffectedSections.Add(NeighbourSection); }
		break;
	}
}


void ATerrainGenerator::SectionUpdateFinished()
{
	bAllowedToUpdateSection = true;
	if (SectionUpdateQueue.IsValidIndex(0)) { SectionUpdateQueue.RemoveAt(0); }	
}


FVector ATerrainGenerator::CalculateVertexNormal(int32 VertexIndex)
{
	int32 VertsPerSide = (ComponentXY * SectionXY - (ComponentXY - 1));
	int32 U = VertexIndex + VertsPerSide;
	int32 D = VertexIndex - VertsPerSide;
	int32 R = VertexIndex + 1;
	int32 L = VertexIndex - 1;

	if (L < 0 || D < 0 || U >= GlobalVertexData.Num() || R >= GlobalVertexData.Num()) { return FVector(0.f, 0.f, 0.f); }

	FVector X = GlobalVertexData[U].Vertices - GlobalVertexData[D].Vertices;
	FVector Y = GlobalVertexData[R].Vertices - GlobalVertexData[L].Vertices;

	return FVector::CrossProduct(X, Y).GetSafeNormal();
}


FVector ATerrainGenerator::CalculateVertexNormalByNoise(FVector Coordinates)
{
	FVector ULoc = FVector((Coordinates.X + 1.f) * QuadSize, Coordinates.Y * QuadSize, GetHeightByNoise(Coordinates.X + 1.f, Coordinates.Y));
	FVector DLoc = FVector((Coordinates.X - 1.f) * QuadSize, Coordinates.Y * QuadSize, GetHeightByNoise(Coordinates.X - 1.f, Coordinates.Y));
	FVector RLoc = FVector(Coordinates.X * QuadSize, (Coordinates.Y + 1.f) * QuadSize, GetHeightByNoise(Coordinates.X, Coordinates.Y + 1.f));
	FVector LLoc = FVector(Coordinates.X * QuadSize, (Coordinates.Y - 1.f) * QuadSize, GetHeightByNoise(Coordinates.X, Coordinates.Y - 1.f));


	FVector X = ULoc - DLoc;
	FVector Y = RLoc - LLoc;

	return FVector::CrossProduct(X, Y).GetSafeNormal();
}


float ATerrainGenerator::GetHeightByNoise(int32 XCoords, int32 YCoords)
{
	float QuadCorrection = QuadSize / 100.f;
	float Noise;

	float Octave1 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(XCoords / Scale1 * QuadCorrection, YCoords / Scale1 * QuadCorrection, 0.f, NoiseMultiplier1);
	float Octave2 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(XCoords / Scale2 * QuadCorrection, YCoords / Scale2 * QuadCorrection, 0.f, NoiseMultiplier2);
	float Octave3 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(XCoords / Scale3 * QuadCorrection, YCoords / Scale3 * QuadCorrection, 0.f, NoiseMultiplier3);
	float Octave4 = USimplexNoiseBPLibrary::SimplexNoiseInRange2D(XCoords / Scale4 * QuadCorrection, YCoords / Scale4 * QuadCorrection, 0.f, NoiseMultiplier4);

	Noise = Octave1;
	Noise += bUseOctave2 ? Octave2 : 0.f;
	Noise += bUseOctave3 ? Octave3 : 0.f;
	Noise += bUseOctave4 ? Octave4 : 0.f;

	return Noise;
}

FColor ATerrainGenerator::CombineColors(FColor A, FColor B)
{
	FColor C;
	C = FColor(
		FMath::Clamp(A.R + B.R, 0, 255),
		FMath::Clamp(A.G + B.G, 0, 255),
		FMath::Clamp(A.B + B.B, 0, 255),
		FMath::Clamp(A.A + B.A, 0, 255));

		return C;
}


FColor ATerrainGenerator::ColorSubtract(FColor A, FColor B)
{
	FColor C;
	C = FColor(
		FMath::Clamp(A.R - B.R, 0, 255),
		FMath::Clamp(A.G - B.G, 0, 255),
		FMath::Clamp(A.B - B.B, 0, 255),
		FMath::Clamp(A.A - B.A, 0, 255));

	return C;
}


void ATerrainGenerator::VertexPaint(int32 VertexIndex, float DistanceFraction, const FSculptSettings& Settings)
{
	FColor CurrentVertexColor = GlobalVertexData[VertexIndex].VertexColor;

	// scale Input color
	float LerpAlpha = Curve->GetFloatValue(DistanceFraction) * Settings.Falloff;
	uint8 R = FMath::Lerp(Settings.Color.R, (uint8)0, LerpAlpha) * Settings.ToolStrength;
	uint8 G = FMath::Lerp(Settings.Color.G, (uint8)0, LerpAlpha) * Settings.ToolStrength;
	uint8 B = FMath::Lerp(Settings.Color.B, (uint8)0, LerpAlpha) * Settings.ToolStrength;
	uint8 A = FMath::Lerp(Settings.Color.A, (uint8)0, LerpAlpha) * Settings.ToolStrength;
	FColor InputColorScaled = FColor(R, G, B, A);

	if (Settings.bInvertToolDirection)
	{
		GlobalVertexData[VertexIndex].VertexColor = ColorSubtract(CurrentVertexColor, InputColorScaled);
	}
	else
	{
		// find the strongest layer from ColorF. Only the strongest will be considered for painting
		TArray<uint8> RGBA;
		RGBA.Add(InputColorScaled.R);
		RGBA.Add(InputColorScaled.G);
		RGBA.Add(InputColorScaled.B);
		RGBA.Add(InputColorScaled.A);

		uint8 Temp = 0;
		uint8 Index = 0;
		for (int32 i = 0; i < RGBA.Num(); i++)
		{
			if (RGBA[i] > Temp)
			{
				Temp = RGBA[i];
				Index = i;
			}
		}

		// check if all layers combined reached the max value of 255
		FColor TempColor = CombineColors(CurrentVertexColor, InputColorScaled);
		int32 Value = TempColor.R + TempColor.G + TempColor.B + TempColor.A;
		bool bRemove = (Value > 255) ? true : false;

		uint8 Red, Green, Blue, Alpha;

		// reduce paint on unused layers equally to make room for painting on selected layer
		if (bRemove)
		{
			int32 ToRemove = (Value - 255) / 3; // TODO if CurrentVertColor RGB or A is already 0, remove from other 2 layers to make enought room for painted color

			Red		= (Index == 0) ? 0 : ToRemove;
			Green	= (Index == 1) ? 0 : ToRemove;
			Blue	= (Index == 2) ? 0 : ToRemove;
			Alpha	= (Index == 3) ? 0 : ToRemove;
			TempColor = ColorSubtract(CurrentVertexColor, FColor(Red, Green, Blue, Alpha));
		}

		// Paint selected layer
		Red		= (Index == 0) ? InputColorScaled.R : 0;
		Green	= (Index == 1) ? InputColorScaled.G : 0;
		Blue	= (Index == 2) ? InputColorScaled.B : 0;
		Alpha	= (Index == 3) ? InputColorScaled.A : 0;
		TempColor = bRemove ? TempColor : CurrentVertexColor;

		FColor EndResult = CombineColors(TempColor, FColor(Red, Green, Blue, Alpha));
		GlobalVertexData[VertexIndex].VertexColor = EndResult;
	}
}


void ATerrainGenerator::VertexSmooth(int32 VertexIndex, float DistanceFraction, const FSculptSettings& Settings)
{
	int32 VertsPerSide = (ComponentXY * SectionXY - (ComponentXY - 1));
	int32 U = VertexIndex + VertsPerSide;
	int32 D = VertexIndex - VertsPerSide;
	int32 R = VertexIndex + 1;
	int32 L = VertexIndex - 1;

	if (L < 0 || D < 0 || U >= GlobalVertexData.Num() || R >= GlobalVertexData.Num()) { return; }

	// get  Z distance from Vertex to Z average of neighbours
	float Average = (GlobalVertexData[U].Vertices + GlobalVertexData[D].Vertices + GlobalVertexData[R].Vertices + GlobalVertexData[L].Vertices).Z / 4.f;
	float ZOffset = (FVector(0.f, 0.f, Average) - FVector(0.f, 0.f, GlobalVertexData[VertexIndex].Vertices.Z)).Size();

	// scale Offset according to user sculpt settings
	float Alpha = Curve->GetFloatValue(DistanceFraction) * Settings.Falloff;
	ZOffset = FMath::Lerp(ZOffset, 0.f, Alpha) * Settings.ToolStrength;

	// check if vertex has to be lowered or raised 
	bool bToRaise = GlobalVertexData[VertexIndex].Vertices.Z < Average;
	ZOffset = bToRaise ? (ZOffset) : -(ZOffset);

	GlobalVertexData[VertexIndex].Vertices += FVector(0.f, 0.f, ZOffset);
}


void ATerrainGenerator::VertexChangeHeight(int32 VertexIndex, float DistanceFraction, const FSculptSettings& Settings)
{
	float ScaledZStrength = MaxZValueOffsetPerUpdate * Settings.ToolStrength;
	float Alpha = Curve->GetFloatValue(DistanceFraction) * Settings.Falloff;
	float ZValue = FMath::Lerp(ScaledZStrength, 0.f, Alpha);

	GlobalVertexData[VertexIndex].Vertices += (Settings.bInvertToolDirection) ? (FVector(0.f, 0.f, -ZValue)) : (FVector(0.f, 0.f, ZValue));
}


void ATerrainGenerator::VertexFlatten(int32 VertexIndex, float DistanceFraction, const FSculptSettings& Settings, const FSculptInputInfo& InputInfo)
{
	float Alpha = Curve->GetFloatValue(DistanceFraction) * Settings.Falloff;
	float StartLocationHight = InputInfo.StartLocation.Z - GetActorLocation().Z;
	float ZValue = FMath::Lerp(StartLocationHight, GlobalVertexData[VertexIndex].Vertices.Z, Alpha);

	float ZValue2 = FMath::Lerp(GlobalVertexData[VertexIndex].Vertices.Z, ZValue, Settings.ToolStrength);
	FVector Flatted = FVector(GlobalVertexData[VertexIndex].Vertices.X, GlobalVertexData[VertexIndex].Vertices.Y, ZValue2);

	GlobalVertexData[VertexIndex].Vertices = Flatted;
}


void ATerrainGenerator::VertexAddNoise(int32 VertexIndex, float DistanceFraction, const FSculptSettings& Settings)
{
	int32 VertsPerSide = (ComponentXY * SectionXY - (ComponentXY - 1));
	int32 X = VertexIndex / VertsPerSide;
	int32 Y = VertexIndex % VertsPerSide;

	float QuadCorrection = QuadSize / 100.f;
	float Noise = USimplexNoiseBPLibrary::SimplexNoiseInRange2D((X  * QuadCorrection) / Settings.NoiseScale, (Y * QuadCorrection) / Settings.NoiseScale, 0.f, MaxZValueOffsetPerUpdate) * Settings.ToolStrength;

	float Alpha = Curve->GetFloatValue(DistanceFraction) * Settings.Falloff;
	float ZValue = FMath::Lerp(Noise, 0.f, Alpha);

	GlobalVertexData[VertexIndex].Vertices += (Settings.bInvertToolDirection) ? (FVector(0.f, 0.f, -ZValue)) : (FVector(0.f, 0.f, ZValue));
}
