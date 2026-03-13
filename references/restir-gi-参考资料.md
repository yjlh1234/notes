# ReSTIR GI 参考资料

> **ReSTIR GI** (Reservoir Spatiotemporal Importance Resampling for Global Illumination) 是一种用于实时全局照明的先进采样技术，基于 ReSTIR 框架扩展而来。

---

## 📄 原始论文与核心研究

### 1. ReSTIR 原始论文 (2020)
- **标题**: Spatiotemporal Reservoir Resampling for Real-Time Ray Tracing with Dynamic Direct Lighting
- **作者**: Benedikt Bitterli, Kai Zhang, et al. (NVIDIA)
- **发表**: SIGGRAPH 2020
- **URL**: https://research.nvidia.com/sites/default/files/pubs/2020-07_Spatiotemporal-reservoir-resampling/ReSTIR.pdf
- **核心内容**: 
  - 提出了 ReSTIR (Reservoir Spatiotemporal Importance Resampling) 基础框架
  - 实现了时空重采样技术，用于高效直接光照计算
  - 为后续 ReSTIR GI 奠定了基础

### 2. ReSTIR GI 论文 (2022)
- **标题**: ReSTIR GI: Reservoir Spatiotemporal Importance Resampling for Global Illumination
- **作者**: Benedikt Bitterli, Kai Zhang, et al.
- **发表**: SIGGRAPH 2022
- **URL**: https://www.dgp.toronto.edu/projects/restir-gi/ (项目页面)
- **DOI**: https://doi.org/10.1145/3528223.3530166
- **核心内容**:
  - 将 ReSTIR 扩展到全局照明 (GI) 场景
  - 实现了路径引导 (path guiding) 与时空复用的结合
  - 支持动态场景中的实时全局光照
  - 显著减少噪声，提高收敛速度

### 3. ReSTIR SSS - 次表面散射扩展 (2024)
- **标题**: ReSTIR Subsurface Scattering for Real-Time Path Tracing
- **作者**: Mirco Werner, Vincent Schüßler, Carsten Dachsbacher
- **发表**: HPG 2024 (High Performance Graphics)
- **URL**: https://doi.org/10.1145/3675372
- **GitHub**: https://github.com/MircoWerner/ReSTIR-SSS
- **核心内容**:
  - 将 ReSTIR 应用于次表面散射 (SSS)
  - 提出了混合移位策略 (hybrid shift) 和顺序移位 (sequential shift)
  - 实时性能下显著减少噪声和去噪伪影

### 4. ReSTIR FG - 光子最终聚集 (EGSR)
- **标题**: ReSTIR FG: Real-Time Reservoir Resampled Photon Final Gathering
- **作者**: René Kern, Felix Brüll, Thorsten Grosch (TU Clausthal)
- **发表**: EGSR (Eurographics Symposium on Rendering)
- **URL**: https://diglib.eg.org/items/df98f89d-a0ca-4800-9bc4-74528feaf872
- **GitHub**: https://github.com/JanSpindler/ReSTIR-FG
- **核心内容**:
  - 结合光子最终聚集与 ReSTIR 重采样
  - 支持实时焦散 (caustics) 渲染
  - 集成 RTXDI 用于直接光照，支持 DLSS 和 NRD 去噪

### 5. Volume ReSTIR - 体积渲染扩展
- **标题**: Fast Volume Rendering with Spatiotemporal Reservoir Resampling
- **作者**: Zhihao Ruan, Shubham Sharma, Raymond Yang (University of Pennsylvania)
- **发表**: 2021
- **URL**: https://research.nvidia.com/publication/2021-11_Fast-Volume-Rendering
- **GitHub**: https://github.com/TheSmokeyGuys/Volume-ReSTIR-Vulkan
- **核心内容**:
  - 将 ReSTIR 扩展到体积渲染 (烟雾、云、火焰等)
  - 基于 Vulkan 光线追踪和 OpenVDB 实现
  - 支持体素网格数据的实时路径追踪

---

## 🎮 NVIDIA 官方资源

