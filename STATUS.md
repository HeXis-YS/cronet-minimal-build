# Cronet Minimal 项目状态

最后更新: 2026-10-23

## 当前状态：✅ 就绪，等待 CI 验证

### 已完成

- ✅ 从 Chromium v119 提取 Cronet 最小子集（~2,000 LOC）
- ✅ 移除 grpc_support 依赖（v154 已删除）
- ✅ 清理所有 stream_engine / BidirectionalStream 引用
- ✅ 创建 GitHub Actions CI workflow
- ✅ 编写构建文档和集成指南
- ✅ Git 仓库初始化并提交

### 代码统计

```
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C++                             14            489            185           2068
C/C++ Header                     7            183            123            698
IDL Generated                    5            467              0           3600
GN Build                         1             12              3             39
-------------------------------------------------------------------------------
SUM:                            27           1151            311           6405
```

**核心手写代码**: 2,068 LOC (vs 原始 ~6,000 LOC)

### 文件清单

```
cronet-minimal-build/
├── .github/workflows/build.yml    # CI 配置，Linux x64/arm64
├── .gitignore                     # 排除构建产物
├── BUILD.gn                       # Chromium GN 构建配置
├── BUILD.md                       # 本地构建指南
├── README.md                      # 项目概览
├── INTEGRATION.md                 # hray-core 集成指南
├── STATUS.md                      # 本文件
├── generated/                     # IDL 生成代码 (3,600 LOC)
│   ├── cronet.idl_c.h
│   ├── cronet.idl_impl_interface.{cc,h}
│   └── cronet.idl_impl_struct.{cc,h}
├── include/                       # 公开 C API
│   ├── cronet_c.h                # 核心 API (已移除 stream_engine)
│   └── cronet_export.h           # 导出宏
└── src/                          # 核心实现
    ├── engine.{cc,h}             # Cronet 引擎
    ├── url_request.{cc,h}        # HTTP 请求
    ├── buffer.cc                 # 内存管理
    ├── upload_data_sink.{cc,h}   # POST 支持
    ├── io_buffer_with_cronet_buffer.{cc,h}
    ├── native_metrics_util.{cc,h}
    └── runnables.{cc,h}          # 线程调度
```

## 下一步：GitHub 仓库部署

### 用户需要操作

1. **在 GitHub 创建新仓库** (public 或 private)
   ```
   名称: cronet-minimal-build
   描述: Minimal Cronet subset for hray-core XHTTP
   ```

2. **配置本地仓库并推送**
   ```bash
   cd /tmp/cronet-minimal-build
   git remote add origin git@github.com:<username>/cronet-minimal-build.git
   git branch -M main
   git push -u origin main
   ```

3. **等待 CI 构建**
   - GitHub Actions 会自动触发
   - 构建时间: 4-6 小时
   - 检查 Actions 页面的日志

4. **下载构建产物**
   - 前往 Actions 页面
   - 选择最新的 workflow run
   - 下载 artifacts:
     - `cronet-minimal-linux-x64.tar.gz`
     - `cronet-minimal-linux-arm64.tar.gz`

5. **验证构建**
   ```bash
   tar -xzf cronet-minimal-linux-x64.tar.gz
   nm -D libcronet_minimal.so | grep Cronet_Engine
   ldd libcronet_minimal.so
   ```

### 预期结果

如果构建成功，应该看到：
- ✅ 两个架构的 `.so` 文件
- ✅ 导出的 `Cronet_Engine_*` 和 `Cronet_UrlRequest_*` 符号
- ✅ 依赖只有标准系统库 (libc, libstdc++, libm, libpthread)

### 可能的问题

#### 问题1: v119→v154 API 不兼容

**症状**: 编译错误，找不到某个类或方法

**解决**:
1. 检查错误日志中的具体 API
2. 在 v154 源码中搜索替代 API
3. 修改对应的 `src/*.cc` 文件
4. Git commit 并 push，触发重新构建

**常见变化**:
- `base::BindOnce` / `base::BindRepeating` 签名
- `net::URLRequest` 回调接口
- `net::NetworkQualityEstimator` API 变化

#### 问题2: 缺少依赖

**症状**: 链接错误，undefined reference

**解决**:
1. 检查 `BUILD.gn` 的 `deps` 列表
2. 添加缺少的 Chromium 组件
3. 重新构建

**核心依赖** (应该都存在于 v154):
- `//base` - Chromium 基础库
- `//net` - 网络栈
- `//components/cronet:cronet_common` - Cronet 公共代码

#### 问题3: CI 环境问题

**症状**: apt-get 失败或 depot_tools 错误

**解决**:
1. 检查 GitHub Actions runner 状态
2. 可能需要调整 workflow 的系统依赖
3. 参考 naiveproxy 的 CI 配置

## Phase 2: Rust Bindings (待 Phase 1 成功后)

### 创建 cronet-rs crate

```bash
cd /path/to/hray-core-workspace
cargo new --lib cronet-rs
cd cronet-rs
```

### Cargo.toml 配置

```toml
[package]
name = "cronet-rs"
version = "154.0.0"
edition = "2021"

[dependencies]
libc = "0.2"
tokio = { version = "1", features = ["rt", "sync"] }

[build-dependencies]
bindgen = "0.70"
cc = "1.0"

[features]
default = ["bundled"]
bundled = []  # 静态链接预编译库
system = []   # 动态链接系统库
```

