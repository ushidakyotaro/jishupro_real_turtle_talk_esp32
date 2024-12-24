// motion_patterns.h
#pragma once
#include <vector>
#include <array>

struct WingMotionPoint {
    double timeRatio;   // 0.0 ~ 1.0
    double angleRatio;  // -1.0 ~ 1.0
    bool operator==(const WingMotionPoint& other) const {
        return timeRatio == other.timeRatio && angleRatio == other.angleRatio;
    }
};


struct SplineCoefficients {
    std::vector<double> a;  // 3次項の係数
    std::vector<double> b;  // 2次項の係数
    std::vector<double> c;  // 1次項の係数
    std::vector<double> d;  // 定数項
};

namespace MotionPatterns {
    // 基本的な泳ぎパターン
    const std::vector<WingMotionPoint> SWIM_PATTERN = {
        {0.0,  0.0},   // 開始位置（中心）
        {0.25, 1.0},   // 最大位置
        {0.5,  0.0},   // 中間位置
        {0.75, -1.0},  // 最小位置
        {1.0,  0.0}    // 終了位置（中心に戻る）
    };

    // その場で浮遊するパターン
    const std::vector<WingMotionPoint> STAY_PATTERN = {
        {0.0,  0.0},   // 開始位置
        {0.25, 0.5},   // 小さめの振幅
        {0.5,  0.0},
        {0.75, -0.5},
        {1.0,  0.0}
    };
}

class SplineInterpolator {
public:
    SplineInterpolator() {}

    void calculateCoefficients(const std::vector<WingMotionPoint>& points) {
        size_t n = points.size() - 1;
        
        // 係数配列の初期化
        coeffs.a.resize(n);
        coeffs.b.resize(n);
        coeffs.c.resize(n);
        coeffs.d.resize(n);

        // h[i] = x[i+1] - x[i]
        std::vector<double> h(n);
        for (size_t i = 0; i < n; i++) {
            h[i] = points[i + 1].timeRatio - points[i].timeRatio;
        }

        // 3次スプライン補間の係数計算
        std::vector<double> alpha(n);
        std::vector<double> l(n + 1);
        std::vector<double> mu(n);
        std::vector<double> z(n + 1);

        // Step 1: 前進消去
        l[0] = 1.0;
        for (size_t i = 1; i < n; i++) {
            alpha[i] = 3.0 * (points[i + 1].angleRatio - points[i].angleRatio) / h[i]
                    - 3.0 * (points[i].angleRatio - points[i - 1].angleRatio) / h[i - 1];
            l[i] = 2.0 * (points[i + 1].timeRatio - points[i - 1].timeRatio) - h[i - 1] * mu[i - 1];
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
        }

        // Step 2: 後退代入
        l[n] = 1.0;
        z[n] = 0.0;
        coeffs.c[n - 1] = z[n - 1];
        
        for (int i = n - 2; i >= 0; i--) {
            coeffs.c[i] = z[i] - mu[i] * coeffs.c[i + 1];
            coeffs.b[i] = (points[i + 1].angleRatio - points[i].angleRatio) / h[i]
                       - h[i] * (coeffs.c[i + 1] + 2.0 * coeffs.c[i]) / 3.0;
            coeffs.a[i] = (coeffs.c[i + 1] - coeffs.c[i]) / (3.0 * h[i]);
        }

        // d係数の設定
        for (size_t i = 0; i < n; i++) {
            coeffs.d[i] = points[i].angleRatio;
        }
    }

    double interpolate(double t, const std::vector<WingMotionPoint>& points) {
        // 適切な区間を見つける
        size_t i = 0;
        while (i < points.size() - 1 && t > points[i + 1].timeRatio) {
            i++;
        }

        // 相対時間の計算
        double dt = t - points[i].timeRatio;
        
        // スプライン補間値の計算
        return coeffs.a[i] * dt * dt * dt +
               coeffs.b[i] * dt * dt +
               coeffs.c[i] * dt +
               coeffs.d[i];
    }

private:
    SplineCoefficients coeffs;
};
