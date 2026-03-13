/**
 * @file restir_gi.c
 * @brief ReSTIR GI 完整实现 - 直接光照 + 间接光照 + 阴影
 * @author 咯哩
 * @date 2026-03-13
 * 
 * 实现功能：
 * - 单个方向光（平行光）
 * - 完美漫反射 BRDF (Lambertian)
 * - 阴影（阴影射线测试）
 * - 间接光照（一次反弹，使用 ReSTIR 采样）
 * - 无环境光
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// ============================================================================
// 基本类型和常量
// ============================================================================

#define IMAGE_WIDTH 512
#define IMAGE_HEIGHT 512
#define MAX_SPHERES 16
#define MAX_BOUNCES 2       // 光线反弹次数（1 次间接光）
#define RESERVOIR_SIZE 16   // 蓄水池容量

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 三维向量
typedef struct {
    float x, y, z;
} Vec3;

// 颜色（RGB）
typedef struct {
    float r, g, b;
} Color;

// 光线
typedef struct {
    Vec3 origin;
    Vec3 direction;
} Ray;

// 球体
typedef struct {
    Vec3 center;
    float radius;
    Color albedo;  // Lambertian BRDF 的反照率
    float emit;    // 自发光强度
} Sphere;

// 方向光（平行光）
typedef struct {
    Vec3 direction;  // 光的方向（指向光源）
    Color intensity; // 光强度
    float shadowBias; // 阴影偏移
} DirectionalLight;

// 蓄水池样本（用于间接光采样）
typedef struct {
    Vec3 direction;    // 采样方向
    Color radiance;    // 从该方向来的辐射度
    float weight;      // 重要性权重
    float pdf;         // 概率密度
} ReservoirSample;

// 蓄水池结构
typedef struct {
    ReservoirSample samples[RESERVOIR_SIZE];
    int count;
    float totalWeight;
} Reservoir;

// 击中信息
typedef struct {
    float t;           // 相交距离
    int sphereIndex;   // 击中的球体索引
    Vec3 hitPoint;     // 击中点
    Vec3 normal;       // 表面法线
    int hit;           // 是否击中
} HitInfo;

// ============================================================================
// 场景全局变量
// ============================================================================

Sphere spheres[MAX_SPHERES];
int numSpheres = 0;
DirectionalLight mainLight;

// ============================================================================
// 向量运算
// ============================================================================

static inline Vec3 vec3(float x, float y, float z) {
    Vec3 v = {x, y, z};
    return v;
}

static inline Vec3 vec3_add(Vec3 a, Vec3 b) {
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline Vec3 vec3_mul(Vec3 a, float s) {
    return vec3(a.x * s, a.y * s, a.z * s);
}

static inline Vec3 vec3_negate(Vec3 a) {
    return vec3(-a.x, -a.y, -a.z);
}

static inline float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float vec3_length(Vec3 v) {
    return sqrtf(vec3_dot(v, v));
}

static inline Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_length(v);
    if (len > 0.0001f) {
        return vec3_mul(v, 1.0f / len);
    }
    return vec3(0, 0, 0);
}

// ============================================================================
// 颜色运算
// ============================================================================

static inline Color color(float r, float g, float b) {
    Color c = {r, g, b};
    return c;
}

static inline Color color_add(Color a, Color b) {
    return color(a.r + b.r, a.g + b.g, a.b + b.b);
}

static inline Color color_mul(Color a, float s) {
    return color(a.r * s, a.g * s, a.b * s);
}

static inline Color color_mul_color(Color a, Color b) {
    return color(a.r * b.r, a.g * b.g, a.b * b.b);
}

static inline Color color_clamp(Color c) {
    return color(
        fminf(1.0f, fmaxf(0.0f, c.r)),
        fminf(1.0f, fmaxf(0.0f, c.g)),
        fminf(1.0f, fmaxf(0.0f, c.b))
    );
}

// ============================================================================
// 随机数生成
// ============================================================================

static inline float random_float() {
    return (float)rand() / (float)RAND_MAX;
}

/**
 * @brief 余弦加权半球采样（Lambertian BRDF 的重要性采样）
 * 
 * 对于完美漫反射表面，按 cosθ 加权采样最优
 */
