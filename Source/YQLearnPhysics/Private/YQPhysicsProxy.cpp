#include "YQPhysicsProxy.h"

FYQPhysicsProxy::FYQPhysicsProxy()
: RenderSceneProxy(nullptr)
, BufferIndexOffset(0)
, bIsRegisteredInGPUPhysicsScene(false)
, bHasRenderSceneProxy(false)
, bEnableGravity(false)
, PhysicsSceneInfo(nullptr)

{
    
}