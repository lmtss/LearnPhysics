#pragma once
#include "YQPhysicsProxy.h"


// ��̬�������
// �����ƻ��������ƶ�
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