static Vec3 cosine_weighted_hemisphere(Vec3 normal) {
    float r1 = random_float();
    float r2 = random_float();
    
    float phi = 2.0f * (float)M_PI * r1;
    float cosTheta = sqrtf(1.0f - r2);  // cosθ = sqrt(1-ξ₂)
    float sinTheta = sqrtf(r2);          // sinθ = sqrt(ξ₂)
    
    // 局部坐标系中的方向
    float x = sinTheta * cosf(phi);
    float y = sinTheta * sinf(phi);
    float z = cosTheta;
    
    // 构建 TBN 矩阵转换到世界空间
    // 简化：假设 normal 接近 (0,1,0)，构建正交基
    Vec3 up = (fabsf(normal.y) > 0.99f) ? vec3(1, 0, 0) : vec3(0, 1, 0);
    Vec3 tangent = vec3_normalize(vec3_sub(up, vec3_mul(normal, vec3_dot(normal, up))));
    Vec3 bitangent = vec3_normalize(vec3_sub(vec3(0,0,0), vec3_sub(vec3(0,0,0), 
        vec3_sub(vec3(0,0,0), vec3_sub(vec3(0,0,0), vec3(0,0,0))))));
    
    // 简化处理：直接使用法线方向
    Vec3 local = vec3(x, y, z);
    
    // 将局部方向旋转到法线方向
    // 使用反射公式简化
    float dot = vec3_dot(local, vec3(0, 1, 0));
    Vec3 worldDir = vec3_add(
        vec3_mul(normal, dot),
        vec3_mul(vec3(local.x, 0, local.z), 1.0f - fabsf(dot))
    );
    
    return vec3_normalize(worldDir);
}

// ============================================================================
// 蓄水池算法（ReSTIR 核心）
// ============================================================================

static void reservoir_reset(Reservoir* reservoir) {
    reservoir->count = 0;
    reservoir->totalWeight = 0;
}

/**
 * @brief 蓄水池更新
 * 
 * 算法 R 的变种：以一定概率保留新样本
 */
static void reservoir_update(Reservoir* reservoir, ReservoirSample sample, float weight) {
    if (reservoir->count < RESERVOIR_SIZE) {
        // 蓄水池未满，直接添加
        reservoir->samples[reservoir->count] = sample;
        reservoir->count++;
        reservoir->totalWeight += weight;
    } else {
        // 蓄水池已满，以 M/(M+1) 的概率考虑替换
        reservoir->totalWeight += weight;
        float p = (float)reservoir->count / reservoir->totalWeight * weight;
        
        if (random_float() < p) {
            // 随机替换一个现有样本
            int idx = rand() % RESERVOIR_SIZE;
            reservoir->samples[idx] = sample;
        }
        reservoir->count++;
    }
}

/**
 * @brief 从蓄水池重采样一个样本
 */
static ReservoirSample reservoir_resample(Reservoir* reservoir) {
    if (reservoir->count == 0) {
        ReservoirSample empty = {{0, 0, 0}, {0, 0, 0}, 0, 0};
        return empty;
    }
    
    // 按权重比例随机选择
    float r = random_float() * reservoir->totalWeight;
    float cumulative = 0;
    
    for (int i = 0; i < reservoir->count && i < RESERVOIR_SIZE; i++) {
        cumulative += reservoir->samples[i].weight;
        if (r <= cumulative) {
            return reservoir->samples[i];
        }
    }
    
    return reservoir->samples[reservoir->count - 1];
}

/**
 * @brief 获取修正后的权重
 * 
 * ReSTIR 关键公式：W = Σwᵢ / M
 */
static float reservoir_get_weight(Reservoir* reservoir) {
    if (reservoir->count == 0) return 0;
    return reservoir->totalWeight / (float)reservoir->count;
}

// ============================================================================
// 场景设置
// ============================================================================

static void init_scene() {
    // 地面（大球模拟平面）
    spheres[numSpheres++] = (Sphere){
        .center = vec3(0, -1000, 0),
        .radius = 1000,
        .albedo = color(0.7f, 0.7f, 0.7f),  // 灰色地面
        .emit = 0
    };
    
    // 红球
    spheres[numSpheres++] = (Sphere){
        .center = vec3(0, 0, -5),
        .radius = 1.0f,
        .albedo = color(0.9f, 0.1f, 0.1f),
        .emit = 0
    };
    
    // 绿球
    spheres[numSpheres++] = (Sphere){
        .center = vec3(2.5f, 0, -4),
        .radius = 1.0f,
        .albedo = color(0.1f, 0.9f, 0.1f),
        .emit = 0
    };
    
    // 蓝球
    spheres[numSpheres++] = (Sphere){
        .center = vec3(-2.5f, 0, -4),
        .radius = 1.0f,
        .albedo = color(0.1f, 0.1f, 0.9f),
        .emit = 0
    };
    
    // 白球（在中间，用于展示间接光）
    spheres[numSpheres++] = (Sphere){
        .center = vec3(0, 0, -2.5),
        .radius = 0.5f,
        .albedo = color(0.95f, 0.95f, 0.95f),
        .emit = 0
    };
    
    // 方向光（从左上角照射）
    mainLight.direction = vec3_normalize(vec3(-1, -2, -1));  // 指向右下方
    mainLight.intensity = color(3.0f, 3.0f, 3.0f);  // 较强的光
    mainLight.shadowBias = 0.001f;
}

