#pragma once

#include <glm/glm.hpp>

// https://github.com/prisonerjohn/ofxWarp/blob/master/src/ofxWarp/WarpPerspective.cpp

namespace {
void gaussianElimination(float *input, int n) {
    auto i = 0;
    auto j = 0;
    auto m = n - 1;

    while (i < m && j < n) {
        auto iMax = i;
        for (auto k = i + 1; k < m; ++k) {
            if (fabs(input[k * n + j]) > fabs(input[iMax * n + j])) {
                iMax = k;
            }
        }

        if (input[iMax * n + j] != 0) {
            if (i != iMax) {
                for (auto k = 0; k < n; ++k) {
                    auto ikIn = input[i * n + k];
                    input[i * n + k] = input[iMax * n + k];
                    input[iMax * n + k] = ikIn;
                }
            }

            float ijIn = input[i * n + j];
            for (auto k = 0; k < n; ++k) {
                input[i * n + k] /= ijIn;
            }

            for (auto u = i + 1; u < m; ++u) {
                auto ujIn = input[u * n + j];
                for (auto k = 0; k < n; ++k) {
                    input[u * n + k] -= ujIn * input[i * n + k];
                }
            }

            ++i;
        }
        ++j;
    }

    for (auto i = m - 2; i >= 0; --i) {
        for (auto j = i + 1; j < n - 1; ++j) {
            input[i * n + m] -= input[i * n + j] * input[j * n + m];
        }
    }
}
} // namespace

namespace ComputeMat3 {
glm::mat3 computePerspective(const glm::vec2 src[4],
                             const glm::vec2 dst[4]) {
    float p[8][9] = {
        {-src[0][0], -src[0][1], -1, 0, 0, 0, src[0][0] * dst[0][0],
         src[0][1] * dst[0][0], -dst[0][0]}, // h11
        {0, 0, 0, -src[0][0], -src[0][1], -1, src[0][0] * dst[0][1],
         src[0][1] * dst[0][1], -dst[0][1]}, // h12
        {-src[1][0], -src[1][1], -1, 0, 0, 0, src[1][0] * dst[1][0],
         src[1][1] * dst[1][0], -dst[1][0]}, // h13
        {0, 0, 0, -src[1][0], -src[1][1], -1, src[1][0] * dst[1][1],
         src[1][1] * dst[1][1], -dst[1][1]}, // h21
        {-src[2][0], -src[2][1], -1, 0, 0, 0, src[2][0] * dst[2][0],
         src[2][1] * dst[2][0], -dst[2][0]}, // h22
        {0, 0, 0, -src[2][0], -src[2][1], -1, src[2][0] * dst[2][1],
         src[2][1] * dst[2][1], -dst[2][1]}, // h23
        {-src[3][0], -src[3][1], -1, 0, 0, 0, src[3][0] * dst[3][0],
         src[3][1] * dst[3][0], -dst[3][0]}, // h31
        {0, 0, 0, -src[3][0], -src[3][1], -1, src[3][0] * dst[3][1],
         src[3][1] * dst[3][1], -dst[3][1]}, // h32
    };

    gaussianElimination(&p[0][0], 9);

    return glm::mat3(p[0][8], p[3][8], p[6][8],
                     p[1][8], p[4][8], p[7][8],
                     p[2][8], p[5][8], 1);
}

} // namespace ComputeMat3
