#pragma once

enum class EConstraintType
{
    Distance = 0,
    DistanceBending,
    ShapeMatch,
    Num
};

enum class EConstraintSourceType
{
    // �������������Զ���������
    // 
    Mesh = 0,   
    CustomBuffer,
    Num
};