// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Banette/Banette.h"
#include "UE5Coro.h"

namespace Banette::Kit
{
	using namespace Banette::Core;

	/// Configuration for retry behavior


	/// Generic Retry Layer that wraps any Service to add retry logic
	/// Both InServiceType and OutServiceType are the same Service type
	template <CService ServiceT>
	class TRetryLayer : public TLayer<ServiceT, ServiceT>
	{
	public:
		using RequestType = ServiceT::RequestType;
		using ResponseType = ServiceT::ResponseType;

		struct FRetryConfig
		{
			// Maximum number of retry attempts (1 = no retries, only the original attempt)
			uint8 MaxAttempts = 3;

			// Delay between retries in seconds
			float DelayBetweenRetries = 0.1f;

			// Challenge whether we should retry (if false, then retry)
			TFunction<bool(const ResponseType&)> Challenge;
		};

		explicit TRetryLayer(const FRetryConfig& InConfig = FRetryConfig())
			: Config(InConfig)
		{
		}

		virtual ~TRetryLayer() override = default;

		/// Wraps the inner Service with retry logic
		virtual TSharedRef<ServiceT> Wrap(TSharedRef<ServiceT> Inner) const override
		{
			return MakeShared<FRetryService>(Inner, Config);
		}

	private:
		FRetryConfig Config;

		/// The actual wrapper Service that implements retry logic
		class FRetryService : public ServiceT
		{
		public:
			explicit FRetryService(TSharedRef<ServiceT> InInnerService, const FRetryConfig& InConfig)
				: InnerService(InInnerService)
				  , Config(InConfig)
			{
			}

			virtual ~FRetryService() override = default;

			virtual UE5Coro::TCoroutine<TResult<ResponseType>> Call(
				const RequestType& Request) override
			{
				for (int32 Attempt = 1; Attempt <= Config.MaxAttempts; ++Attempt)
				{
					auto Result = co_await InnerService->Call(Request);

					if (Result.IsValid())
					{
						// Don't retry if Challenge is not set or if Challenge returns true
						if (!Config.Challenge || Config.Challenge(Result.GetValue()))
						{
							co_return Result;
						}
					}

					// If this is the last attempt, return the error
					if (Attempt >= Config.MaxAttempts)
					{
						co_return Result;
					}

					// Wait before retrying
					if (Config.DelayBetweenRetries > 0.f)
					{
						co_await UE5Coro::Latent::Seconds(Config.DelayBetweenRetries);
					}

					// Log retry attempt
					UE_LOG(LogTemp, Warning,
					       TEXT("RetryLayer: Retrying (attempt %d/%d)"),
					       Attempt + 1, Config.MaxAttempts);
				}

				// Should never reach here, but just in case
				co_return TResult<ResponseType>();
			}

		private:
			TSharedRef<ServiceT> InnerService;
			FRetryConfig Config;
		};
	};
}
