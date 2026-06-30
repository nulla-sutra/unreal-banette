// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Banette.h"
#include "BanetteTransport/Http/HttpClient.h"
#include "UE5Coro.h"

namespace Banette::Transport::Http
{
	using namespace Banette::Core;

	/// Type alias for async lazy origin providers.
	/// A function that returns a coroutine yielding the origin URL.
	using FLazyOriginProvider = TFunction<UE5Coro::TCoroutine<FString>()>;

	/// Layer that prefixes request URLs with a configured origin (base URL).
	///
	/// This layer wraps an FHttpService and prepends the origin to each
	/// request's URL if the URL does not already start with http:// or https://.
	///
	/// URL concatenation handles trailing/leading slashes:
	/// - origin "https://example.com" + url "/a/b" -> "https://example.com/a/b"
	/// - origin "https://example.com/" + url "a/b" -> "https://example.com/a/b"
	/// - origin "https://example.com/" + url "/a/b" -> "https://example.com/a/b"
	/// - origin "https://example.com" + url "a/b" -> "https://example.com/a/b"
	///
	/// If the origin is empty and the request URL is relative, the layer
	/// returns an InvalidUrl error without calling the inner service.
	///
	/// Usage example (static origin):
	/// @code
	/// using namespace Banette::Pipeline;
	/// using namespace Banette::Transport::Http;
	/// using namespace Banette::Kit;
	///
	/// TSharedRef<FHttpService> Base = MakeShared<FHttpService>();
	/// FHttpOriginLayer OriginLayer(TEXT("https://someorigin.com"));
	///
	/// auto Builder = TServiceBuilder<>::New(Base)
	///     .Layer(OriginLayer);
	///
	/// TSharedRef<FHttpService> ServiceWithOrigin = Builder.Build();
	/// @endcode
	///
	/// Usage example (async origin provider):
	/// @code
	/// using namespace Banette::Pipeline;
	/// using namespace Banette::Transport::Http;
	/// using namespace Banette::Kit;
	///
	/// TSharedRef<FHttpService> Base = MakeShared<FHttpService>();
	/// FHttpOriginLayer OriginLayer([]() -> UE5Coro::TCoroutine<FString> {
	///     // Dynamically resolve origin URL (e.g., from config service)
	///     co_return TEXT("https://dynamic-origin.com");
	/// });
	///
	/// TSharedRef<FHttpService> ServiceWithOrigin = TServiceBuilder<>::New(Base)
	///     .Layer(OriginLayer)
	///     .Build();
	/// @endcode
	class FHttpOriginLayer : public TLayer<FHttpClient, FHttpClient>
	{
	public:
		/**
		 * Construct an origin-prefixing layer with a static origin.
		 * @param InOrigin The base URL to prefix (e.g., "https://example.com").
		 */
		explicit FHttpOriginLayer(const FString& InOrigin = FString())
			: Origin(InOrigin)
		{
		}

		/**
		 * Construct an origin-prefixing layer with an async origin provider.
		 * The provider coroutine is awaited at call time to dynamically resolve the origin URL.
		 * @param InOriginProvider Async function (coroutine) that returns the origin URL when awaited.
		 */
		explicit FHttpOriginLayer(FLazyOriginProvider InOriginProvider)
			: Origin()
			, OriginProvider(MoveTemp(InOriginProvider))
		{
		}

		virtual TSharedRef<FHttpClient> Wrap(TSharedRef<FHttpClient> Inner) const override
		{
			return MakeShared<FHttpOriginService>(Inner, Origin, OriginProvider);
		}

		virtual ~FHttpOriginLayer() override = default;

	private:
		FString Origin;
		FLazyOriginProvider OriginProvider;

		/**
		 * Internal service wrapper that performs URL prefixing on each request.
		 */
		class FHttpOriginService : public FHttpClient
		{
		public:
			FHttpOriginService(
				const TSharedRef<FHttpClient>& InInner,
				const FString& InOrigin,
				const FLazyOriginProvider& InOriginProvider)
				: InnerService(InInner)
				, Origin(InOrigin)
				, OriginProvider(InOriginProvider)
			{
			}

