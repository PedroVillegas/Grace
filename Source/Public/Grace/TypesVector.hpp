#pragma once

#include <cstdint>

#include <vulkan/vulkan.h>

namespace Grace
{

struct Int1
{
    int x;
};

struct Int2
{
    int x, y;

    Int2() = default;

    Int2(int x, int y) : x(x), y(y) {}

    explicit Int2(int xy) : x(xy), y(xy) {}
};

struct Int3
{
    int x, y, z;

    Int3() = default;

    Int3(int x, int y, int z) : x(x), y(y), z(z) {}

    Int3(int xy, int z) : x(xy), y(xy), z(z) {}

    explicit Int3(int xyz) : x(xyz), y(xyz), z(xyz) {}
};

struct Int4
{
    int x, y, z, w;

    Int4() = default;

    Int4(int x, int y, int z, int w) : x(x), y(y), z(z), w(w) {}

    Int4(int xyz, int w) : x(xyz), y(xyz), z(xyz), w(w) {}

    explicit Int4(int xyzw) : x(xyzw), y(xyzw), z(xyzw), w(xyzw) {}
};

struct UInt1
{
    uint32_t x;
};

struct UInt2
{
    uint32_t x, y;

    UInt2() = default;

    UInt2(uint32_t x, uint32_t y) : x(x), y(y) {}

    explicit UInt2(uint32_t xy) : x(xy), y(xy) {}
};

struct UInt3
{
    uint32_t x, y, z;

    UInt3() = default;

    UInt3(uint32_t x, uint32_t y, uint32_t z) : x(x), y(y), z(z) {}

    UInt3(uint32_t xy, uint32_t z) : x(xy), y(xy), z(z) {}

    explicit UInt3(uint32_t xyz) : x(xyz), y(xyz), z(xyz) {}
};

struct UInt4
{
    uint32_t x, y, z, w;

    UInt4() = default;

    UInt4(uint32_t x, uint32_t y, uint32_t z, uint32_t w) : x(x), y(y), z(z), w(w) {}

    UInt4(uint32_t xyz, uint32_t w) : x(xyz), y(xyz), z(xyz), w(w) {}

    explicit UInt4(uint32_t xyzw) : x(xyzw), y(xyzw), z(xyzw), w(xyzw) {}
};

struct Float1
{
    float x;
};

struct Float2
{
    float x, y;

    Float2() = default;

    Float2(float x, float y) : x(x), y(y) {}

    explicit Float2(float xy) : x(xy), y(xy) {}
};

struct Float3
{
    float x, y, z;

    Float3() = default;

    Float3(float x, float y, float z) : x(x), y(y), z(z) {}

    Float3(float xy, float z) : x(xy), y(xy), z(z) {}

    explicit Float3(float xyz) : x(xyz), y(xyz), z(xyz) {}
};

struct Float4
{
    float x, y, z, w;

    Float4() = default;

    Float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    Float4(float xyz, float w) : x(xyz), y(xyz), z(xyz), w(w) {}

    explicit Float4(float xyzw) : x(xyzw), y(xyzw), z(xyzw), w(xyzw) {}
};

struct ClearColourValue
{
    VkClearColorValue clear;

    ClearColourValue(const Float4& float4) : clear({ .float32 = { float4.x, float4.y, float4.z, float4.w } }) {}

    ClearColourValue(const Int4& int4) : clear({ .int32 = { int4.x, int4.y, int4.z, int4.w } }) {}

    ClearColourValue(const UInt4& uint4) : clear({ .uint32 = { uint4.x, uint4.y, uint4.z, uint4.w } }) {}
};

} // namespace Grace
