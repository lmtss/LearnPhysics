#pragma once
#include "CommonRenderResources.h"


void YQTest_InitializeBuffer(FRHICommandList& RHICmdList);


void YQTest_UpdateBuffer(FRHICommandList& RHICmdList);

void YQInitializeSizeBuffer(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef RWFreeIDListSizes
	, uint32 LengthToClear
	, uint32 Value
);