### 6. NVIDIA Developer Blog - ReSTIR 系列
- **来源**: NVIDIA Technical Blog
- **URL**: https://developer.nvidia.com/blog/tag/restir/
- **内容**: 
  - ReSTIR 技术介绍和更新
  - RTXDI (Real-Time Denoised Direct Illumination) 集成指南
  - 与 DLSS、OptiX 等技术的结合使用

### 7. NVIDIA Falcor 框架
- **描述**: NVIDIA 的实时渲染研究框架，支持 ReSTIR 实现
- **GitHub**: https://github.com/NVIDIAGameWorks/Falcor
- **用途**: 
  - ReSTIR GI 参考实现的开发平台
  - 提供完整的渲染管线和工具链
  - 支持 DirectX 12 光线追踪

---

## 💻 GitHub 实现与教程

### 8. Restir_CPP - C++ 实现 (Falcor 8.0)
- **作者**: Trylz
- **URL**: https://github.com/Trylz/Restir_CPP
- **视频演示**: https://www.youtube.com/watch?v=pyQrblYP_VY
- **核心内容**:
  - 在 Falcor 8.0 中的完整 ReSTIR 实现
  - 包含源代码和测试场景
  - 需要 CUDA 12.6 和 RTX 显卡

### 9. Falcor-ReSTIR - ReSTIR DI 实现
- **作者**: Karel Tománek (CTU Prague)
- **URL**: https://github.com/karel-tomanec/Falcor-ReSTIR
- **视频**: https://youtu.be/CCJOeeGfKTU
- **论文**: 硕士论文 "Efficient Sampling for Computing Complex Illumination in Real-Time"
- **核心内容**:
  - ReSTIR DI (Direct Illumination) 的完整实现
  - 包含详细的渲染图脚本和配置
  - 适合作为学习 ReSTIR 的入门项目

### 10. 其他 ReSTIR 实现
- **RestirFalcor**: https://github.com/Trylz/RestirFalcor
- **ReSTIR DI Raytracer**: https://github.com/XiaoyiHu1998/ReSTIR_DI_Raytracer
- **C++ ReSTIR**: https://github.com/MrMagnifico/cpp-restir
- **CUDA ReSTIR**: https://github.com/HummaWhite/ReSTIR
- **ReSTIR-FG**: https://github.com/JanSpindler/ReSTIR-FG (光子最终聚集)

---

## 🎤 GDC 演讲与视频资源

### 11. GDC Vault - ReSTIR 相关演讲
- **URL**: https://www.gdcvault.com/
- **搜索关键词**: "ReSTIR", "Global Illumination", "Ray Tracing"
- **内容**: 
  - 游戏开发中的 ReSTIR 应用案例
  - NVIDIA 工程师的技术分享
  - 实时渲染最佳实践

### 12. YouTube 技术演示
- **Restir_CPP 演示**: https://www.youtube.com/watch?v=pyQrblYP_VY
- **Falcor-ReSTIR 演示**: https://youtu.be/CCJOeeGfKTU
- **ReSTIR-FG 演示**: https://youtu.be/7esNRZSQxQA

---

## 📚 学习路径建议

### 入门顺序:
1. **先读原始 ReSTIR 论文 (2020)** - 理解基础概念
2. **查看 Falcor-ReSTIR 项目** - 动手实践 DI 实现
3. **阅读 ReSTIR GI 论文 (2022)** - 扩展到全局光照
4. **探索扩展变体** (SSS, FG, Volume) - 了解应用范围

### 前置知识:
- 蒙特卡洛积分与重要性采样
- 路径追踪基础
- 光线追踪 (DirectX 12 / Vulkan RT)
- GPU 编程 (CUDA / HLSL / GLSL)

### 开发环境:
- Windows 10/11 (20H2 或更新)
- Visual Studio 2019/2022
- CUDA Toolkit (12.6 推荐)
- RTX 系列显卡 (支持 DXR/Vulkan RT)
- Falcor 渲染框架

---

## 🔗 相关技术

- **RTXDI**: NVIDIA 实时直接光照解决方案
- **DLSS**: 深度学习超级采样
- **NRD**: NVIDIA 光线追踪去噪器
- **OptiX**: NVIDIA 光线追踪引擎
- **Falcor**: NVIDIA 实时渲染研究框架

---

*最后更新: 2026-03-13*
*整理者：辣辣 (Lala) 🌶️*
