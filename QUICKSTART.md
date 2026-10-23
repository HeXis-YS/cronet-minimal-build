# 快速开始

## 对于用户：部署 GitHub 仓库

### 1. 创建 GitHub 仓库

```bash
# 在 GitHub 上创建新仓库 (public/private 均可)
# 仓库名: cronet-minimal-build
```

### 2. 推送代码

```bash
cd /tmp/cronet-minimal-build

# 添加远程仓库
git remote add origin git@github.com:YOUR_USERNAME/cronet-minimal-build.git

# 推送到 GitHub
git branch -M main
git push -u origin main
```

### 3. 等待自动构建

- 访问 `https://github.com/YOUR_USERNAME/cronet-minimal-build/actions`
- 查看 "Build Cronet Minimal" workflow
- 等待 4-6 小时（首次构建）

### 4. 下载构建产物

构建成功后：

```bash
# 方式1: 从 Actions 下载 artifacts
# 前往 Actions 页面 → 选择最新的 workflow → 下载 artifacts

# 方式2: 如果打了 tag，从 Releases 下载
git tag v154.0.0
git push origin v154.0.0
# 前往 Releases 页面下载
```

### 5. 验证构建

```bash
# 解压
tar -xzf cronet-minimal-linux-x64.tar.gz

# 检查符号表
nm -D libcronet_minimal.so | grep Cronet_Engine
# 应该看到:
# 00001234 T Cronet_Engine_Create
# 00005678 T Cronet_Engine_Destroy
# ...

# 检查依赖
ldd libcronet_minimal.so
# 应该只依赖系统库:
# linux-vdso.so.1
# libstdc++.so.6
# libm.so.6
# libgcc_s.so.1
# libc.so.6
```

## 对于开发者：本地构建

⚠️ **不推荐**: 需要 100GB 磁盘和 4-6 小时。优先使用 GitHub Actions。

如果确实需要本地构建，参考 [BUILD.md](BUILD.md)。

## 下一步：Rust 集成

构建成功后，前往 hray-core 集成：

```bash
cd ~/桌面/hray-core

# 创建 Rust bindings
cargo new --lib cronet-rs
cd cronet-rs

# 复制头文件和库
mkdir -p include lib
cp /tmp/cronet-minimal-build/include/*.h include/
cp path/to/libcronet_minimal.so lib/

# 配置 Cargo.toml 和 build.rs
# 参考 INTEGRATION.md
```

详细集成指南: [INTEGRATION.md](INTEGRATION.md)

## 故障排除

### 构建失败

1. **检查 Actions 日志**
   ```
   前往 Actions 页面 → 点击失败的 workflow → 查看详细日志
   ```

2. **常见错误**
   - API 不兼容: 需要适配 v119→v154 的代码变化
   - 依赖缺失: 检查 BUILD.gn 的 deps
   - 磁盘空间不足: GitHub Actions runner 应该有足够空间

3. **获取帮助**
   - 查看 STATUS.md 中的"可能的问题"部分
   - 搜索 Chromium 源码了解 API 变化
   - 参考 naiveproxy 的适配方式

### 性能问题

如果 CI 构建时间过长（超过 8 小时）：

1. **检查 runner 状态**: GitHub Actions 可能繁忙
2. **使用 ccache**: 可以在 workflow 中添加缓存
3. **减少并行度**: 调整 ninja 的 -j 参数

### 使用问题

如果 Rust bindings 遇到困难：

1. **检查 bindgen 版本**: 需要 0.70+
2. **验证头文件路径**: 确保 include/ 可访问
3. **链接错误**: 确认 libcronet_minimal.so 路径正确

## 资源

- **项目状态**: [STATUS.md](STATUS.md)
- **构建指南**: [BUILD.md](BUILD.md)
- **集成指南**: [INTEGRATION.md](INTEGRATION.md)
- **Chromium 文档**: https://www.chromium.org/developers/
- **naiveproxy**: https://github.com/klzgrad/naiveproxy
- **cronet-binaries**: https://github.com/sleeyax/cronet-binaries

## 时间线

| 阶段 | 任务 | 预计时间 | 状态 |
|------|------|----------|------|
| Phase 0 | 提取与准备 | 5h | ✅ 完成 |
| Phase 1 | CI 构建验证 | 4-6h (等待) + 10-15h (适配) | ⏳ 进行中 |
| Phase 2 | Rust bindings | 5-8h | ⏸️ 待开始 |
| Phase 3 | XHTTP 集成 | 3-5h | ⏸️ 待开始 |
| Phase 4 | 测试优化 | 3-5h | ⏸️ 待开始 |

**总预计**: 30-44 小时（含等待时间）

## 成功标准

✅ **Phase 1**: 
- libcronet_minimal.so 成功编译
- 导出所有 Cronet_Engine_* 和 Cronet_UrlRequest_* 符号
- 无 undefined references

✅ **Phase 2**:
- bindgen 成功生成 Rust bindings
- Safe wrapper 通过基本单元测试
- 可以创建 Engine 并发起简单 HTTP 请求

✅ **Phase 3**:
- hray-core 使用 Cronet 成功建立 XHTTP 连接
- 流量特征与真实 Chrome 浏览器一致
- 性能满足代理场景要求（延迟 < rustls * 1.2，吞吐量 > rustls * 0.8）

✅ **最终目标**:
- 完全替代 rustls fork
- TLS 指纹无法与真实 Chrome 区分
- 年度维护成本 < 50h

## 许可证

本项目基于 Chromium 的 BSD 许可证。详见 Chromium 源码中的 LICENSE 文件。