// ============================================================================
// 光线追踪核心
// ============================================================================

/**
 * @brief 光线与球体相交
 * @return 相交距离，无交点返回 -1
 */
static float ray_sphere_intersect(Ray ray, Sphere sphere) {
    Vec3 oc = vec3_sub(ray.origin, sphere.center);
    
    float a = vec3_dot(ray.direction, ray.direction);
    float b = 2.0f * vec3_dot(oc, ray.direction);
    float c = vec3_dot(oc, oc) - sphere.radius * sphere.radius;
    
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) return -1;
    
    float sqrtDisc = sqrtf(discriminant);
    float t = (-b - sqrtDisc) / (2 * a);
    
    if (t < 0.001f) {
        t = (-b + sqrtDisc) / (2 * a);
    }
    
    return t > 0.001f ? t : -1;
}

/**
 * @brief 查找最近交点
 */
static HitInfo find_nearest_hit(Ray ray) {
    HitInfo info = {-1, -1, {0,0,0}, {0,0,0}, 0};
    
    float nearestT = -1;
    
    for (int i = 0; i < numSpheres; i++) {
        float t = ray_sphere_intersect(ray, spheres[i]);
        if (t > 0 && (nearestT < 0 || t < nearestT)) {
            nearestT = t;
            info.sphereIndex = i;
        }
    }
    
    if (nearestT > 0) {
        info.t = nearestT;
        info.hit = 1;
        info.hitPoint = vec3_add(ray.origin, vec3_mul(ray.direction, nearestT));
        info.normal = vec3_normalize(vec3_sub(info.hitPoint, spheres[info.sphereIndex].center));
        
        // 地面法线特殊处理
        if (info.sphereIndex == 0) {
            info.normal = vec3(0, 1, 0);
        }
    }
    
    return info;
}

/**
 * @brief 阴影测试
 * 
 * 检查从 hitPoint 到光源方向是否被遮挡
 * 对于方向光，沿光反方向发射阴影射线
 * 
 * @return 1 = 在阴影中，0 = 被照亮
 */
static int in_shadow(Vec3 hitPoint, Vec3 normal) {
    // 阴影射线起点（加偏移避免自相交）
    Vec3 shadowOrigin = vec3_add(hitPoint, vec3_mul(normal, mainLight.shadowBias));
    
    // 阴影射线方向（指向光源，与光方向相反）
    Vec3 shadowDir = vec3_negate(mainLight.direction);
    
    Ray shadowRay = {shadowOrigin, shadowDir};
    
    // 检查是否与任何物体相交
    for (int i = 0; i < numSpheres; i++) {
        float t = ray_sphere_intersect(shadowRay, spheres[i]);
        if (t > 0.001f) {
            return 1;  // 被遮挡，在阴影中
        }
    }
    
    return 0;  // 无遮挡，被照亮
}

// ============================================================================
// ReSTIR GI 渲染
// ============================================================================

/**
 * @brief 计算直接光照（方向光 + 阴影）
 * 
 * Lambertian BRDF: f = albedo / π
 * 光照：L = max(0, n·l) * lightIntensity * visibility
 */
static Color compute_direct_lighting(Vec3 hitPoint, Vec3 normal, Color albedo) {
    // 阴影测试
    int shadow = in_shadow(hitPoint, normal);
    
    if (shadow) {
        return color(0, 0, 0);  // 完全在阴影中
    }
    
    // Lambertian 反射
    float nDotL = vec3_dot(normal, vec3_negate(mainLight.direction));
    
    if (nDotL <= 0) {
        return color(0, 0, 0);  // 背光面
    }
    
    // BRDF = albedo / π
    float brdf = 1.0f / (float)M_PI;
    
    // 直接光照 = 光强度 * BRDF * cosθ
    Color direct = color_mul(mainLight.intensity, brdf * nDotL);
    
    return color_mul_color(albedo, direct);
}

