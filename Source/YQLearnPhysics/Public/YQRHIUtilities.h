#pragma once
#include "RenderResource.h"

struct FWriteToCPUBuffer
{
	uint8* MappedBuffer;
	FBufferRHIRef Buffer;
	FUnorderedAccessViewRHIRef UAV;
	uint32 NumBytes;

	FWriteToCPUBuffer() : NumBytes(0)
	{
	}

	~FWriteToCPUBuffer()
	{
		Release();
	}

	void Initialize(const TCHAR* InDebugName, uint32 BytesPerElement, uint32 NumElements, EPixelFormat Format, EBufferUsageFlags AdditionalUsage = BUF_None)
	{
		MappedBuffer = nullptr;
		NumBytes = BytesPerElement * NumElements;
		FRHIResourceCreateInfo CreateInfo(InDebugName);
		Buffer = RHICreateVertexBuffer(NumBytes, BUF_UnorderedAccess | BUF_KeepCPUAccessible | AdditionalUsage, ERHIAccess::UAVCompute | ERHIAccess::CPURead, CreateInfo);
		UAV = RHICreateUnorderedAccessView(Buffer, UE_PIXELFORMAT_TO_UINT8(Format));
	}

	void Release()
	{
		NumBytes = 0;
		Buffer.SafeRelease();
		UAV.SafeRelease();
	}

	uint8* Lock()
	{
		check(MappedBuffer == nullptr);
		check(IsValidRef(Buffer));
		MappedBuffer = (uint8*)RHILockBuffer(Buffer, 0, NumBytes, RLM_ReadOnly);
		return MappedBuffer;
	}

	void Unlock()
	{
		check(MappedBuffer);
		check(IsValidRef(Buffer));
		RHIUnlockBuffer(Buffer);
		MappedBuffer = nullptr;
	}
};