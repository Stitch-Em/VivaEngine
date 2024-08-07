#pragma once
#include "CoreMinimal.h"
#include "RomanceMinigame.generated.h"

//Object Type Enum
UENUM(BlueprintType)
enum class EObjectType : uint8
{
	//Null Object
	Null,
	//Standard Bomb
	RedBomb,
	//Sleepy Bomb You need to sneek past it or it will explode
	OrangeBomb,
	//Super Bomb has a larger detonation radius and is persistent even after detonation
	BlueBomb,
	//You can only see where it is at the begining and while holding a button that also locks imput, they are 
	HiddenBomb,
	//These bombs move around and are green
	MovingBomb,
	//These bombs Freeze the pinata when they explode for a short time
	WhiteBomb,
	//OilBomb that makes the pinata slide for a short time (Has to have a cowboy hat)
	BrownBomb,
	//These bombs make the pinata move in the opposite direction for a short time
	PurpleBomb,
	//These bombs force the pinata move faster for a short time (Good for speed runs)
	YellowBomb,
	//These bombs make smoke obscuring the players vision for a breef time
	BlackBomb,
	//Magnetic bombs that will pull the player towards them
	GrayBomb,
	//These bombs are 2D and look like a shadow with the loather face, it will move in the direction of the player slowly then pop out of the ground and explode.
	ShadowBomb,
	//Pink bombs destracts the end pinata make makes them follow
	PinkBomb,
	//Collectable that you need to collect to obtain master romancer
	Heart,
	//Gives the player 10 coin
	Coins_T1, 
	//Gives the player 50 coins
	Coins_T2, 
	//Gives the player 100 coins
	Coins_T3, 
	//Gives the player 200 coins
	Coins_T4, 
	//Where the player starts
	Spawner, 
	//End of the the level
	Exit
};

//Game mode enum
UENUM(BlueprintType)
enum class EGameMode : uint8
{
	Create,
	Play
};

//Location Enum
UENUM(BlueprintType)
enum class ELocation : uint8
{
	LandGarden,
	SeaGarden,
	SnowGarden,
	DesertGarden,
	JungleGarden,
	TrenchGarden
};

//Creator Mode Enum
UENUM(BlueprintType)
enum class ECreatorMode : uint8
{
	Object,
	Track,
	LinkObjectToTrack,
	Packet
};

//Creator Action Enum
UENUM(BlueprintType)
enum class EAction : uint8
{
	Place,
	Delete
};

//Struct that holds all the information for one minigame object
USTRUCT(BlueprintType)
struct FMinigameObject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EObjectType ObjectType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsInvisible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int TrackNum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int StartNode;
};

//Used for storing a history of edits made in the editor
USTRUCT(BlueprintType)
struct FCreatorAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAction Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMinigameObject Object;
};

//Struct for storing the information of a fixed track objects travel
USTRUCT(BlueprintType)
struct FTrack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> Nodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed;
};

UENUM(BlueprintType)
enum class EMazeSize : uint8
{
	Small,
	Medium,
	Large,
	Huge
};

//Holds all the combined information for an individual minigame
USTRUCT(BlueprintType)
struct FMinigame
{
	GENERATED_BODY()
	
	//The time limit in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeLimit;
	
	//The size of the maze
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMazeSize Size;
	
	//Array containing all the objects in the minigame
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FMinigameObject> Objects;
	
	//Array containing all the fixed tracks in the minigame
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTrack> Tracks;

	//Name of RomanceMinigameFolder
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

};


UCLASS(BlueprintType)
class ROMANCEMINIGAMEASSETRUNTIME_API URomanceMinigame : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FMinigameObject> Objects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ELocation Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeLimit; //Time limit in seconds

	//Array containing all the fixed tracks in the minigame
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTrack> Tracks;

	//Name of RomanceMinigameFolder
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	//The size of the maze
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMazeSize Size;
};