/**
 * @brief 使用 ReSTIR 计算间接光照
 * 
 * 核心思想：
 * 1. 在半球内采样多个方向
 * 2. 对每个方向发射光线，获取击中点的辐射度
 * 3. 用蓄水池收集这些样本
 * 4. 重采样得到一个最优方向
 * 5. 用修正权重计算最终间接光
 */
static Color compute_indirect_lighting(HitInfo hit, int bounce) {
    if (bounce >= MAX_BOUNCES) {
        return color(0, 0, 0);
    }
    
    Reservoir reservoir;
    reservoir_reset(&reservoir);
    
    Color albedo = spheres[hit.sphereIndex].albedo;
    
    // 采样多个方向收集间接光
    int numSamples = 8;  // 每个像素的间接光采样数
    
    for (int i = 0; i < numSamples; i++) {
        // 余弦加权采样（匹配 Lambertian BRDF）
        Vec3 sampleDir = cosine_weighted_hemisphere(hit.normal);
        
        // 发射光线
        Ray indirectRay;
        indirectRay.origin = vec3_add(hit.hitPoint, vec3_mul(hit.normal, 0.001f));
        indirectRay.direction = sampleDir;
        
        HitInfo nextHit = find_nearest_hit(indirectRay);
        
        if (nextHit.hit) {
            // 获取击中点的颜色（包括直接光和更深层的间接光）
            Color incomingRadiance = compute_indirect_lighting(nextHit, bounce + 1);
            
            // 计算重要性权重
            // w = f * cosθ / pdf
            // 对于余弦加权采样，pdf = cosθ/π，所以 w = f * π = albedo
            float cosTheta = vec3_dot(hit.normal, sampleDir);
            if (cosTheta > 0) {
                float pdf = cosTheta / (float)M_PI;  // 余弦加权的 PDF
                float brdf = 1.0f / (float)M_PI;      // Lambertian BRDF
                
                // 权重 = BRDF * cosθ / pdf = (1/π) * cosθ / (cosθ/π) = 1
                // 但还要乘以 albedo 和 incoming radiance
                float weight = (brdf * cosTheta / pdf);
                
                ReservoirSample sample;
                sample.direction = sampleDir;
                sample.radiance = incomingRadiance;
                sample.weight = weight;
                sample.pdf = pdf;
                
                reservoir_update(&reservoir, sample, weight);
            }
        }
    }
    
    // 重采样得到最终样本
    ReservoirSample final = reservoir_resample(&reservoir);
    
    if (reservoir.count == 0) {
        return color(0, 0, 0);
    }
    
    // 应用权重修正
    float W = reservoir_get_weight(&reservoir);
    
    // 最终间接光 = incoming radiance * 修正权重 * albedo
    Color indirect = color_mul(final.radiance, W);
    indirect = color_mul_color(indirect, albedo);
    
    return indirect;
}

/**
 * @brief 计算单个像素的颜色
 */
static Color render_pixel(int x, int y) {
    // 计算相机射线（简化针孔相机）
    float aspect = (float)IMAGE_WIDTH / (float)IMAGE_HEIGHT;
    float viewportHeight = 2.0f;
    float viewportWidth = aspect * viewportHeight;
    float focalLength = 1.0f;
    
    // 归一化设备坐标
    float u = ((float)x + 0.5f) / IMAGE_WIDTH;
    float v = ((float)y + 0.5f) / IMAGE_HEIGHT;
    
    // 射线方向
    Vec3 rayDir = vec3_normalize(vec3(
        (u - 0.5f) * viewportWidth,
        (v - 0.5f) * viewportHeight,
        -focalLength
    ));
    
    Ray ray;
    ray.origin = vec3(0, 0, 0);
    ray.direction = rayDir;
    
    // 查找交点
    HitInfo hit = find_nearest_hit(ray);
    
    if (!hit.hit) {
        // 背景：渐变天空
        float t = 0.5f * (rayDir.y + 1.0f);
        Color skyTop = color(0.1f, 0.1f, 0.2f);    // 深蓝
        Color skyBottom = color(0.3f, 0.3f, 0.35f); // 灰蓝
        return color_add(color_mul(skyTop, 1-t), color_mul(skyBottom, t));
    }
    
    Color albedo = spheres[hit.sphereIndex].albedo;
    
    // 直接光照（带阴影）
    Color direct = compute_direct_lighting(hit.hitPoint, hit.normal, albedo);
    
    // 间接光照（使用 ReSTIR）
    Color indirect = compute_indirect_lighting(hit, 0);
    
    // 总光照 = 直接光 + 间接光
    Color result = color_add(direct, indirect);
    
    // Gamma 校正 (sRGB)
    result.r = powf(fminf(1, result.r), 1.0f / 2.2f);
    result.g = powf(fminf(1, result.g), 1.0f / 2.2f);
    result.b = powf(fminf(1, result.b), 1.0f / 2.2f);
    
    return color_clamp(result);
}

