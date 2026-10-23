# hray-core 集成指南

本文档说明如何在 hray-core 中集成 Cronet Minimal。

## 架构概览

```
hray-core (Rust)
    ↓
cronet-rs (Rust bindings)
    ↓
libcronet_minimal.so (C API)
    ↓
Chromium network stack
```

## Rust Bindings

参考 sleeyax/cronet-rs 的实现模式，但只绑定我们需要的最小子集：

### 需要绑定的 C API

```c
// Engine lifecycle
Cronet_EnginePtr Cronet_Engine_Create();
void Cronet_Engine_Destroy(Cronet_EnginePtr engine);
Cronet_RESULT Cronet_Engine_StartWithParams(
    Cronet_EnginePtr engine,
    Cronet_EngineParamsPtr params);
void Cronet_Engine_Shutdown(Cronet_EnginePtr engine);

// UrlRequest
Cronet_UrlRequestPtr Cronet_UrlRequest_Create();
void Cronet_UrlRequest_Destroy(Cronet_UrlRequestPtr request);
Cronet_RESULT Cronet_UrlRequest_InitWithParams(
    Cronet_UrlRequestPtr request,
    Cronet_EnginePtr engine,
    Cronet_String url,
    Cronet_UrlRequestParamsPtr params,
    Cronet_UrlRequestCallbackPtr callback);
Cronet_RESULT Cronet_UrlRequest_Start(Cronet_UrlRequestPtr request);
void Cronet_UrlRequest_Cancel(Cronet_UrlRequestPtr request);

// Callbacks
Cronet_UrlRequestCallbackPtr Cronet_UrlRequestCallback_CreateWith(...);
```

### bindgen 配置

```rust
// build.rs
use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rustc-link-lib=cronet_minimal");
    println!("cargo:rustc-link-search=native=/path/to/lib");

    let bindings = bindgen::Builder::default()
        .header("include/cronet_c.h")
        .allowlist_function("Cronet_.*")
        .allowlist_type("Cronet_.*")
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
```

## XHTTP 协议适配

### 当前 rustls 方案

```rust
// 当前使用修改版 rustls，内置 JA3/JA4 指纹
let connector = TlsConnector::builder()
    .with_fingerprint(Fingerprint::Chrome120)
    .build()?;
```

### Cronet 方案

```rust
// 使用 Cronet 提供的完整浏览器栈
let engine = CronetEngine::builder()
    .user_agent("Chrome/154.0.0.0")
    .enable_http2(true)
    .enable_quic(false)  // H3 support 后续添加
    .build()?;

let request = engine.new_request("https://example.com")?;
request.set_method("POST");
request.set_header("X-Padding", &padding_value);
request.write_body(xhttp_payload)?;
```

### Padding 策略

XHTTP 协议有混淆 padding，需要评估：

1. **保留当前 padding** - 在 HTTP 层添加
2. **依赖 Cronet padding** - 检查 Chromium 是否有内置 padding
3. **双层 padding** - 如果不冲突可以叠加

需要抓包对比：
- hray-core + rustls + XHTTP padding
- hray-core + Cronet + XHTTP padding
- 真实 Chrome 浏览器

## 连接池与并发

### 潜在问题

Cronet 默认有连接池限制（per-host、per-proxy 等），需要验证：

1. 是否影响代理场景的并发连接数
2. 能否通过参数调整
3. 是否需要多 Engine 实例

### 测试方案

```rust
// 压力测试：同时发起 N 个请求到同一目标
for i in 0..1000 {
    tokio::spawn(async move {
        engine.request(url).send().await?;
    });
}
```

## 性能对比

需要对比的指标：

| 指标 | rustls 方案 | Cronet 方案 |
|------|-------------|-------------|
| 二进制大小 | ~5MB | ~15-20MB |
| 内存占用 | ~10MB | ~30-50MB |
| TLS 握手延迟 | 基准 | 待测 |
| 吞吐量 | 基准 | 待测 |
| CPU 占用 | 基准 | 待测 |

## 维护成本对比

### rustls fork 方案

- **初始成本**: 15-20h（已完成）
- **年度维护**: 60-80h
  - 跟进 rustls 上游变化
  - 更新 JA3/JA4 指纹
  - 适配 hray-core 变化
- **风险**: rustls API 变化可能导致大量改动

