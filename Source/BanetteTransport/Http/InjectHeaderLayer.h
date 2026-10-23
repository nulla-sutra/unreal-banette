// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Banette/Banette.h"
#include "BanetteTransport/Http/HttpClient.h"
#include "UE5Coro.h"

namespace Banette::Transport::Http
{
	using namespace Banette::Core;

	/// Type alias for async lazy header providers.
	/// A function that returns a coroutine yielding the header value.
	using FLazyHeaderProvider = TFunction<UE5Coro::TCoroutine<FString>()>;

	/// Layer that injects HTTP headers into outgoing requests.
	///
	/// This layer wraps an FHttpService and merges configured headers into each
	/// request before forwarding it to the inner service.
	///
	/// Usage example:
	/// @code
	/// using namespace Banette::Transport::Http;
	///
	/// TSharedRef<FHttpService> Base = MakeShared<FHttpService>();
	/// FInjectHeaderLayer Layer({{TEXT("X-Auth"), TEXT("Token")}}, /* bOverrideExisting = */ true);
	/// TSharedRef<FHttpService> WithHeaders = Layer.Wrap(Base);
	/// @endcode
	class FInjectHeaderLayer : public TLayer<FHttpClient, FHttpClient>
	{
	public:
		/**
		 * Construct a header-injection layer.
		 * @param InHeaders       Headers to inject into each request.
		 * @param bInOverrideExisting If true, injected headers override existing request headers.
		 *                            If false, headers are only added if not already present.
		 */
		explicit FInjectHeaderLayer(
			const TMap<FString, FString>& InHeaders = {},
			const bool bInOverrideExisting = false)
			: Headers(InHeaders)
			  , bOverrideExisting(bInOverrideExisting)
		{
		}

		/**
		 * Add a header to the injection set. Returns *this for chaining.
		 */
		FInjectHeaderLayer& AddHeader(const FString& Name, const FString& Value)
		{
			Headers.Add(Name, Value);
			return *this;
		}

		/**
		 * Add a lazy header to the injection set. The provider function is evaluated
		 * at call time to retrieve the header value dynamically.
		 * Returns *this for chaining.
		 * @param Name           Header name.
		 * @param Provider       Function that returns the header value when invoked.
		 */
		FInjectHeaderLayer& LazyHeader(const FString& Name, TFunction<FString()> Provider)
		{
			LazyHeaders.Add(Name, MoveTemp(Provider));
			return *this;
		}

		/**
		 * Add an async lazy header to the injection set. The provider coroutine is
		 * awaited at call time to retrieve the header value dynamically.
		 * Returns *this for chaining.
		 * @param Name           Header name.
		 * @param Provider       Async function (coroutine) that returns the header value when awaited.
		 */
		FInjectHeaderLayer& LazyHeader(const FString& Name, FLazyHeaderProvider Provider)
		{
			AsyncLazyHeaders.Add(Name, MoveTemp(Provider));
			return *this;
		}

		virtual TSharedRef<FHttpClient> Wrap(TSharedRef<FHttpClient> Inner) const override
		{
			return MakeShared<FInjectHeaderService>(Inner, Headers, LazyHeaders, AsyncLazyHeaders, bOverrideExisting);
		}

		virtual ~FInjectHeaderLayer() override = default;

	private:
		TMap<FString, FString> Headers;
		TMap<FString, TFunction<FString()>> LazyHeaders;
		TMap<FString, FLazyHeaderProvider> AsyncLazyHeaders;
		bool bOverrideExisting;

		/**
		 * Internal service wrapper that performs header injection on each request.
		 */
		class FInjectHeaderService : public FHttpClient
		{
		public:
			FInjectHeaderService(
				const TSharedRef<FHttpClient>& InInner,
				const TMap<FString, FString>& InHeaders,
				const TMap<FString, TFunction<FString()>>& InLazyHeaders,
				const TMap<FString, FLazyHeaderProvider>& InAsyncLazyHeaders,
				const bool bInOverrideExisting)
				: InnerService(InInner)
				  , Headers(InHeaders)
				  , LazyHeaders(InLazyHeaders)
				  , AsyncLazyHeaders(InAsyncLazyHeaders)
				  , bOverrideExisting(bInOverrideExisting)
			{
			}

			virtual UE5Coro::TCoroutine<TResult<FHttpResponse>> Call(
				const FHttpRequest& Request) override
			{
				// Create a copy of the request to inject headers
				FHttpRequest ModifiedRequest = Request;

				// Merge configured static headers into the request
				for (const auto& Kvp : Headers)
				{
					if (bOverrideExisting)
					{
						// Always set/replace the header
						ModifiedRequest.Headers.FindOrAdd(Kvp.Key) = Kvp.Value;
					}
					else
					{
						// Only add if the key does not already exist
						if (!ModifiedRequest.Headers.Contains(Kvp.Key))
						{
							ModifiedRequest.Headers.Add(Kvp.Key, Kvp.Value);
						}
					}
				}

				// Merge configured lazy headers into the request (evaluated at call time)
				for (const auto& Kvp : LazyHeaders)
				{
					if (bOverrideExisting)
					{
						// Always set/replace the header with the lazily evaluated value
						ModifiedRequest.Headers.FindOrAdd(Kvp.Key) = Kvp.Value();
					}
					else
					{
						// Only add if the key does not already exist
						// Evaluate the provider only when the header will actually be added
						if (!ModifiedRequest.Headers.Contains(Kvp.Key))
						{
							ModifiedRequest.Headers.Add(Kvp.Key, Kvp.Value());
						}
					}
				}

				// Merge configured async lazy headers into the request (awaited at call time)
				for (const auto& Kvp : AsyncLazyHeaders)
				{
					if (bOverrideExisting)
					{
						// Always set/replace the header with the async lazily evaluated value
						ModifiedRequest.Headers.FindOrAdd(Kvp.Key) = co_await Kvp.Value();
					}
					else
					{
						// Only add if the key does not already exist
						// Await the provider only when the header will actually be added
						if (!ModifiedRequest.Headers.Contains(Kvp.Key))
						{
							ModifiedRequest.Headers.Add(Kvp.Key, co_await Kvp.Value());
						}
					}
				}

				// Forward the modified request to the inner service
				co_return co_await InnerService->Call(ModifiedRequest);
			}

		private:
			TSharedRef<FHttpClient> InnerService;
			TMap<FString, FString> Headers;
			TMap<FString, TFunction<FString()>> LazyHeaders;
			TMap<FString, FLazyHeaderProvider> AsyncLazyHeaders;
			bool bOverrideExisting;
		};
	};
}
