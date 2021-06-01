#pragma once

#include <Math/XMath.h>


class Color
{
public:
    Color(float r, float g, float b, float a)
        : m_Color(r, g, b, a)
    {
    }

    Color(const XMath::Vector4& v)
        : m_Color(v)
    {
    }

    static Color Black() { return Color(0.0f, 0.0f, 0.0f, 1.0f); }
    static Color Blue() { return Color(0.0f, 0.0f, 1.0f, 1.0f); }
    static Color Clear() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }
    static Color Cyan() { return Color(0.0f, 1.0f, 1.0f, 1.0f); }
    static Color Gray() { return Color(0.5f, 0.5f, 0.5f, 1.0f); }
    static Color Green() { return Color(0.0f, 1.0f, 0.0f, 1.0f); }
    static Color Grey() { return Color(0.5f, 0.5f, 0.5f, 1.0f); }
    static Color Magenta() { return Color(1.0f, 0.0f, 1.0f, 1.0f); }
    static Color Red() { return Color(1.0f, 0.0f, 0.0f, 1.0f); }
    static Color White() { return Color(1.0f, 1.0f, 1.0f, 1.0f); }
    static Color Yellow() { return Color(1.0f, 0.92f, 0.016f, 1.0f); }

    float& r() { return m_Color.x(); }
    const float& r() const { return m_Color.x(); }

    float& g() { return m_Color.y(); }
    const float& g() const { return m_Color.y(); }

    float& b() { return m_Color.z(); }
    const float& b() const { return m_Color.z(); }

    float& a() { return m_Color.w(); }
    const float& a() const { return m_Color.w(); }

    float& operator[](size_t idx) { return m_Color[idx]; }
    const float& operator[](size_t idx) const { return m_Color[idx]; }

    operator XMath::Vector4&() { return m_Color; }
    operator float*() { return m_Color.data(); }
    operator const XMath::Vector4&() const { return m_Color; }
    operator const float*() const { return m_Color.data(); }


private:
    XMath::Vector4 m_Color;
};

class Rect
{
public:
    Rect(float x, float y, float w, float h)
        : m_XYWH(x, y, w, h)
    {
    }

    float& x() { return m_XYWH.x(); }
    const float& x() const { return m_XYWH.x(); }

    float& y() { return m_XYWH.y(); }
    const float& y() const { return m_XYWH.y(); }

    float& width() { return m_XYWH.z(); }
    const float& width() const { return m_XYWH.z(); }

    float& height() { return m_XYWH.w(); }
    const float& height() const { return m_XYWH.w(); }

private:
    XMath::Vector4 m_XYWH;
};
