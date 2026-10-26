# 构建指南

## GitHub Actions 自动构建（推荐）

1. Fork 本仓库
2. Push 到 `main` 分支触发构建
3. 等待约 4-6 小时（Chromium 编译时间）
4. 下载 Artifacts 或从 Release 获取

## 本地构建

### 系统要求

- Ubuntu 22.04 / Debian 12
- 至少 100GB 磁盘空间
- 16GB+ RAM
- 构建时间：3-5 小时（取决于硬件）

**CI 优化**: GitHub Actions 使用浅克隆（--depth=1），磁盘占用 20-30GB，克隆 5-10 分钟。
本地构建需完整克隆（~100GB，30-60 分钟）。

### 步骤

```bash
# 1. 安装 depot_tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH="$PATH:$PWD/depot_tools"

# 2. 获取 Chromium 源码
mkdir chromium && cd chromium
fetch --nohooks chromium
cd src
git checkout 154.0.8037.49
gclient sync -D --force --reset

# 3. 安装依赖
build/install-build-deps.sh

# 4. 复制 Cronet Minimal 源码
mkdir -p components/cronet/native_minimal
cp -r /path/to/cronet-minimal-build/src/* components/cronet/native_minimal/
cp /path/to/cronet-minimal-build/BUILD.gn components/cronet/native_minimal/

# 5. 配置构建
gn gen out/Release --args='
  is_official_build=true
  is_debug=false
  symbol_level=0
  is_component_build=false
  use_sysroot=true
  target_cpu="x64"
  use_custom_libcxx=true
  enable_nacl=false
  enable_remoting=false
'

# 6. 编译
ninja -C out/Release components/cronet/native_minimal:cronet_minimal

# 7. 产物位置
# out/Release/libcronet_minimal.so
# components/cronet/native_minimal/include/
```

## 跨架构编译

修改 `target_cpu` 参数：
- `x64` - x86_64
- `arm64` - aarch64
- `arm` - armv7

## 故障排除

### 编译错误

如果遇到 API 不兼容错误，需要适配 v119 → v154 的变化：

1. 检查错误信息中的头文件路径
2. 对比 v119 和 v154 的 API 差异
3. 修改 `src/` 下的源文件

常见问题：
- `base::BindOnce` 参数变化
- `net::URLRequest` 回调签名变化
- 移除的 API（如 grpc_support）

### 依赖缺失

确保运行了 `build/install-build-deps.sh`。

如果是 ARM 交叉编译，需要额外的 sysroot：
```bash
build/linux/sysroot_scripts/install-sysroot.py --arch=arm64
```

## 验证构建

```bash
# 检查符号
nm -D out/Release/libcronet_minimal.so | grep Cronet

# 检查依赖
ldd out/Release/libcronet_minimal.so

# 测试加载
LD_LIBRARY_PATH=out/Release ./test_executable
```
