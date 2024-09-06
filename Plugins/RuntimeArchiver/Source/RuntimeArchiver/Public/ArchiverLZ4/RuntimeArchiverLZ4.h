﻿// Georgy Treshchev 2024.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeArchiverBase.h"
#include "ArchiverTar/RuntimeArchiverTar.h"
#include "UObject/StrongObjectPtr.h"
#include "RuntimeArchiverLZ4.generated.h"

/**
 * LZ4 archiver class. Works with tar.lz4 (tlz4) archives
 * Archiving of data occurs through the Tar archiver and their subsequent compression through LZ4 raw archiver (the same applies for unarchiving)
 * Note that it currently has some bugs when unarchiving data from a tar archive
 */
UCLASS(BlueprintType, Category = "Runtime Archiver")
class RUNTIMEARCHIVER_API URuntimeArchiverLZ4 : public URuntimeArchiverBase
{
	GENERATED_BODY()

public:
	URuntimeArchiverLZ4();

	//~ Begin URuntimeArchiverBase Interface
	virtual bool CreateArchiveInStorage(FString ArchivePath) override;
	virtual bool CreateArchiveInMemory(int32 InitialAllocationSize = 0) override;

	virtual bool OpenArchiveFromStorage(FString ArchivePath) override;
	virtual bool OpenArchiveFromMemory(const TArray64<uint8>& ArchiveData) override;

	virtual bool CloseArchive() override;

	virtual bool GetArchiveData(TArray64<uint8>& ArchiveData) override;

	virtual bool GetArchiveEntries(int32& NumOfArchiveEntries) override;

	virtual bool GetArchiveEntryInfoByName(FString EntryName, FRuntimeArchiveEntry& EntryInfo) override;
	virtual bool GetArchiveEntryInfoByIndex(int32 EntryIndex, FRuntimeArchiveEntry& EntryInfo) override;

	virtual bool AddEntryFromMemory(FString EntryName, const TArray64<uint8>& DataToBeArchived, ERuntimeArchiverCompressionLevel CompressionLevel) override;

	virtual bool ExtractEntryToMemory(const FRuntimeArchiveEntry& EntryInfo, TArray64<uint8>& UnarchivedData) override;

	virtual bool Initialize() override;
	virtual bool IsInitialized() const override;
	virtual void Reset() override;

	virtual void ReportError(ERuntimeArchiverErrorCode ErrorCode, const FString& ErrorString) const override;
	//~ End URuntimeArchiverBase Interface

private:
	/** Tar archiver used for internal operations */
	TStrongObjectPtr<URuntimeArchiverTar> TarArchiver;

	/** Stream containing LZ4 compressed data */
	TUniquePtr<FRuntimeArchiverBaseStream> CompressedStream;

	/** Last saved compression level */
	ERuntimeArchiverCompressionLevel LastCompressionLevel;
};
