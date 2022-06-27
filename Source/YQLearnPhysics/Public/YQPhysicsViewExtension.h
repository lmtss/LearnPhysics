#pragma once

#include "SceneViewExtension.h"

class FYQPhysicsViewExtension : public FSceneViewExtensionBase
{
public:
    FYQPhysicsViewExtension(const FAutoRegister& AutoReg);
    ~FYQPhysicsViewExtension();

    virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {};
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

    virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override {return true;};

    virtual void PostRenderBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
};