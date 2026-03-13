#include "Settings/GASPCharacterSettings.h"
#include "Async/Async.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPCharacterSettings)

namespace
{
	void CollectTablePaths(const UGASPCharacterSettings& Settings, TArray<FSoftObjectPath>& OutPaths)
	{
		OutPaths.Reset();
		OutPaths.Reserve(4);

		const auto AddIfSet = [&OutPaths](const TSoftObjectPtr<UChooserTable>& Table)
		{
			if (!Table.IsNull())
			{
				if (const auto Path{Table.ToSoftObjectPath()}; Path.IsValid())
				{
					OutPaths.Add(Path);
				}
			}
		};

		AddIfSet(Settings.OverlayTable);
		AddIfSet(Settings.PosesTable);
		AddIfSet(Settings.RotationCurveTable);
		AddIfSet(Settings.TraversalTable);
	}
}

UGASPCharacterSettings::UGASPCharacterSettings()
{
}

void UGASPCharacterSettings::PostLoad()
{
	Super::PostLoad();

	TablesLoadHandle.Reset();

	const auto ShouldAutoPreload = [this]()
	{
		if (HasAnyFlags(RF_ClassDefaultObject) || IsRunningCommandlet())
		{
			return false;
		}

#if WITH_EDITOR
		if (GIsEditor && !IsRunningGame())
		{
			return false;
		}
#endif

		return true;
	};

	if (!ShouldAutoPreload())
	{
		return;
	}

	if (IsInGameThread())
	{
		PreloadTables();
	}
	else
	{
		TWeakObjectPtr<UGASPCharacterSettings> WeakThis{this};
		AsyncTask(ENamedThreads::GameThread, [WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->PreloadTables();
			}
		});
	}
}

void UGASPCharacterSettings::PreloadTables()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	if (TablesLoadHandle.IsValid())
	{
		return;
	}

	TArray<FSoftObjectPath> SoftObjectPaths;
	CollectTablePaths(*this, SoftObjectPaths);
	if (SoftObjectPaths.IsEmpty())
	{
		return;
	}

	auto& StreamableManager = UAssetManager::GetStreamableManager();
	TablesLoadHandle = StreamableManager.RequestAsyncLoad(
		SoftObjectPaths,
		FStreamableDelegate(),
		FStreamableManager::AsyncLoadHighPriority);
}
