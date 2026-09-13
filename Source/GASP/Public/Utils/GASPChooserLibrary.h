#pragma once

#include "Chooser.h"
#include "ChooserFunctionLibrary.h"
#include "IObjectChooser.h"

#include <type_traits>

/**
 * Pure C++ wrappers around UChooserFunctionLibrary that collapse the
 * MakeChooserEvaluationContext / AddObjectParam / AddStructParam / MakeEvaluateChooser /
 * EvaluateObjectChooserBase boilerplate into a single typed call.
 *
 * Params are added in the order given - the order the chooser table reads them by - and may
 * mix freely: a UObject-derived pointer (anim instance, UClass, this) becomes an object param,
 * anything else is taken as a USTRUCT. Struct params are bound by reference (FStructView) and
 * written in place, so they must be named, mutable lvalues that outlive the call.
 *
 * Keep object params to four or fewer: FChooserEvaluationContext stores them in a
 * TInlineAllocator<4> array and holds views into it, so a fifth one rehomes that array and
 * dangles the views already taken for the earlier objects.
 */
struct GASP_API FGASPChooserUtils
{
private:
	template <typename T>
	static void AddParam(FChooserEvaluationContext& Context, T&& Param)
	{
		if constexpr (std::is_convertible_v<std::decay_t<T>, UObject*>)
		{
			Context.AddObjectParam(Param);
		}
		else
		{
			Context.AddStructParam(Param);
		}
	}

public:
	/** Evaluates Table with the given params and returns the single result cast to T. */
	template <typename T, typename... TParams>
	static T* EvaluateSingle(UChooserTable* Table, TParams&&... Params)
	{
		if (!IsValid(Table))
		{
			return nullptr;
		}

		auto Context{UChooserFunctionLibrary::MakeChooserEvaluationContext()};
		(AddParam(Context, Forward<TParams>(Params)), ...);

		return Cast<T>(UChooserFunctionLibrary::EvaluateObjectChooserBase(
			Context, UChooserFunctionLibrary::MakeEvaluateChooser(Table), T::StaticClass()));
	}

	/** Multi-result variant. Returns an empty array when Table is invalid. */
	template <typename T, typename... TParams>
	static TArray<T*> EvaluateMulti(UChooserTable* Table, TParams&&... Params)
	{
		if (!IsValid(Table))
		{
			return {};
		}

		auto Context{UChooserFunctionLibrary::MakeChooserEvaluationContext()};
		(AddParam(Context, Forward<TParams>(Params)), ...);

		TArray<T*> Results;
		for (auto* Object : UChooserFunctionLibrary::EvaluateObjectChooserBaseMulti(
			     Context, UChooserFunctionLibrary::MakeEvaluateChooser(Table), T::StaticClass()))
		{
			if (auto* Typed = Cast<T>(Object))
			{
				Results.Add(Typed);
			}
		}

		return Results;
	}
};
