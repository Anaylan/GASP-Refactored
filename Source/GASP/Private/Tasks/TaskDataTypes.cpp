#include "Tasks/TaskDataTypes.h"
#include "Blueprint/BlueprintExceptionInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TaskDataTypes)

#define LOCTEXT_NAMESPACE "UInstanceStructCollectionLibrary"

bool FInstancedStructCollection::RemoveDataByType(const UScriptStruct* DataStructType)
{
	// No check() on DataStructType: this backs the Remove Data From Collection Blueprint node, whose
	// struct pin can legitimately be left empty. IndexOfType already maps null to INDEX_NONE.
	const int32 IndexToRemove{IndexOfType(DataStructType)};
	if (IndexToRemove == INDEX_NONE)
	{
		return false;
	}

	DataArray.RemoveAt(IndexToRemove);
	return true;
}

void FInstancedStructCollection::AddOrOverwriteData(FInstancedStruct DataInstance)
{
	check(DataInstance.IsValid());

	RemoveDataByType(DataInstance.GetScriptStruct());
	DataArray.Add(MoveTemp(DataInstance));
}

void FInstancedStructCollection::AddDataByCopy(const FInstancedStruct* DataInstanceToCopy)
{
	check(DataInstanceToCopy);
	check(DataInstanceToCopy->IsValid());

	const UScriptStruct* TypeToCopy{DataInstanceToCopy->GetScriptStruct()};

	const int32 Index{IndexOfType(TypeToCopy)};
	if (Index == INDEX_NONE)
	{
		// FInstancedStruct's copy constructor deep-copies the payload, so the source stays untouched.
		DataArray.Add(*DataInstanceToCopy);
		return;
	}

	// The matched element may hold a type derived from TypeToCopy. Copying through the source's own
	// type writes only the leading portion the source actually has and leaves the derived tail alone -
	// the same prefix copy K2_GetDataFromCollection relies on. No allocation, which is the whole point
	// of this entry point over AddOrOverwriteData.
	TypeToCopy->CopyScriptStruct(DataArray[Index].GetMutableMemory(), DataInstanceToCopy->GetMemory());
}

