// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Banette/Core/Error.h"
#include "UE5Coro.h"

namespace Banette::Core
{
	template <typename RequestT, typename ResponseT>
	class TService : public TSharedFromThis<TService<RequestT, ResponseT>>
	{
	public:
		using RequestType = RequestT;
		using ResponseType = ResponseT;

		virtual ~TService() = default;
		virtual UE5Coro::TCoroutine<TResult<ResponseT>> Call(const RequestT& Request) = 0;
	};


	template <typename RequestT, typename ResponseT>
	using TServiceRef = TSharedRef<TService<RequestT, ResponseT>>;


	template <typename S>
	concept CService =
		std::is_base_of_v<
			TService<typename S::RequestType, typename S::ResponseType>,
			S>;
}
