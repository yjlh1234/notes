/**
 * @file restir_cpu.c
 * @brief ReSTIR GI CPU 简化实现（教学演示版）
 * @author 咯哩
 * @date 2026-03-13
 * 
 * 实现核心功能：
 * - 蓄水池采样 (Reservoir Sampling)
 * - 重要性重采样 (Importance Resampling)
 * - 简化的全局光照渲染
 * 
 * 注意：这是简化版本，用于理解 ReSTIR 核心思想
 * 完整实现需要光线追踪、时空复用等复杂功能
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
#define MAX_LIGHTS 16
#define RESERVOIR_SIZE 8  // 蓄水池容量（空间 + 时间样本）

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

// 球体（场景中的物体）
typedef struct {
    Vec3 center;
    float radius;
    Color albedo;  // 表面颜色
} Sphere;

// 光源
typedef struct {
    Vec3 position;
    Color intensity;
    float radius;
} Light;

// 蓄水池样本
typedef struct {
    Vec3 sampleDir;      // 采样方向
    float weight;        // 权重
    float pdf;           // 概率密度
    int lightIndex;      // 选中的光源索引
} ReservoirSample;

// 蓄水池结构
typedef struct {
    ReservoirSample samples[RESERVOIR_SIZE];
    int count;           // 当前样本数
    float totalWeight;   // 累积权重
    Vec3 reservedValue;  // 最终保留的样本值
    float reservedWeight;// 保留样本的权重
} Reservoir;

// ============================================================================
// 向量运算
// ============================================================================

Vec3 vec3(float x, float y, float z) {
    Vec3 v = {x, y, z};
    return v;
}

Vec3 vec3_add(Vec3 a, Vec3 b) {
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3 vec3_mul(Vec3 a, float s) {
    return vec3(a.x * s, a.y * s, a.z * s);
}

float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float vec3_length(Vec3 v) {
    return sqrtf(vec3_dot(v, v));
}

Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_length(v);
    if (len > 0.0001f) {
        return vec3_mul(v, 1.0f / len);
    }
    return vec3(0, 0, 0);
}

// ============================================================================
// 颜色运算
// ============================================================================

Color color(float r, float g, float b) {
    Color c = {r, g, b};
    return c;
}

Color color_add(Color a, Color b) {
    return color(a.r + b.r, a.g + b.g, a.b + b.b);
}

Color color_mul(Color a, float s) {
    return color(a.r * s, a.g * s, a.b * s);
}

Color color_mul_color(Color a, Color b) {
    return color(a.r * b.r, a.g * b.g, a.b * b.b);
}

// ============================================================================
// 随机数生成
// ============================================================================

float random_float() {
    return (float)rand() / (float)RAND_MAX;
}

// 在单位半球内均匀采样（余弦加权）
Vec3 cosine_weighted_sample(Vec3 normal) {
    float r1 = random_float();
    float r2 = random_float();
    
    float phi = 2.0f * (float)M_PI * r1;
    float cosTheta = sqrtf(r2);
    float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    
    // 局部坐标
    Vec3 local = vec3(
        sinTheta * cosf(phi),
        sinTheta * sinf(phi),
        cosTheta
    );
    
    // 这里简化处理，假设 normal 是 (0, 1, 0)
    // 完整实现需要构建 TBN 矩阵
    return vec3_add(local, vec3_mul(normal, 0.5f));
}

// ============================================================================
// 蓄水池采样核心算法
// ============================================================================

/**
 * @brief 蓄水池更新（Reservoir Update）
 * 
 * ReSTIR 的核心：每个像素维护一个蓄水池，收集时空样本
 * 
 * @param reservoir 蓄水池
 * @param sample 新样本
 * @param weight 样本权重（重要性采样权重）
 */
