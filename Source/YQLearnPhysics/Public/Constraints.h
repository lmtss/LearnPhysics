#pragma once

enum class EConstraintType
{
    Distance = 0,
    DistanceBending,
    Bending,
    ShapeMatch,
    Num
};

enum class EConstraintSourceType
{
    // 根据网格数据自动生成数据
    // 
    Mesh = 0,   
    CustomBuffer,
    Num
};