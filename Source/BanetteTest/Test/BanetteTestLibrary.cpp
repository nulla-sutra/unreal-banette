// Copyright 2019-Present tarnishablec. All Rights Reserved.


#include "BanetteTest/Test/BanetteTestLibrary.h"
#include "BanetteKit/Layers/RetryLayer.h"
#include "BanetteTransport/Http/HttpClient.h"
#include "Dom/JsonObject.h"

using namespace Banette::Transport::Http;
using namespace Banette::Pipeline;
using namespace Banette::Kit;

FVoidCoroutine UBanetteTestLibrary::Test(FJsonObjectWrapper& Json, FLatentActionInfo LatentInfo)
{
	const auto HttpService = MakeShared<FHttpClient>();

	const TRetryLayer<FHttpClient> RetryLayer({
		.MaxAttempts = 5,
		.DelayBetweenRetries = 0.5f,
	});

	const auto WrappedService =
		TServiceBuilder<>::New(HttpService)
		.Layer(RetryLayer)
		.Build();

	FHttpRequest Request;
	Request.Url = TEXT("https://httpbin.org/json");
	Request.Method = EHttpMethod::Get;

	if (auto Result = co_await WrappedService->Call(Request); Result.IsValid())
	{
		const auto& Response = Result.GetValue();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HTTP Request failed after retries"));
	}

	co_return;
}
