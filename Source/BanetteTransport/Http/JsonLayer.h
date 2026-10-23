// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Banette/Banette.h"
#include "JsonObjectConverter.h"
#include "BanetteTransport/Http/HttpClient.h"
#include "UE5Coro.h"

namespace Banette::Transport::Http
{
	using namespace Banette::Core;

	/// Container for JSON body data, holding both raw bytes and parsed JSON.
	///
	/// The raw bytes are always preserved, even if JSON parsing fails.
	/// The JsonString will be empty if the body could not be parsed as valid JSON.
	struct FJsonBody
	{
		/// Original raw response bytes.
		TArray<uint8> RawBytes;

		/// Parsed JSON String. May be empty if parsing failed.
		FString JsonString;
	};

	/// HTTP response with JSON body.
	///
	/// Similar to FHttpResponse but with FJsonBody instead of TArray<uint8>.
	struct FHttpJsonResponse
	{
		/// Final URL (after redirects if any).
		FString Url;

		/// HTTP status code. 0 means no valid response was received.
		int32 StatusCode = 0;

		/// Response headers.
		TMap<FString, FString> Headers;

		/// Response body containing both raw bytes and parsed JSON.
		FJsonBody Body;

		/// Parsed/echoed content type if present.
		FString ContentType;

		/// Whether the engine reported a successful connection.
		bool bSucceeded = false;

		template <typename T>
		bool GetContent(TArray<T>& OutArr) const
		{
			return FJsonObjectConverter::JsonArrayStringToUStruct(Body.JsonString, &OutArr);
		}

		template <typename T>
		bool GetContent(T& OutStruct) const
		{
			return FJsonObjectConverter::JsonObjectStringToUStruct(Body.JsonString, &OutStruct);
		}
	};

	/// Service type that returns HTTP responses with JSON-parsed bodies.
	class FHttpJsonService : public TService<FHttpRequest, FHttpJsonResponse>
	{
	public:
		virtual ~FHttpJsonService() override = default;
	};

	/// Layer that transforms FHttpService into FHttpJsonService.
	///
	/// This layer wraps an FHttpService and automatically parses the response
	/// body as JSON, constructing an FHttpJsonResponse with both raw bytes
	/// and parsed JSON data.
	///
	/// Usage example:
	/// @code
	/// using namespace Banette::Pipeline;
	/// using namespace Banette::Transport::Http;
	///
	/// TSharedRef<FHttpService> Base = MakeShared<FHttpService>();
	/// FJsonLayer JsonLayer;
	///
	/// auto Builder = TServiceBuilder<>::New(Base)
	///     .Layer(JsonLayer);
	///
	/// TSharedRef<FHttpJsonService> JsonService = Builder.Build();
	///
	/// // Now responses will have Body as FJsonBody with both raw bytes and parsed JSON
	/// @endcode
	class FJsonLayer : public TLayer<FHttpClient, FHttpJsonService>
	{
	public:
		FJsonLayer() = default;

		virtual TSharedRef<FHttpJsonService> Wrap(TSharedRef<FHttpClient> Inner) const override
		{
			return MakeShared<FJsonService>(Inner);
		}

		virtual ~FJsonLayer() override = default;

	private:
		/// Internal service that forwards requests to the inner FHttpService
		/// and converts responses to FHttpJsonResponse with parsed JSON.
		class FJsonService : public FHttpJsonService
		{
		public:
			explicit FJsonService(const TSharedRef<FHttpClient>& InInner)
				: InnerService(InInner)
			{
			}

			virtual UE5Coro::TCoroutine<TResult<FHttpJsonResponse>> Call(
				const FHttpRequest& Request) override
			{
				// Forward the request to the inner HTTP service
				auto Result = co_await InnerService->Call(Request);

				// If the request failed, propagate the error
				if (!Result.IsValid())
				{
					co_return MakeError(Result.GetError());
				}

				// Convert FHttpResponse to FHttpJsonResponse
				const FHttpResponse& HttpResponse = Result.GetValue();
				FHttpJsonResponse JsonResponse;

				JsonResponse.Url = HttpResponse.Url;
				JsonResponse.StatusCode = HttpResponse.StatusCode;
				JsonResponse.Headers = HttpResponse.Headers;
				JsonResponse.ContentType = HttpResponse.ContentType;
				JsonResponse.bSucceeded = HttpResponse.bSucceeded;

				// Preserve the raw bytes
				JsonResponse.Body.RawBytes = HttpResponse.Body;

				// Attempt to parse the body as JSON
				JsonResponse.Body.JsonString = ParseJsonFromBytes(HttpResponse.Body);

				co_return MakeValue(JsonResponse);
			}

		private:
			TSharedRef<FHttpClient> InnerService;

			/// Attempts to parse JSON from raw bytes.
			/// Returns nullptr if parsing fails.
			static FString ParseJsonFromBytes(const TArray<uint8>& Bytes)
			{
				if (Bytes.Num() == 0)
				{
					return "";
				}

				const auto Utf8String = StringCast<TCHAR>(
					reinterpret_cast<const ANSICHAR*>(Bytes.GetData()),
					Bytes.Num()
				);
				return FString(Utf8String.Length(), Utf8String.Get());
			}
		};
	};

	/// Converts a JSON string to UTF-8 encoded bytes.
	static TArray<uint8> JsonStringToBytes(const FString& JsonString)
	{
		const FTCHARToUTF8 Utf8Converter(*JsonString);
		TArray<uint8> Bytes;
		Bytes.Append(reinterpret_cast<const uint8*>(Utf8Converter.Get()), Utf8Converter.Length());
		return Bytes;
	}

	template <typename T>
	static TArray<uint8> ToBytes(const T& Payload)
	{
		FString JsonString;
		if (FJsonObjectConverter::UStructToJsonObjectString(Payload, JsonString))
		{
			return JsonStringToBytes(JsonString);
		}
		return {};
	}

	template <typename T>
	static TArray<uint8> ToBytes(const TArray<T>& Payload)
	{
		FString JsonString;
		if (FJsonObjectConverter::JsonArrayStringToUStruct(Payload, JsonString))
		{
			return JsonStringToBytes(JsonString);
		}
		return {};
	}
}
