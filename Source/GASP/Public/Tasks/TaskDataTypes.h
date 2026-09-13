#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructUtils/InstancedStruct.h"

#include "TaskDataTypes.generated.h"

/**
 * A group of heterogeneous USTRUCT values, at most one instance per struct type, that replicates as a
 * unit. Modelled on FMoverDataCollection, but built on FInstancedStruct, which already provides the
 * type erasure, ownership, deep copy and per-element net serialization that Mover hand-rolls around
 * FMoverDataStructBase.
 *
 * The templated accessors are the whole data API. T must be a UHT-generated USTRUCT, that is one
 * declaring StaticStruct(); core types such as FVector or FGuid do not qualify.
 *
 * Pointers handed out by the accessors alias the internal array and are invalidated by any later add
 * or remove.
 */
USTRUCT(BlueprintType)
struct GASP_API FInstancedStructCollection
{
	GENERATED_BODY()

	/** Removes every element and releases the storage. */
	void Empty() { DataArray.Empty(); }

	/** Number of elements currently held. */
	int32 Num() const { return DataArray.Num(); }

	/**
	 * Serializes every element, struct type included.
	 *
	 * Any USTRUCT is accepted - there is no base type gate as in FMoverDataCollection - so this is
	 * safe for server to client property replication, but must not be received from a client RPC
	 * without an external type filter.
	 *
	 * Needs a real UPackageMapClient: for a struct with no native net serializer the engine falls
	 * back to a delegate that asserts on a null package map.
	 */
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	/** Deep, element-wise comparison. Drives WithIdenticalViaEquality. */
	bool operator==(const FInstancedStructCollection& Other) const;

	/** Exposes the contained structs' object references to the GC. */
	void AddStructReferencedObjects(FReferenceCollector& Collector);

	/** The stored instance of T, or null if the collection holds none. */
	template <typename T>
	const T* FindDataByType() const
	{
		const int32 Index{IndexOfType(T::StaticStruct())};
		return Index != INDEX_NONE ? DataArray[Index].GetPtr<T>() : nullptr;
	}

	/** The stored instance of T, or null if the collection holds none. */
	template <typename T>
	T* FindMutableDataByType()
	{
		const int32 Index{IndexOfType(T::StaticStruct())};
		return Index != INDEX_NONE ? DataArray[Index].GetMutablePtr<T>() : nullptr;
	}

	/**
	 * The stored instance of T, default-constructing and appending one if the collection holds none.
	 */
	template <typename T>
	T* FindOrAddDataByType()
	{
		int32 Index{IndexOfType(T::StaticStruct())};

		if (Index == INDEX_NONE)
		{
			Index = DataArray.AddDefaulted();
			DataArray[Index].InitializeAs(T::StaticStruct());
		}

		return DataArray[Index].GetMutablePtr<T>();
	}

	/** Adds this data instance to the collection, taking ownership of it. If an existing data struct of the same type is already there, it will be removed first. */
	void AddOrOverwriteData(FInstancedStruct DataInstance);

	/** Adds data to the collection by copying over an existing struct or cloning the provided struct if no matching struct exists. 
	 *  This is different than AddOrOverwriteData because the instance isn't touched, avoiding memory allocation & array changing if a matching struct exists.
	 */
	void AddDataByCopy(const FInstancedStruct* DataInstanceToCopy);

	const TArray<FInstancedStruct>& GetDataArray() const
	{
		return DataArray;
	}

	/** Removes data of a specific type in the collection. Returns true if data was removed. */
	bool RemoveDataByType(const UScriptStruct* DataStructType);

private:
	/** The only place DataArray is scanned. Returns INDEX_NONE for an absent or null type. */
	int32 IndexOfType(const UScriptStruct* DataStructType) const;

	/** All data in this collection. Not a UPROPERTY - reachability comes from AddStructReferencedObjects. */
	TArray<FInstancedStruct> DataArray;

	friend class UInstancedStructCollectionLibrary;
};

template <>
struct TStructOpsTypeTraits<FInstancedStructCollection> : public TStructOpsTypeTraitsBase2<FInstancedStructCollection>
{
	enum
	{
		WithCopy = true, // Necessary so that DataArray is copied around
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
		WithAddStructReferencedObjects = true,
	};
};

/**
 * Blueprint access to a FInstanceStructCollection through wildcard struct pins, so a Blueprint can
 * store and retrieve any USTRUCT type without a C++ accessor per type.
 */
UCLASS()
class GASP_API UInstancedStructCollectionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Adds a struct to the collection, overwriting any existing instance of the same type.
	 * @param Collection
	 * @param SourceAsRawBytes The struct instance to store, copied into the collection.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "GASP|Task Data",
		meta = (CustomStructureParam = "SourceAsRawBytes", AllowAbstract = "false",
			DisplayName = "Add Data To Collection"))
	static void K2_AddDataToCollection(UPARAM(Ref) FInstancedStructCollection& Collection,
	                                   UPARAM(DisplayName = "Struct To Add") const int32& SourceAsRawBytes);

	/**
	 * Retrieves a struct from the collection, writing into the target if an instance of the matching
	 * type is present. Changes must be written back with Add Data To Collection.
	 * @param DidSucceed Whether data was actually written to the target struct instance.
	 * @param Collection
	 * @param TargetAsRawBytes The struct instance to write to.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "GASP|Task Data",
		meta = (CustomStructureParam = "TargetAsRawBytes", AllowAbstract = "false",
			DisplayName = "Get Data From Collection"))
	static void K2_GetDataFromCollection(bool& DidSucceed, UPARAM(Ref) const FInstancedStructCollection& Collection,
	                                     UPARAM(DisplayName = "Out Struct") int32& TargetAsRawBytes);

	/** Removes the stored instance of StructType. Returns true if one was removed. */
	UFUNCTION(BlueprintCallable, Category = "GASP|Task Data",
		meta = (DisplayName = "Remove Data From Collection"))
	static bool RemoveDataFromCollection(UPARAM(Ref) FInstancedStructCollection& Collection,
	                                     const UScriptStruct* StructType);

	/** Clears all data from a collection. */
	UFUNCTION(BlueprintCallable, Category = "GASP|Task Data",
		meta = (DisplayName = "Clear Data From Collection"))
	static void ClearDataFromCollection(UPARAM(Ref) FInstancedStructCollection& Collection);

private:
	DECLARE_FUNCTION(execK2_AddDataToCollection);
	DECLARE_FUNCTION(execK2_GetDataFromCollection);
};