void reservoir_update(Reservoir* reservoir, ReservoirSample sample, float weight) {
    if (reservoir->count < RESERVOIR_SIZE) {
        // 蓄水池未满，直接添加
        reservoir->samples[reservoir->count] = sample;
        reservoir->count++;
        reservoir->totalWeight += weight;
    } else {
        // 蓄水池已满，以 M/(M+1) 的概率替换
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
 * @brief 蓄水池重采样（Resample）
 * 
 * 从蓄水池中按权重比例选择一个样本作为最终输出
 * 
 * @param reservoir 蓄水池
 * @return 选中的样本
 */
ReservoirSample reservoir_resample(Reservoir* reservoir) {
    if (reservoir->count == 0) {
        ReservoirSample empty = {{0, 0, 0}, 0, 0, -1};
        return empty;
    }
    
    // 按权重比例随机选择一个样本
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
 * @brief 计算重采样后的修正权重
 * 
 * ReSTIR 的关键：权重修正保证无偏估计
 * W = sum(w_i) / M
 * 
 * @param reservoir 蓄水池
 * @return 修正后的权重
 */
float reservoir_get_weight(Reservoir* reservoir) {
    if (reservoir->count == 0) return 0;
    return reservoir->totalWeight / (float)reservoir->count;
}

/**
 * @brief 重置蓄水池（用于新像素或新帧）
 */
void reservoir_reset(Reservoir* reservoir) {
    reservoir->count = 0;
    reservoir->totalWeight = 0;
    reservoir->reservedValue = vec3(0, 0, 0);
    reservoir->reservedWeight = 0;
}

// ============================================================================
// 场景定义
// ============================================================================

Sphere spheres[3];
Light lights[2];
int numSpheres = 3;
int numLights = 2;

void init_scene() {
    // 三个球体
    spheres[0] = (Sphere){vec3(0, 0, -5), 1.0f, color(0.8f, 0.2f, 0.2f)};  // 红球
    spheres[1] = (Sphere){vec3(2.5f, 0, -4), 1.0f, color(0.2f, 0.8f, 0.2f)}; // 绿球
    spheres[2] = (Sphere){vec3(-2.5f, 0, -4), 1.0f, color(0.2f, 0.2f, 0.8f)}; // 蓝球
    
    // 两个光源
    lights[0] = (Light){vec3(0, 5, -3), color(10.0f, 10.0f, 10.0f), 2.0f};
    lights[1] = (Light){vec3(-3, 3, -2), color(8.0f, 8.0f, 12.0f), 1.5f};
}

// ============================================================================
// 光线追踪
// ============================================================================

/**
 * @brief 光线与球体相交测试
 * @return 相交距离，若无交点返回 -1
 */
float ray_sphere_intersect(Ray ray, Sphere sphere) {
    Vec3 oc = vec3_sub(ray.origin, sphere.center);
    
    float a = vec3_dot(ray.direction, ray.direction);
    float b = 2.0f * vec3_dot(oc, ray.direction);
    float c = vec3_dot(oc, oc) - sphere.radius * sphere.radius;
    
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) return -1;
    
    float t = (-b - sqrtf(discriminant)) / (2 * a);
    if (t < 0.001f) {
        t = (-b + sqrtf(discriminant)) / (2 * a);
    }
    
    return t > 0.001f ? t : -1;
}

/**
 * @brief 查找最近交点
 * @return 最近的相交距离，若无交点返回 -1
 */
float find_nearest_hit(Ray ray, int* hitIndex) {
    float nearestT = -1;
    *hitIndex = -1;
    
    for (int i = 0; i < numSpheres; i++) {
        float t = ray_sphere_intersect(ray, spheres[i]);
        if (t > 0 && (nearestT < 0 || t < nearestT)) {
            nearestT = t;
            *hitIndex = i;
        }
    }
    
    return nearestT;
}

/**
 * @brief 计算表面法线
 */
Vec3 get_sphere_normal(Vec3 point, Sphere sphere) {
    return vec3_normalize(vec3_sub(point, sphere.center));
}

// ============================================================================
// ReSTIR GI 渲染
// ============================================================================

/**
 * @brief 计算直接光照（使用 ReSTIR 采样）
 * 
 * 这是 ReSTIR GI 的核心简化版：
 * 1. 从多个光源收集样本到蓄水池
 * 2. 重采样得到一个最优样本
 * 3. 用修正权重计算最终光照
 */
Color compute_direct_lighting(Vec3 hitPoint, Vec3 normal, Reservoir* reservoir) {
    reservoir_reset(reservoir);
    
    // 1. 收集所有光源的样本到蓄水池
    for (int i = 0; i < numLights; i++) {
        ReservoirSample sample;
        sample.lightIndex = i;
        
        // 计算到光源的方向
        Vec3 lightDir = vec3_sub(lights[i].position, hitPoint);
        float dist = vec3_length(lightDir);
        lightDir = vec3_normalize(lightDir);
        
        sample.sampleDir = lightDir;
        
        // 计算重要性采样权重
        // w = L * f / (p * N)
        // 简化：假设均匀采样，pdf = 1/numLights
        float pdf = 1.0f / (float)numLights;
        
        // 衰减（距离平方反比）
        float attenuation = 1.0f / (dist * dist);
        
        // Lambertian BRDF: f = albedo / π
        float brdf = 1.0f / (float)M_PI;
        
        // 权重 = 光照强度 * BRDF * 衰减 / pdf
        sample.weight = attenuation * brdf / pdf;
        sample.pdf = pdf;
        
        // 更新蓄水池
        reservoir_update(reservoir, sample, sample.weight);
    }
    
    // 2. 重采样得到最终样本
    ReservoirSample final = reservoir_resample(reservoir);
    
    if (final.lightIndex < 0) {
        return color(0, 0, 0);
    }
    
    // 3. 计算最终光照（使用修正权重）
    float W = reservoir_get_weight(reservoir);
    
    Light light = lights[final.lightIndex];
    Vec3 lightDir = vec3_sub(light.position, hitPoint);
    float dist = vec3_length(lightDir);
    float attenuation = 1.0f / (dist * dist);
    
    // Lambertian: max(0, n·l)
    float nDotL = fmaxf(0, vec3_dot(normal, vec3_normalize(lightDir)));
    
    Color radiance = color_mul(light.intensity, attenuation * nDotL);
    Color result = color_mul(radiance, W);
    
    return result;
}

/**
 * @brief 渲染单个像素
 */
Color render_pixel(int x, int y, Reservoir* reservoir) {
    // 计算射线方向（简化相机模型）
    float u = (2.0f * x - IMAGE_WIDTH) / IMAGE_HEIGHT;
    float v = (IMAGE_HEIGHT - 2.0f * y) / IMAGE_HEIGHT;
    
    Ray ray;
    ray.origin = vec3(0, 0, 0);
    ray.direction = vec3_normalize(vec3(u, v, -1));
    
    // 查找交点
    int hitIndex;
    float t = find_nearest_hit(ray, &hitIndex);
    
    if (t < 0) {
        // 背景色（渐变）
        float sky = 0.5f * (ray.direction.y + 1.0f);
        return color_add(color_mul(color(1, 1, 1), 1 - sky),
                        color_mul(color(0.5f, 0.7f, 1.0f), sky));
    }
    
    // 计算交点位置
    Vec3 hitPoint = vec3_add(ray.origin, vec3_mul(ray.direction, t));
    Vec3 normal = get_sphere_normal(hitPoint, spheres[hitIndex]);
    
    // 获取表面颜色
    Color albedo = spheres[hitIndex].albedo;
    
    // 使用 ReSTIR 计算直接光照
    Color direct = compute_direct_lighting(hitPoint, normal, reservoir);
    
    // 简单的环境光
    Color ambient = color(0.02f, 0.02f, 0.03f);
    
    // 最终颜色 = 表面颜色 * (直接光 + 环境光)
    Color result = color_mul_color(albedo, color_add(direct, ambient));
    
    // Gamma 校正
    result.r = powf(fminf(1, result.r), 1.0f / 2.2f);
    result.g = powf(fminf(1, result.g), 1.0f / 2.2f);
    result.b = powf(fminf(1, result.b), 1.0f / 2.2f);
    
    return result;
}

// ============================================================================
// 图像输出（PPM 格式）
// ============================================================================

void save_ppm(const char* filename, Color* image) {
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
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║     ReSTIR GI CPU 简化实现（教学演示版）          ║\n");
    printf("║     ReSTIR: Reservoir Spatiotemporal             ║\n");
    printf("║            Importance Resampling                 ║\n");
    printf("╚══════════════════════════════════════════════════╝\n\n");
    
    // 初始化随机数
    srand((unsigned int)time(NULL));
    
    // 初始化场景
    init_scene();
    printf("✓ 场景初始化完成\n");
    printf("  - 球体数量：%d\n", numSpheres);
    printf("  - 光源数量：%d\n", numLights);
    printf("  - 图像尺寸：%dx%d\n", IMAGE_WIDTH, IMAGE_HEIGHT);
    printf("  - 蓄水池容量：%d\n\n", RESERVOIR_SIZE);
    
    // 分配图像内存
    Color* image = (Color*)malloc(IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(Color));
    if (!image) {
        printf("错误：内存分配失败\n");
        return 1;
    }
    
    // 创建蓄水池（每个像素一个）
    Reservoir* reservoirs = (Reservoir*)malloc(IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(Reservoir));
    if (!reservoirs) {
        printf("错误：内存分配失败\n");
        free(image);
        return 1;
    }
    
    // 初始化所有蓄水池
    for (int i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; i++) {
        reservoir_reset(&reservoirs[i]);
    }
    
    printf("开始渲染...\n");
    clock_t start = clock();
    
    // 渲染每个像素
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            int idx = y * IMAGE_WIDTH + x;
            image[idx] = render_pixel(x, y, &reservoirs[idx]);
        }
        
        // 进度显示
        if ((y + 1) % 64 == 0) {
            printf("  进度：%d%% (%d/%d 行)\n", 
                   (y + 1) * 100 / IMAGE_HEIGHT, y + 1, IMAGE_HEIGHT);
        }
    }
    
    clock_t end = clock();
    float duration = (float)(end - start) / CLOCKS_PER_SEC;
    
    printf("\n✓ 渲染完成！耗时：%.2f 秒\n", duration);
    printf("  性能：%.2f 像素/秒\n", 
           (float)(IMAGE_WIDTH * IMAGE_HEIGHT) / duration);
    
    // 保存图像
    save_ppm("restir_output.ppm", image);
    
    // 打印 ReSTIR 核心公式
    printf("\n╔══════════════════════════════════════════════════╗\n");
    printf("║  ReSTIR 核心公式：                                ║\n");
    printf("║                                                  ║\n");
    printf("║  蓄水池权重修正：                                 ║\n");
    printf("║  W = Σwᵢ / M                                      ║\n");
    printf("║                                                  ║\n");
    printf("║  其中：                                           ║\n");
    printf("║  - wᵢ: 第 i 个样本的重要性权重                      ║\n");
    printf("║  - M: 候选样本总数                                ║\n");
    printf("║  - W: 修正后的权重（保证无偏估计）                 ║\n");
    printf("╚══════════════════════════════════════════════════╝\n");
    
    // 清理内存
    free(image);
    free(reservoirs);
    
    printf("\n按回车键退出...\n");
    getchar();
    
    return 0;
}
