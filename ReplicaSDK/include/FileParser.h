#pragma once
#include <fstream>
#include <iostream>

#include "Assert.h"

int parseInt(std::fstream &fileStream)
{
    ASSERT(fileStream);
    int tmp;
    fileStream >> tmp;
    return tmp;
}

float parseFloat(std::fstream &fileStream)
{
    ASSERT(fileStream);
    float tmp;
    fileStream >> tmp;
    return tmp;
}

double parseDouble(std::fstream &fileStream)
{
    ASSERT(fileStream);
    double tmp;
    fileStream >> tmp;
    return tmp;
}

Eigen::Vector2d parseVector2d(std::fstream &fileStream)
{
    ASSERT(fileStream);
    Eigen::Vector2d tmp;
    for (int i = 0; i < 2; i++)
    {
        fileStream >> tmp(i);
    }
    return tmp;
}

Eigen::Vector3d parseVector3d(std::fstream &fileStream)
{
    ASSERT(fileStream);
    Eigen::Vector3d tmp;
    for (int i = 0; i < 3; i++)
    {
        fileStream >> tmp(i);
    }
    return tmp;
}

Eigen::Vector4d parseVector4d(std::fstream &fileStream)
{
    ASSERT(fileStream);
    Eigen::Vector4d tmp;
    for (int i = 0; i < 4; i++)
    {
        fileStream >> tmp(i);
    }
    return tmp;
}

Eigen::Matrix3d parseMatrix3d(std::fstream &fileStream)
{
    ASSERT(fileStream);
    Eigen::Matrix3d tmp;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            fileStream >> tmp(i, j);
        }
    }
    return tmp;
}

Eigen::Matrix4d parseMatrix4d(std::fstream &fileStream)
{
    ASSERT(fileStream);
    Eigen::Matrix4d tmp;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            fileStream >> tmp(i, j);
        }
    }
    return tmp;
}