// ============================================================================
// 图像输出（PPM 格式）
// ============================================================================

static void save_ppm(const char* filename, Color* image) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("错误：无法创建文件 %s\n", filename);
        return;
    }
    
    fprintf(f, "P3\n%d %d\n255\n", IMAGE_WIDTH, IMAGE_HEIGHT);
    
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            Color c = image[y * IMAGE_WIDTH + x];
            int r = (int)(255.99f * c.r);
            int g = (int)(255.99f * c.g);
            int b = (int)(255.99f * c.b);
            fprintf(f, "%d %d %d\n", r, g, b);
        }
    }
    
    fclose(f);
    printf("图像已保存：%s\n", filename);
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, const char * argv[]) {
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║          ReSTIR GI - 全局光照渲染器                   ║\n");
    printf("║          Reservoir Spatiotemporal                    ║\n");
    printf("║          Importance Resampling for GI                ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");
    
    // 初始化随机数
    srand((unsigned int)time(NULL));
    
    // 初始化场景
    init_scene();
    printf("✓ 场景初始化完成\n");
    printf("  - 球体数量：%d\n", numSpheres);
    printf("  - 光源：单个方向光\n");
    printf("  - 光方向：(%.2f, %.2f, %.2f)\n", 
           mainLight.direction.x, mainLight.direction.y, mainLight.direction.z);
    printf("  - 图像尺寸：%dx%d\n", IMAGE_WIDTH, IMAGE_HEIGHT);
    printf("  - 蓄水池容量：%d\n", RESERVOIR_SIZE);
    printf("  - 间接光反弹：%d 次\n\n", MAX_BOUNCES);
    
    // 分配图像内存
    Color* image = (Color*)malloc(IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(Color));
    if (!image) {
        printf("错误：内存分配失败\n");
        return 1;
    }
    
    printf("开始渲染（直接光 + 阴影 + 间接光）...\n");
    clock_t start = clock();
    
    // 渲染每个像素
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            int idx = y * IMAGE_WIDTH + x;
            image[idx] = render_pixel(x, y);
        }
        
        // 进度显示
        if ((y + 1) % 64 == 0) {
            printf("  进度：%d%% (%d/%d 行)\n", 
                   (y + 1) * 100 / IMAGE_HEIGHT, y + 1, IMAGE_HEIGHT);
        }
    }
    
    clock_t end = clock();
    float duration = (float)(end - start) / CLOCKS_PER_SEC;
    
    printf("\n✓ 渲染完成！\n");
    printf("  耗时：%.2f 秒\n", duration);
    printf("  性能：%.0f 像素/秒\n", 
           (float)(IMAGE_WIDTH * IMAGE_HEIGHT) / duration);
    
    // 保存图像
    save_ppm("restir_gi_output.ppm", image);
    
    // 打印核心公式
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║  ReSTIR GI 核心公式：                                  ║\n");
    printf("║                                                       ║\n");
    printf("║  渲染方程：                                            ║\n");
    printf("║  Lₒ = L_direct + L_indirect                           ║\n");
    printf("║                                                       ║\n");
    printf("║  直接光（Lambertian + 阴影）:                          ║\n");
    printf("║  L_direct = (albedo/π) * L_light * max(0,n·l) * V     ║\n");
    printf("║  V = 可见性（0=阴影，1=照亮）                          ║\n");
    printf("║                                                       ║\n");
    printf("║  间接光（ReSTIR 采样）:                                ║\n");
    printf("║  L_indirect = ∫ f * L_in * cosθ dω                    ║\n");
    printf("║  使用蓄水池重采样近似积分                              ║\n");
    printf("║                                                       ║\n");
    printf("║  权重修正：                                            ║\n");
    printf("║  W = Σwᵢ / M                                           ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    
    // 清理内存
    free(image);
    
    printf("\n提示：用 Preview 或 Photoshop 打开 restir_gi_output.ppm 查看结果\n");
    printf("按回车键退出...\n");
    getchar();
    
    return 0;
}
