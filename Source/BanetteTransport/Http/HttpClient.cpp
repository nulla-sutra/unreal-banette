// Copyright 2019-Present tarnishablec. All Rights Reserved.

#include "Http/HttpClient.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "UE5Coro.h"

// Define UnifiedError module and error codes for HTTP transport
BANETTE_DEFINE_ERROR_MODULE(Banette::Transport::Http);

BANETTE_DEFINE_ERROR(Banette::Transport::Http, InvalidUrl);

BANETTE_DEFINE_ERROR(Banette::Transport::Http, RequestCreationFailed);

BANETTE_DEFINE_ERROR(Banette::Transport::Http, ConnectionFailed);

BANETTE_DEFINE_ERROR(Banette::Transport::Http, NoResponse);

namespace Banette::Transport::Http
{
	static FString ToVerb(const EHttpMethod Method)
	{
		switch (Method)
		{
		case EHttpMethod::Get: return TEXT("GET");
		case EHttpMethod::Post: return TEXT("POST");
		case EHttpMethod::Put: return TEXT("PUT");
		case EHttpMethod::Delete: return TEXT("DELETE");
		case EHttpMethod::Patch: return TEXT("PATCH");
		case EHttpMethod::Head: return TEXT("HEAD");
		default: return TEXT("GET");
		}
	}

	void ParseHeaders(const TArray<FString>& InAllHeaders, TMap<FString, FString>& OutHeaders)
	{
		OutHeaders.Reset();
		for (const FString& Line : InAllHeaders)
		{
			if (int32 ColonIndex = INDEX_NONE; Line.FindChar(TEXT(':'), ColonIndex) && ColonIndex > 0)
			{
				FString Key = Line.Left(ColonIndex).TrimStartAndEnd();
				FString Value = Line.Mid(ColonIndex + 1).TrimStartAndEnd();
				OutHeaders.Add(MoveTemp(Key), MoveTemp(Value));
			}
		}
	}

	UE5Coro::TCoroutine<TResult<FHttpResponse>>
	FHttpClient::Call(const FHttpRequest& Request)
	{
		// Validate URL
		if (Request.Url.IsEmpty())
			co_return MakeError(BANETTE_MAKE_ERROR(Banette::Transport::Http, InvalidUrl));

		// Create request
		auto* Module = &FHttpModule::Get();
		if (!Module)
			co_return MakeError(BANETTE_MAKE_ERROR(Banette::Transport::Http, RequestCreationFailed));

		const auto HttpReq = Module->CreateRequest();

		HttpReq->SetURL(Request.Url);
		HttpReq->SetVerb(ToVerb(Request.Method));
		if (Request.TimeoutSeconds > 0.f)
		{
			HttpReq->SetTimeout(Request.TimeoutSeconds);
		}

		// Apply headers
		for (auto&& Kvp : Request.Headers)
		{
			HttpReq->SetHeader(Kvp.Key, Kvp.Value);
		}
		if (!Request.ContentType.IsEmpty())
		{
			if (!Request.Headers.Contains(TEXT("Content-Type")))
			{
				HttpReq->SetHeader(TEXT("Content-Type"), Request.ContentType);
			}
		}

		// Apply body if present
		if (Request.Body.Num() > 0)
		{
			auto BodyCopy = Request.Body; // copy from const
			HttpReq->SetContent(MoveTemp(BodyCopy));
		}

		// Process asynchronously via UE5Coro's awaiter
		auto [Response, bConnected] = co_await UE5Coro::Http::ProcessAsync(HttpReq);

		if (!bConnected || !Response.IsValid())
			co_return MakeError(BANETTE_MAKE_ERROR(Banette::Transport::Http, ConnectionFailed));

		FHttpResponse Out;
		Out.Url = HttpReq->GetURL();
		Out.StatusCode = Response->GetResponseCode();
		Out.ContentType = Response->GetContentType();
		Out.bSucceeded = (Out.StatusCode >= 200 && Out.StatusCode < 300);

		// Headers
		ParseHeaders(Response->GetAllHeaders(), Out.Headers);

		// Body
		const TArray<uint8>& Content = Response->GetContent();
		Out.Body = Content;

		co_return MakeValue(MoveTemp(Out));
	}
}
