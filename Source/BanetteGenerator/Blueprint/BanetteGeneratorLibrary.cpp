// Copyright 2019-Present tarnishablec. All Rights Reserved.


#include "BanetteGenerator/Blueprint/BanetteGeneratorLibrary.h"
#include "BanetteGenerator/generator/bindings.h"

void UBanetteGeneratorLibrary::GenerateOpenApi(const FString OpenApiPath,
                                               const FString OutputDir,
                                               const FString FileName,
                                               const FString ModuleName,
                                               const FString ExtraHeaders)
{
	using namespace banette::ffi::generator::openapi;
	generate(StringCast<ANSICHAR>(*OpenApiPath).Get(),
	         StringCast<ANSICHAR>(*OutputDir).Get(),
	         StringCast<ANSICHAR>(*FileName).Get(),
	         StringCast<ANSICHAR>(*ModuleName).Get(), StringCast<ANSICHAR>(*ExtraHeaders).Get());
}
