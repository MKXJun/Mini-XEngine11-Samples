#pragma once

#include <Eigen/Geometry>

namespace XMath
{
    using Vector2 = Eigen::Vector2f;
	using Vector3 = Eigen::Vector3f;
    using Vector4 = Eigen::Matrix<float, 4, 1, Eigen::DontAlign>;
    using Vector4A = Eigen::Vector4f;
    using Vector2Int = Eigen::Vector2i;
    using Vector3Int = Eigen::Vector3i;

	using QuaternionA = Eigen::Quaternionf;
    using Quaternion = Eigen::Quaternion<float, Eigen::DontAlign>;

    using Matrix2x2 = Eigen::Matrix<float, 2, 2, Eigen::DontAlign>;
    using Matrix2x2A = Eigen::Matrix2f;
    using Matrix3x3 = Eigen::Matrix3f;
    using Matrix4x4 = Eigen::Matrix<float, 4, 4, Eigen::DontAlign>;
    using Matrix4x4A = Eigen::Matrix4f;


    using Vector4ArrayA = std::vector<Vector4A, Eigen::aligned_allocator<Vector4A>>;
    using QuaternionArrayA = std::vector<QuaternionA, Eigen::aligned_allocator<QuaternionA>>;
    using Matrix2x2ArrayA = std::vector<Matrix2x2A, Eigen::aligned_allocator<Matrix2x2A>>;
    using Matrix4x4ArrayA = std::vector<Matrix4x4A, Eigen::aligned_allocator<Matrix4x4A>>;

    constexpr float PI = 3.141592654f;

    namespace Scalar
    {
        inline float ConvertToRadians(float angles)
        {
            return angles * (PI / 180);
        }

        inline float ConvertToDegrees(float radians)
        {
            return radians * (180 / PI);
        }

        inline float Clamp(float val, float minVal, float maxVal)
        {
            return (std::min)((std::max)(val, minVal), maxVal);
        }

        inline float Lerp(float a, float b, float t)
        {
            return a + (b - a) * t;
        }

        inline float ModAngles(float angleInRadians)
        {
            // Normalize the range from 0.0f to 2 * PI
            angleInRadians = angleInRadians + PI;
            // Perform the modulo, unsigned
            float fTemp = fabsf(angleInRadians);
            fTemp = fTemp - (2 * PI * static_cast<float>(static_cast<int32_t>(fTemp / (2 * PI))));
            // Restore the number to the range of -PI to PI-epsilon
            fTemp = fTemp - PI;
            // If the modulo'd value was negative, restore negation
            if (angleInRadians < 0.0f) {
                fTemp = -fTemp;
            }
            return fTemp;
        }
    }

    namespace Vector
    {
        inline Vector3 TransformCoord(const Vector3& coord, const Matrix4x4A& T)
        {
            Vector4A v = coord.homogeneous();
            v = T * v;
            return v.head<3>();
        }

        inline Vector3 TransformNormal(const Vector3& normal, const Matrix4x4A& T)
        {
            Vector4A v = normal.homogeneous();
            v.w() = 0.0f;
            v = T * v;
            return v.head<3>();
        }

        inline Vector4A Transform(const Vector4A& vec, const Matrix4x4A& T)
        {
            return T * vec;
        }
    }

    namespace Quat
    {
        inline QuaternionA RotationRollPitchYaw(const Vector3& anglesInRadians)
        {
            Vector4A Sign = { 1.0f, -1.0f, -1.0f, 1.0f };
            Vector4A HalfAngles = (Vector4::Ones() * 0.5f).cwiseProduct(anglesInRadians.homogeneous());
            Vector4A SinAngles = HalfAngles.array().sin();
            Vector4A CosAngles = HalfAngles.array().cos();

            Vector4A P0(SinAngles(0), CosAngles(0), CosAngles(0), CosAngles(0));
            Vector4A Y0(CosAngles(1), SinAngles(1), CosAngles(1), CosAngles(1));
            Vector4A R0(CosAngles(2), CosAngles(2), SinAngles(2), CosAngles(2));
            Vector4A P1(CosAngles(0), SinAngles(0), SinAngles(0), SinAngles(0));
            Vector4A Y1(SinAngles(1), CosAngles(1), SinAngles(1), SinAngles(1));
            Vector4A R1(SinAngles(2), SinAngles(2), CosAngles(2), SinAngles(2));

            Vector4A Q1 = P1.cwiseProduct(Sign);
            Vector4A Q0 = P0.cwiseProduct(Y0);
            Q1 = Q1.cwiseProduct(Y1);
            Q0 = Q0.cwiseProduct(R0);
            QuaternionA Q = QuaternionA(Q1.cwiseProduct(R1) + Q0);
            return Q;
        }

