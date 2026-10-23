// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Banette/Core/Service.h"
#include "Banette/Core/Layer.h"

namespace Banette::Pipeline
{
	using namespace Banette::Core;

	/// Empty state: no Service yet
	class FEmptyServiceState
	{
	public:
		using ServiceType = void;
	};

	/// ServiceBuilder: chain-build a Service wrapped by multiple Layers
	template <typename CurrentServiceT = void>
	class TServiceBuilder
	{
		template <typename>
		friend class TServiceBuilder;

	public:
		TServiceBuilder() = default;

		/// Start from a concrete Service
		template <CService S>
		static TServiceBuilder<S> New(TSharedRef<S> InService)
		{
			return TServiceBuilder<S>(InService);
		}

		/// Apply a Layer: pass the current Service to the Layer to get a new Service
		/// The Layer must satisfy: its InServiceType is CurrentServiceT, and OutServiceType is the new type
		template <CLayer L>
			requires std::is_same_v<typename L::InServiceType, CurrentServiceT>
		TServiceBuilder<typename L::OutServiceType> Layer(L& InLayer)
		{
			static_assert(!std::is_same_v<CurrentServiceT, void>,
			              "Cannot apply layer to empty builder. Start with New(service) first.");

			auto WrappedService = InLayer.Wrap(CurrentService);
			return TServiceBuilder<typename L::OutServiceType>(WrappedService);
		}

		/// Get the final Service
		template <typename S = CurrentServiceT>
			requires (!std::is_same_v<S, void>)
		TSharedRef<S> Build() const
		{
			static_assert(!std::is_same_v<CurrentServiceT, void>,
			              "Cannot build from empty builder. Start with New(service) first.");
			return CurrentService;
		}

	private:
		explicit TServiceBuilder(TSharedRef<CurrentServiceT> InService)
			: CurrentService(InService)
		{
		}

		TSharedRef<CurrentServiceT> CurrentService;
	};

	/// Specialization: empty state
	template <>
	class TServiceBuilder<void>
	{
	public:
		TServiceBuilder() = default;

		/// Start from a concrete Service
		template <CService S>
		static TServiceBuilder<S> New(TSharedRef<S> InService)
		{
			return TServiceBuilder<S>(InService);
		}
	};
}
