#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "TextureToolsMaterialNodes.generated.h"

UCLASS(meta = (DisplayName = "Data Texture Sample"))
class TEXTURETOOLS_API UDataTextureSampleNode : public UMaterialExpressionTextureSample
{
    GENERATED_BODY()

public:
    // Name of the row in the database
    UPROPERTY(EditAnywhere, Category = Texture)
    FString RowName;

    // Reference to the texture object
    UPROPERTY()
    UTexture2D* DataTexture;

    // Constructor
    UMyTextureSampleNode()
    {
        DataTexture = nullptr;
    }

#if WITH_EDITOR
    // Override GetCaption to customize the node's name in the material editor
    virtual void GetCaption(TArray<FString>& OutCaptions) const override
    {
        OutCaptions.Add(TEXT("My Texture Sample Node"));
    }
#endif // WITH_EDITOR

    // Override Compile to return the texture object
    virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override
    {
        if (DataTexture == nullptr)
        {
            // If the texture object is not yet set, try to find it in the database
            UTexture2D* FoundTexture = UMyDatabase::GetTexture(RowName);
            if (FoundTexture != nullptr)
            {
                DataTexture = FoundTexture;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Texture with row name '%s' not found in database."), *RowName);
            }
        }

        // Output the texture object
        return Compiler->Texture(DataTexture);
    }
};