        inline QuaternionA RotationRollPitchYaw(float pitch, float yaw, float roll)
        {
            return RotationRollPitchYaw(Vector3(pitch, yaw, roll));
        }
    }

    namespace Matrix
    {
        template<size_t rows, size_t cols, int options, size_t maxRows, size_t maxCols>
        inline Eigen::Matrix<float, rows, cols, options, maxRows, maxCols> ConvertToRadians(const Eigen::Matrix<float, rows, cols, options, maxRows, maxCols>& Mat)
        {
            return Mat * (PI / 180);
        }

        template<size_t rows, size_t cols, int options, size_t maxRows, size_t maxCols>
        inline Eigen::Matrix<float, rows, cols, options, maxRows, maxCols> ConvertToAngles(const Eigen::Matrix<float, rows, cols, options, maxRows, maxCols>& Mat)
        {
            return Mat * (180 / PI);
        }


        template<size_t rows, size_t cols, int options, size_t maxRows, size_t maxCols>
        inline Eigen::Matrix<float, rows, cols, options, maxRows, maxCols> ModAngles(const Eigen::Matrix<float, rows, cols, options, maxRows, maxCols>& Mat)
        {
            return Mat.unaryExpr([](float x) { 
                // Normalize the range from 0.0f to 2 * PI
                x = x + PI;
                // Perform the modulo, unsigned
                float fTemp = fabsf(x);
                fTemp = fTemp - (2 * PI * static_cast<float>(static_cast<int32_t>(fTemp / (2 * PI))));
                // Restore the number to the range of -PI to PI-epsilon
                fTemp = fTemp - PI;
                // If the modulo'd value was negative, restore negation
                if (x < 0.0f) {
                    fTemp = -fTemp;
                }
                return fTemp;
            });
        }

        inline Matrix4x4A PerspectiveLH(float ViewWidth, float ViewHeight, float NearZ, float FarZ)
        {
            Matrix4x4A P = Matrix4x4A::Zero();

            float TwoNearZ = NearZ + NearZ;
            float fRange = FarZ / (FarZ - NearZ);

            P(0, 0) = TwoNearZ / ViewWidth;
            P(1, 1) = TwoNearZ / ViewHeight;
            P(2, 2) = fRange;
            P(2, 3) = -fRange * NearZ;
            P(3, 2) = 1.0f;

            return P;
        }

        inline Matrix4x4A PerspectiveFovLH(float FovAngleY, float AspectRatio, float NearZ, float FarZ)
        {
            Matrix4x4A P = Matrix4x4A::Zero();

            float SinFov = sinf(0.5f * FovAngleY);
            float CosFov = cosf(0.5f * FovAngleY);

            float Height = CosFov / SinFov;
            float Width = Height / AspectRatio;
            float fRange = FarZ / (FarZ - NearZ);

            P(0, 0) = Width;
            P(1, 1) = Height;
            P(2, 2) = fRange;
            P(2, 3) = -fRange * NearZ;
            P(3, 2) = 1.0f;

            return P;
        }

        inline Matrix4x4A __vectorcall InverseTranspose(Matrix4x4A Mat)
        {
            Matrix4x4A A = Mat;
            A.topRightCorner<3, 1>() = Vector3::Zero();
            return A.inverse().transpose();
        }

    }
    
}
