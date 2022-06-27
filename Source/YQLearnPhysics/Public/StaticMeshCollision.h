#pragma once
#include "YQPhysicsProxy.h"


// 静态网格代理
// 不可破坏，不可移动
class FStaticMeshPhysicsProxy final : public FYQPhysicsProxy {

public:
	FStaticMeshPhysicsProxy() : FYQPhysicsProxy() {

	}

	virtual void GetCollisionInfo(FYQCollisionInfo& OutInfo) const {
		OutInfo.Type = EYQCollisionType::StaticMesh;
		OutInfo.BoundingBox = BoundingBox;
	}

	FBox BoundingBox;
};