int32 FInstancedStructCollection::IndexOfType(const UScriptStruct* DataStructType) const
{
	if (!DataStructType)
	{
		return INDEX_NONE;
	}

	for (int32 Index{0}; Index < DataArray.Num(); ++Index)
	{
		const auto* ElementStruct{DataArray[Index].GetScriptStruct()};
		if (ElementStruct && ElementStruct->IsChildOf(DataStructType))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

bool FInstancedStructCollection::operator==(const FInstancedStructCollection& Other) const
{
	if (DataArray.Num() != Other.DataArray.Num())
	{
		return false;
	}

	for (int32 Index{0}; Index < DataArray.Num(); ++Index)
	{
		if (!DataArray[Index].Identical(&Other.DataArray[Index], PPF_None))
		{
			return false;
		}
	}

	return true;
}

void FInstancedStructCollection::AddStructReferencedObjects(FReferenceCollector& Collector)
{
	for (auto& Data : DataArray)
	{
		Data.AddStructReferencedObjects(Collector);
	}
}

bool FInstancedStructCollection::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	uint8 NumDataStructs{0};

	if (Ar.IsSaving())
	{
		// The count travels as a single byte, so a larger collection cannot be represented. Clamp
		// rather than let the cast wrap and describe a payload that was never written.
		ensureMsgf(DataArray.Num() <= MAX_uint8,
		           TEXT(
			           "FInstanceStructCollection::NetSerialize: %d elements exceed the %d the wire format holds, dropping the surplus."
		           ),
		           DataArray.Num(), static_cast<int32>(MAX_uint8));

		NumDataStructs = static_cast<uint8>(FMath::Min(DataArray.Num(), static_cast<int32>(MAX_uint8)));
	}

	Ar << NumDataStructs;

	if (Ar.IsLoading())
	{
		DataArray.SetNum(NumDataStructs);
	}

	for (int32 Index{0}; Index < NumDataStructs && !Ar.IsError(); ++Index)
	{
		// FInstancedStruct::NetSerialize writes the struct type alongside the payload and reuses the
		// existing allocation when the loaded type already matches.
		bool bElementSuccess{false};
		DataArray[Index].NetSerialize(Ar, Map, bElementSuccess);

		if (!bElementSuccess)
		{
			UE_LOG(LogTemp, Error,
			       TEXT("FInstanceStructCollection::NetSerialize: failed to serialize element %d (%s)."),
			       Index, *GetNameSafe(DataArray[Index].GetScriptStruct()));

			Ar.SetError();
			break;
		}
	}

	bOutSuccess = !Ar.IsError();
	return bOutSuccess;
}

void UInstancedStructCollectionLibrary::K2_AddDataToCollection(FInstancedStructCollection& Collection,
                                                               const int32& SourceAsRawBytes)
{
	// Native stub - execution goes through execK2_AddDataToCollection custom thunk
	checkNoEntry();
}

DEFINE_FUNCTION(UInstancedStructCollectionLibrary::execK2_AddDataToCollection)
{
	P_GET_STRUCT_REF(FInstancedStructCollection, TargetCollection);

	// Read wildcard SourceAsRawBytes input
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);

	const auto* SourceStructProp{CastField<FStructProperty>(Stack.MostRecentProperty)};
	const void* SourceValuePtr{Stack.MostRecentPropertyAddress};

	P_FINISH;

	if (!SourceStructProp || !SourceValuePtr || !SourceStructProp->Struct)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			LOCTEXT("InstanceStructCollection_AddInvalidSource",
			        "Failed to resolve the Struct To Add for Add Data To Collection.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		return;
	}

	P_NATIVE_BEGIN;
		const UScriptStruct* SourceStruct{SourceStructProp->Struct};

		int32 Index{TargetCollection.IndexOfType(SourceStruct)};
		if (Index == INDEX_NONE)
		{
			Index = TargetCollection.DataArray.AddDefaulted();
		}

		// InitializeAs copies in place when the element already holds this exact type, and reallocates
		// otherwise - which also covers overwriting an element that held a derived type.
		TargetCollection.DataArray[Index].InitializeAs(SourceStruct, static_cast<const uint8*>(SourceValuePtr));
	P_NATIVE_END;
}

void UInstancedStructCollectionLibrary::K2_GetDataFromCollection(bool& DidSucceed,
                                                                 const FInstancedStructCollection& Collection,
                                                                 int32& TargetAsRawBytes)
{
	// Native stub - execution goes through execK2_GetDataFromCollection custom thunk
	checkNoEntry();
}

DEFINE_FUNCTION(UInstancedStructCollectionLibrary::execK2_GetDataFromCollection)
{
	P_GET_UBOOL_REF(DidSucceed);
	P_GET_STRUCT_REF(FInstancedStructCollection, TargetCollection);

	// Read wildcard TargetAsRawBytes output
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);

	const auto* TargetStructProp{CastField<FStructProperty>(Stack.MostRecentProperty)};
	void* TargetValuePtr{Stack.MostRecentPropertyAddress};

	P_FINISH;

	DidSucceed = false;

	if (!TargetStructProp || !TargetValuePtr || !TargetStructProp->Struct)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			LOCTEXT("InstanceStructCollection_GetInvalidTarget",
			        "Failed to resolve the Out Struct for Get Data From Collection.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		return;
	}

	P_NATIVE_BEGIN;
		const UScriptStruct* TargetStruct{TargetStructProp->Struct};

		const int32 Index{TargetCollection.IndexOfType(TargetStruct)};
		if (Index != INDEX_NONE)
		{
			// Copy through the target's own type: a stored element may be a derived struct, and only its
			// leading TargetStruct portion fits the target instance.
			TargetStruct->CopyScriptStruct(TargetValuePtr, TargetCollection.DataArray[Index].GetMemory());
			DidSucceed = true;
		}
	P_NATIVE_END;
}

bool UInstancedStructCollectionLibrary::RemoveDataFromCollection(FInstancedStructCollection& Collection,
                                                                 const UScriptStruct* StructType)
{
	return Collection.RemoveDataByType(StructType);
}

void UInstancedStructCollectionLibrary::ClearDataFromCollection(FInstancedStructCollection& Collection)
{
	Collection.Empty();
}

#undef LOCTEXT_NAMESPACE
