# Cronet Minimal Build

Chromium Cronet 最小子集的自动化构建系统。

## 目标

为 hray-core XHTTP 提供完美的 Chrome TLS 指纹，同时最小化维护成本。

## 特性

- 只包含 UrlRequest 核心 API（约 35% 的完整 Cronet）
- 自动化 CI 构建（GitHub Actions）
- 支持 Linux x64/arm64
- 基于 Chromium 154.0.8037.49

## 构建产物

- `libcronet_minimal.so` - 共享库
- `cronet_c.h` - C API 头文件
- `cronet_export.h` - 导出宏

## 本地构建

需要 Chromium 154 构建环境。详见 [BUILD.md](BUILD.md)

## CI 构建

Push 到 `main` 分支自动触发构建，产物上传到 GitHub Releases。

## 维护策略

每季度评估 Chromium 稳定版，只跟进影响 UrlRequest 的破坏性变更。

预期维护成本：35-50 小时/年

## License

基于 Chromium 的 BSD 许可证。