			virtual UE5Coro::TCoroutine<TResult<FHttpResponse>> Call(
				const FHttpRequest& Request) override
			{
				// Check if the URL is already absolute (starts with http:// or https://)
				if (IsAbsoluteUrl(Request.Url))
				{
					// Pass through unchanged
					co_return co_await InnerService->Call(Request);
				}

				// Resolve the origin: use cached value, async provider, or static origin
				FString ResolvedOrigin;
				if (OriginProvider.IsSet())
				{
					// Check the cache first (thread-safe read)
					{
						FScopeLock Lock(&CacheLock);
						if (CachedOrigin.IsSet())
						{
							ResolvedOrigin = CachedOrigin.GetValue();
						}
					}
					
					// If not cached, call provider and cache the result
					// Note: Multiple concurrent requests might call the provider during initialization,
					// but the cache will converge to a consistent value
					if (ResolvedOrigin.IsEmpty())
					{
						{
							FString ProviderResult = co_await OriginProvider();
							FScopeLock Lock(&CacheLock);
							// Cache the result only if it's not empty
							// (empty origins are not cached to allow retrying on the next request)
							if (!ProviderResult.IsEmpty())
							{
								CachedOrigin = ProviderResult;
							}
							ResolvedOrigin = ProviderResult;
						}
					}
				}
				else
				{
					ResolvedOrigin = Origin;
				}

				// URL is relative; we need to prefix with origin
				if (ResolvedOrigin.IsEmpty())
				{
					// Cannot construct a valid URL without an origin
					co_return MakeError(BANETTE_MAKE_ERROR(Banette::Transport::Http, InvalidUrl));
				}

				// Combine origin and relative URL
				const FHttpRequest ModifiedRequest = Request;
				ModifiedRequest.Url = CombineUrl(ResolvedOrigin, Request.Url);

				co_return co_await InnerService->Call(ModifiedRequest);
			}

		private:
			TSharedRef<FHttpClient> InnerService;
			FString Origin;
			FLazyOriginProvider OriginProvider;
			
			// Cache for OriginProvider result
			FCriticalSection CacheLock;
			TOptional<FString> CachedOrigin;

			/**
			 * Check if a URL is absolute (starts with http:// or https://).
			 */
			static bool IsAbsoluteUrl(const FString& Url)
			{
				return Url.StartsWith(TEXT("http://"), ESearchCase::IgnoreCase) ||
				       Url.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase);
			}

			/**
			 * Combine origin and relative path, handling trailing/leading slashes.
			 * Examples:
			 * - CombineUrl("https://example.com", "/a/b") -> "https://example.com/a/b"
			 * - CombineUrl("https://example.com/", "a/b") -> "https://example.com/a/b"
			 * - CombineUrl("https://example.com/", "/a/b") -> "https://example.com/a/b"
			 * - CombineUrl("https://example.com", "a/b") -> "https://example.com/a/b"
			 */
			static FString CombineUrl(const FString& BaseOrigin, const FString& RelativePath)
			{
				// Find where trailing slashes end in origin (trim from right)
				int32 OriginEnd = BaseOrigin.Len();
				while (OriginEnd > 0 && BaseOrigin[OriginEnd - 1] == TEXT('/'))
				{
					--OriginEnd;
				}
				FStringView NormalizedOrigin(BaseOrigin.GetCharArray().GetData(), OriginEnd);

				// Find where leading slashes end in a path (trim from left)
				int32 PathStart = 0;
				while (PathStart < RelativePath.Len() && RelativePath[PathStart] == TEXT('/'))
				{
					++PathStart;
				}
				const FStringView NormalizedPath(RelativePath.GetCharArray().GetData() + PathStart, RelativePath.Len() - PathStart);

				// Combine with a single slash
				if (NormalizedPath.Len() == 0)
				{
					return FString(NormalizedOrigin);
				}

				return FString::Printf(TEXT("%.*s/%.*s"),
					NormalizedOrigin.Len(), NormalizedOrigin.GetData(),
					NormalizedPath.Len(), NormalizedPath.GetData());
			}
		};
	};
}
