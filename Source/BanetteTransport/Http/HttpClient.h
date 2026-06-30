// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once
#include "Banette.h"
#include "Containers/Map.h"
#include "Containers/Array.h"

// UnifiedError declarations for HTTP transport failures
// Expose stable error codes so callers can branch on explicit conditions.
BANETTE_DECLARE_ERROR_MODULE(BANETTETRANSPORT_API, Banette::Transport::Http);

BANETTE_DECLARE_ERROR(BANETTETRANSPORT_API, Banette::Transport::Http, 1, InvalidUrl,
                      "Invalid or empty URL.");

BANETTE_DECLARE_ERROR(BANETTETRANSPORT_API, Banette::Transport::Http, 2, RequestCreationFailed,
                      "Failed to create HTTP request.");

BANETTE_DECLARE_ERROR(BANETTETRANSPORT_API, Banette::Transport::Http, 3, ConnectionFailed,
                      "HTTP connection failed.");

BANETTE_DECLARE_ERROR(BANETTETRANSPORT_API, Banette::Transport::Http, 4, NoResponse,
                      "No HTTP response received.");


namespace Banette::Transport::Http
{
	using namespace Banette::Core;

	// Supported HTTP methods for the transport layer.
	enum class EHttpMethod : uint8
	{
		Get,
		Post,
		Put,
		Delete,
		Patch,
		Head
	};

	// Helper macro to handle commas in macro arguments
#define BANETTE_COMMA ,

	// Macro to define a mutable field with a builder pattern method
#define BANETTE_BUILDER_FIELD(StructType, FieldType, FieldName) \
	mutable FieldType FieldName; \
	StructType& With_##FieldName(const FieldType& In##FieldName) \
	{ \
		FieldName = In##FieldName; \
		return *this; \
	}

	// Macro to define a mutable field with a default value and builder pattern method
#define BANETTE_BUILDER_FIELD_DEFAULT(StructType, FieldType, FieldName, DefaultValue) \
	mutable FieldType FieldName = DefaultValue; \
	StructType& With_##FieldName(const FieldType& In##FieldName) \
	{ \
		FieldName = In##FieldName; \
		return *this; \
	}

	// Macro to add a move semantic builder method for an existing field
#define BANETTE_BUILDER_MOVE(StructType, FieldType, FieldName) \
	StructType& With_##FieldName(FieldType&& In##FieldName) \
	{ \
		FieldName = MoveTemp(In##FieldName); \
		return *this; \
	}

	// Request data for HTTP calls.
	struct BANETTETRANSPORT_API FHttpRequest
	{
		BANETTE_BUILDER_FIELD(FHttpRequest, FString, Url)
		BANETTE_BUILDER_FIELD_DEFAULT(FHttpRequest, EHttpMethod, Method, EHttpMethod::Get)
		BANETTE_BUILDER_FIELD_DEFAULT(FHttpRequest, TMap<FString BANETTE_COMMA FString>, Headers, {})
		BANETTE_BUILDER_FIELD_DEFAULT(FHttpRequest, FString, ContentType, "application/json")
		BANETTE_BUILDER_FIELD(FHttpRequest, TArray<uint8>, Body)
		BANETTE_BUILDER_MOVE(FHttpRequest, TArray<uint8>, Body)
		BANETTE_BUILDER_FIELD_DEFAULT(FHttpRequest, float, TimeoutSeconds, 0.f)

		// Adds a single header and returns a reference for chaining.
		FHttpRequest& AddHeader(const FString& Key, const FString& Value)
		{
			Headers.Add(Key, Value);
			return *this;
		}
	};

#undef BANETTE_BUILDER_FIELD
#undef BANETTE_BUILDER_FIELD_DEFAULT
#undef BANETTE_BUILDER_MOVE
#undef BANETTE_COMMA

	// Response data for HTTP calls.
	struct BANETTETRANSPORT_API FHttpResponse
	{
		// Final URL (after redirects if any, according to the HTTP module behavior).
		FString Url;

		// HTTP status code. 0 means no valid response was received.
		int32 StatusCode = 0;

		// Response headers.
		TMap<FString, FString> Headers;

		// Response payload.
		TArray<uint8> Body;

		// Parsed/echoed content type if present.
		FString ContentType;

		// Whether the engine reported a successful connection and a response object was received.
		bool bSucceeded = false;
	};

	static FString ToVerb(const EHttpMethod Method);

	class BANETTETRANSPORT_API FHttpClient : public TService<FHttpRequest, FHttpResponse>
	{
	public:
		virtual ~FHttpClient() override = default;

		// Perform an HTTP call using Unreal's HTTP module.
		virtual UE5Coro::TCoroutine<TResult<FHttpResponse>> Call(
			const FHttpRequest& Request) override;
	};
}