### Cronet 最小子集方案

- **初始成本**: 18-28h
  - v119→v154 适配: 10-15h（待验证）
  - Rust bindings: 5-8h
  - XHTTP 集成: 3-5h
- **年度维护**: 35-50h
  - 季度评估 Chromium 稳定版: 12h/年
  - 破坏性变更适配: 0.5次/年 × 25h = 12.5h
  - 预编译库更新: 4h/年
  - Rust bindings 维护: 6-10h/年
- **风险**: UrlRequest 核心 API 相对稳定

**结论**: Cronet 方案长期维护成本降低 40-50%

## 部署与分发

### 预编译库

GitHub Actions 自动构建产物：
- Linux x64: `libcronet_minimal.so`
- Linux arm64: `libcronet_minimal.so`

### Cargo 集成

```toml
[dependencies]
cronet-minimal = { version = "154.0", features = ["bundled"] }
```

或动态链接系统库：
```toml
[dependencies]
cronet-minimal = { version = "154.0", features = ["system"] }
```

### 分发策略

1. **选项A**: 静态链接到 hray-core
   - 优点：单一二进制，无依赖
   - 缺点：体积增加 15-20MB

2. **选项B**: 动态库分发
   - 优点：多个程序共享
   - 缺点：部署需要额外文件

推荐：静态链接，简化部署

## 后续步骤

### Phase 1: 验证编译 (1-2 天)

1. 等待 GitHub Actions 首次构建完成
2. 检查编译错误，修复 v119→v154 API 变化
3. 验证生成的 .so 文件符号表

### Phase 2: Rust bindings (3-5 天)

1. 使用 bindgen 生成初始绑定
2. 创建 safe Rust wrapper
3. 实现异步支持（tokio integration）
4. 单元测试：基本请求、POST、headers

### Phase 3: XHTTP 集成 (5-7 天)

1. 替换 rustls connector 为 Cronet
2. 迁移 padding 逻辑
3. 处理错误转换和重试逻辑
4. 集成测试：完整 XHTTP 流程

### Phase 4: 验证与优化 (3-5 天)

1. 流量特征对比（抓包分析）
2. 性能基准测试
3. 内存泄漏检查（valgrind）
4. 并发连接测试

### Phase 5: H3 支持（可选，后期）

1. 评估 QUIC/H3 是否对 XHTTP 有价值
2. 如果需要，在 Cronet 中启用 QUIC
3. 适配 H3 的 padding 策略

## 风险与缓解

### 风险1：v119→v154 API 变化过大

**缓解**: 
- 已移除 grpc_support（最大变化）
- 其他依赖（base、net、cronet_common）在两版本间都存在
- 最坏情况：回退到修改 naiveproxy 的 v154 实现

### 风险2：性能不满足要求

**缓解**:
- Chromium 网络栈高度优化，性能应该优于 rustls
- 如果内存占用过高，可以调整 Cronet 参数
- 保留 rustls 方案作为 fallback

### 风险3：流量特征仍然被识别

**缓解**:
- Cronet 是 Chrome 的真实网络栈，特征最接近浏览器
- 如果还不够，需要完整 Chrome（naiveproxy 路径）
- 评估是否需要 JavaScript 执行等更高层特征

### 风险4：Rust bindings 维护负担

**缓解**:
- C API 相对稳定
- bindgen 自动生成绑定
- safe wrapper 只需维护我们使用的子集

## 决策点

完成 Phase 1 后，根据实际编译结果决定：

1. **继续 Cronet 路径** - 如果适配成本在预期内（< 20h）
2. **转向 naiveproxy** - 如果 v119→v154 变化过大
3. **保持 rustls** - 如果 Cronet 引入的复杂度不值得

## 参考资源

- sleeyax/cronet-binaries: v119 补丁和预编译库
- sleeyax/cronet-rs: Rust bindings 参考实现
- klzgrad/naiveproxy: 完整 Chromium 栈方案
- Chromium 源码: components/cronet/
- XHTTP 协议: ../xray-core/transport/internet/xhttp/

## 联系与反馈

如果集成过程中发现 Cronet Minimal API 不足，需要扩展：

1. 评估新 API 的必要性
2. 检查是否会引入新的维护负担
3. 更新提取脚本和 BUILD.gn
4. 重新评估维护成本
