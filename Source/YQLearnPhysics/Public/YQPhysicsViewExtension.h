#pragma once

#include "SceneViewExtension.h"
#include "YQPhysicsSimulator.h"

class FYQPhysicsViewExtension : public FWorldSceneViewExtension
{
public:
    FYQPhysicsViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld, FYQPhysicsSimulator* InPhysicsSimulator, FYQPhysicsScene* InPhysicsScene);
    ~FYQPhysicsViewExtension();

    virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {};
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

    virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

    virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
    virtual void PostRenderBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;

    void EnableSimulate()
    {
        bEnableSimulate = true;
    }

    void DisableSimulate()
    {
        bEnableSimulate = false;
    }

private:
    FYQPhysicsSimulator* PhysicsSimulator;
    FYQPhysicsScene* PhysicsScene;

    bool bEnableSimulate;
};