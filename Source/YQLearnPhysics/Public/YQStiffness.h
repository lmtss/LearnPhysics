#pragma once
#include "Math/UnrealMathUtility.h"

namespace FYQStiffness
{
	// 参考chaos的公式
	float CalcPBDStiffness(const float DeltaTime, const int NumIterations, const float Stiffness)
	{
		const float Exponent = 1.0 / (float)NumIterations;
		const float EmpiricalBase = 1000.0;		// chaos的默认经验值

		return 1.0 - powf(
			(1.0 - powf(
				EmpiricalBase
				, FMath::Clamp(Stiffness, 0.0, 1.0) - 1
			))
			, Exponent
		);
	}
}

