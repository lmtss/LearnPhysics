#pragma once
#include "Math/UnrealMathUtility.h"

namespace FYQStiffness
{
	// �ο�chaos�Ĺ�ʽ
	float CalcPBDStiffness(const float DeltaTime, const int NumIterations, const float Stiffness)
	{
		const float Exponent = 1.0 / (float)NumIterations;
		const float EmpiricalBase = 1000.0;		// chaos��Ĭ�Ͼ���ֵ

		return 1.0 - powf(
			(1.0 - powf(
				EmpiricalBase
				, FMath::Clamp(Stiffness, 0.0, 1.0) - 1
			))
			, Exponent
		);
	}
}

