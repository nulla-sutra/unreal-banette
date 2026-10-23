// Copyright 2019-Present tarnishablec. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Banette/Core/Service.h"

namespace Banette::Core
{
	template <CService InServiceT, CService OutService>
	class TLayer
	{
	public:
		using InServiceType = InServiceT;
		using OutServiceType = OutService;

		virtual ~TLayer() = default;

		virtual TSharedRef<OutService> Wrap(TSharedRef<InServiceT> Inner) const = 0;
	};

	template <typename S>
	concept CLayer =
		std::is_base_of_v<
			TLayer<typename S::InServiceType, typename S::OutServiceType>,
			S>;
}