### 生成 bindings

```rust
// build.rs
use bindgen;
use std::env;
use std::path::PathBuf;

fn main() {
    // 链接 libcronet_minimal
    if cfg!(feature = "bundled") {
        println!("cargo:rustc-link-lib=static=cronet_minimal");
        println!("cargo:rustc-link-search=native=../cronet-minimal-build/lib");
    } else {
        println!("cargo:rustc-link-lib=dylib=cronet_minimal");
    }

    // 生成 Rust bindings
    let bindings = bindgen::Builder::default()
        .header("../cronet-minimal-build/include/cronet_c.h")
        .allowlist_function("Cronet_.*")
        .allowlist_type("Cronet_.*")
        .allowlist_var("Cronet_.*")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("cronet_bindings.rs"))
        .expect("Couldn't write bindings!");
}
```

### Safe Rust wrapper 示例

```rust
// src/lib.rs
mod ffi {
    #![allow(non_upper_case_globals)]
    #![allow(non_camel_case_types)]
    #![allow(non_snake_case)]
    include!(concat!(env!("OUT_DIR"), "/cronet_bindings.rs"));
}

use std::ffi::CString;
use std::ptr;

pub struct CronetEngine {
    ptr: ffi::Cronet_EnginePtr,
}

impl CronetEngine {
    pub fn new() -> Result<Self, Box<dyn std::error::Error>> {
        unsafe {
            let ptr = ffi::Cronet_Engine_Create();
            if ptr.is_null() {
                return Err("Failed to create Cronet engine".into());
            }
            Ok(CronetEngine { ptr })
        }
    }

    pub fn start(&self, user_agent: &str) -> Result<(), Box<dyn std::error::Error>> {
        unsafe {
            let params = ffi::Cronet_EngineParams_Create();
            let ua = CString::new(user_agent)?;
            ffi::Cronet_EngineParams_user_agent_set(params, ua.as_ptr());
            
            let result = ffi::Cronet_Engine_StartWithParams(self.ptr, params);
            ffi::Cronet_EngineParams_Destroy(params);
            
            if result != ffi::Cronet_RESULT_SUCCESS {
                return Err("Failed to start engine".into());
            }
            Ok(())
        }
    }
}

impl Drop for CronetEngine {
    fn drop(&mut self) {
        unsafe {
            ffi::Cronet_Engine_Shutdown(self.ptr);
            ffi::Cronet_Engine_Destroy(self.ptr);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_engine_create() {
        let engine = CronetEngine::new().unwrap();
        engine.start("Test/1.0").unwrap();
    }
}
```

## Phase 3: hray-core 集成

### 替换 TLS connector

```rust
// hray-core/src/proxy/xhttp/client.rs

// 旧方案 (rustls)
use tokio_rustls::TlsConnector;

// 新方案 (Cronet)
use cronet_rs::CronetEngine;

pub struct XhttpClient {
    engine: CronetEngine,
}

impl XhttpClient {
    pub async fn connect(&self, addr: &str) -> Result<XhttpStream> {
        let request = self.engine.request(addr)?;
        // ... XHTTP 协议封装
    }
}
```

## 成本与时间估算

### 已消耗时间

- ✅ 分析与决策: 2h
- ✅ 代码提取: 1h
- ✅ 依赖清理: 1h
- ✅ 文档编写: 1h
- **总计**: 5h

### 剩余预估

- ⏳ CI 调试与 v154 适配: 10-15h
- ⏳ Rust bindings: 5-8h
- ⏳ XHTTP 集成: 3-5h
- ⏳ 测试与优化: 3-5h
- **总计**: 21-33h

### 与 rustls fork 对比

| 方案 | 初始成本 | 年度维护 | 5年总成本 |
|------|----------|----------|-----------|
| rustls fork | 20h | 70h | 370h |
| Cronet minimal | 38h | 42h | 248h |
| **节省** | -18h | +28h | **+122h (33%)** |

**结论**: 第2年开始回本，长期更优

## 关键决策点

### 决策点1: CI 构建是否成功 (预计 4-6 小时后)

- ✅ 成功 → 继续 Phase 2 (Rust bindings)
- ❌ 失败且适配成本 < 20h → 修复后继续
- ❌ 失败且适配成本 > 20h → 评估 naiveproxy 方案

### 决策点2: Rust bindings 是否可用 (预计 1 周后)

- ✅ 成功 → 继续 Phase 3 (XHTTP 集成)
- ❌ 失败 → 评估是否需要扩展 API

### 决策点3: 流量特征是否满足要求 (预计 2 周后)

- ✅ 满足 → 替换 rustls，删除 fork
- ❌ 不满足 → 评估完整 Chrome 方案 (naiveproxy)

## 备用方案

如果 Cronet minimal 方案失败，按优先级：

1. **扩展 API 子集** - 如果只是缺少少量 API
2. **使用 naiveproxy** - 完整 Chrome 栈，但维护成本高
3. **保持 rustls fork** - 现状，持续手动更新指纹

## 资源链接

- 本仓库: `/tmp/cronet-minimal-build/`
- naiveproxy 源码: `/tmp/naiveproxy/`
- v119 原始代码: `/tmp/cronet-binaries/`
- hray-core: `~/桌面/hray-core/`
- rustls fork: `~/桌面/rustls/`

## 联系方式

项目负责人: (待填写)
技术讨论: (待填写)
