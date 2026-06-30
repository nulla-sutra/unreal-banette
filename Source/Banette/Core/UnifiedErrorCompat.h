// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "Experimental/UnifiedError/UnifiedError.h"
#include "Runtime/Launch/Resources/Version.h"

// UnifiedError is still experimental and its declaration macros changed between UE 5.7 and 5.8.
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8)

#define BANETTE_DECLARE_ERROR_MODULE(DeclareApi, ModuleNamespace) \
	UE_DECLARE_ERROR_MODULE(DeclareApi, ModuleNamespace)

#define BANETTE_DECLARE_ERROR(DeclareApi, ModuleNamespace, ErrorCode, ErrorName, FormatString) \
	UE_DECLARE_ERROR(DeclareApi, ModuleNamespace, ErrorCode, ErrorName, FormatString)

#define BANETTE_DEFINE_ERROR_MODULE(ModuleNamespace) \
	UE_DEFINE_ERROR_MODULE(ModuleNamespace)

#define BANETTE_DEFINE_ERROR(ModuleNamespace, ErrorName) \
	UE_DEFINE_ERROR(ModuleNamespace, ErrorName)

#define BANETTE_MAKE_ERROR(ModuleNamespace, ErrorName) \
	::ModuleNamespace::ErrorName()

#else

#define BANETTE_DECLARE_ERROR_MODULE(DeclareApi, ModuleNamespace) \
	UE_DECLARE_ERROR_MODULE(DeclareApi, ModuleNamespace)

#define BANETTE_DECLARE_ERROR(DeclareApi, ModuleNamespace, ErrorCode, ErrorName, FormatString) \
	UE_DECLARE_ERROR(DeclareApi, ErrorName, ErrorCode, ModuleNamespace, NSLOCTEXT(#ModuleNamespace, #ErrorName, FormatString))

#define BANETTE_DEFINE_ERROR_MODULE(ModuleNamespace) \
	UE_DEFINE_ERROR_MODULE(ModuleNamespace)

#define BANETTE_DEFINE_ERROR(ModuleNamespace, ErrorName) \
	UE_DEFINE_ERROR(ErrorName, ModuleNamespace)

#define BANETTE_MAKE_ERROR(ModuleNamespace, ErrorName) \
	::UE::UnifiedError::ModuleNamespace::ErrorName::MakeError()

#endif
