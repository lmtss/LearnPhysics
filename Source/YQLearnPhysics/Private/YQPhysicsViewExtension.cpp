#include "YQPhysicsViewExtension.h"

#include "RenderGraphUtils.h"

#include "GPUBasedBVH.h"

#include "YQPhysicsScene.h"


#include "Renderer/Private/SceneRendering.h"
#include "Renderer/Private/ScenePrivate.h"

static TAutoConsoleVariable<int32> CVarVisualBVHLayer(
    TEXT("r.VisualBVHLayer"), 0,
    TEXT(".\n"),
    ECVF_RenderThreadSafe);


FYQPhysicsViewExtension::FYQPhysicsViewExtension(const FAutoRegister& AutoReg, UWorld* InWorld, FYQPhysicsSimulator* InPhysicsSimulator, FYQPhysicsScene* InPhysicsScene) 
    : FWorldSceneViewExtension(AutoReg, InWorld)
    , PhysicsSimulator(InPhysicsSimulator)
    , PhysicsScene(InPhysicsScene)
    , bEnableSimulate(true)
{

}

FYQPhysicsViewExtension::~FYQPhysicsViewExtension()
{

}

bool FYQPhysicsViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
    
    return bEnableSimulate;//&& Context.Scene == GetWorld()->Scene;
}

void FYQPhysicsViewExtension::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
    if (InView.bIsSceneCapture || InView.bIsVirtualTexture || InView.bIsReflectionCapture || InView.bIsPlanarReflection || InView.bIsViewInfo == false) // 
    {
        return;
    }

    /*PhysicsScene->Tick(RHICmdList);
    PhysicsSimulator->Tick_RenderThread(RHICmdList, 0.03);*/
}

void FYQPhysicsViewExtension::PostRenderBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
    /*FYQPhysicsScene* PhysicsScene = FYQPhysicsScene::Get();
    
    if (PhysicsScene->IsBVHInit && CVarVisualBVHLayer.GetValueOnRenderThread() != 0)
    {
        FGPUBvhBuffers& GPUBvhBuffers = PhysicsScene->GetGPUBvhBuffers();
        VisualizeBVH_RenderThread
        (
            RHICmdList
            , InView
            , GPUBvhBuffers.BVPositionBufferA[1].SRV
            , GPUBvhBuffers.BVPositionBufferB[1].SRV
            , GPUBvhBuffers.BVPositionBufferA[0].SRV
            , GPUBvhBuffers.BVPositionBufferB[0].SRV
            , GPUBvhBuffers.BVHChildIDBuffer.SRV
            , PhysicsScene->GetPhysicsSceneViewBuffer().SRV
            , PhysicsScene->GetVisualizeBVHNodeBuffer()
            , PhysicsScene->GetIndirectArgBuffer()
            , CVarVisualBVHLayer.GetValueOnRenderThread()
        );
    }*/
    
    //VisualizeBVH_RenderThread(RHICmdList, InView, PhysicsScene->GetBVPositionBufferA().SRV, PhysicsScene->GetBVPositionBufferB().SRV);
}