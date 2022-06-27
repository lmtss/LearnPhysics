#include "YQPhysicsViewExtension.h"

#include "RenderGraphUtils.h"

#include "GPUBasedBVH.h"

#include "YQPhysicsScene.h"

static TAutoConsoleVariable<int32> CVarVisualBVHLayer(
    TEXT("r.VisualBVHLayer"), 0,
    TEXT(".\n"),
    ECVF_RenderThreadSafe);


FYQPhysicsViewExtension::FYQPhysicsViewExtension(const FAutoRegister& AutoReg) : FSceneViewExtensionBase(AutoReg)
{

}

FYQPhysicsViewExtension::~FYQPhysicsViewExtension()
{

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