// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Banette/Core/Service.h"

namespace Banette::Pipeline
{
	using namespace Banette::Core;

	/// TagT is a phantom type to allow multiple specializations per service type T
	template <CService T, typename TagT = void>
	struct TServiceProvider
	{
		using TagType = TagT;

		virtual ~TServiceProvider() = default;

		static TSharedPtr<T> BuildService()
		{
			static_assert(sizeof(T) == 0,
			              "Banette::Pipeline::GetServiceImpl(TServiceTag<T>) is not specialized "
			              "for this service type T. Please provide an overload in some header.");
			return nullptr;
		}

		static TSharedPtr<T> GetService()
		{
			static TSharedPtr<T> Service = nullptr;

			if (!Service.IsValid())
			{
				Service = BuildService();
				return Service;
			}
			return Service;
		}
	};
}

#define BANETTE_SERVICE_PROVIDER(T) \
static TSharedPtr<T> GetService() \
{ \
static TSharedPtr<T> Service = nullptr; \
\
if (!Service.IsValid()) \
{ \
Service = BuildService(); \
return Service; \
} \
return Service; \
} \
static TSharedPtr<T> BuildService()
