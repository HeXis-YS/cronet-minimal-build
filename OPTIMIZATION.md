# CI 构建优化

## 浅克隆策略

为了减少磁盘占用和加快构建速度，GitHub Actions CI 使用浅克隆（shallow clone）策略。

## 优化前 vs 优化后

| 指标 | 优化前 (fetch chromium) | 优化后 (shallow clone) | 改进 |
|------|------------------------|------------------------|------|
| 磁盘占用 | ~100GB | ~20-30GB | -70% |
| 克隆时间 | 30-60 分钟 | 5-10 分钟 | -83% |
| 总构建时间 | 4-6 小时 | 3-4 小时 | -25% |
| 网络传输 | ~15-20GB | ~3-5GB | -75% |

## 实现细节

### 优化前

```bash
# 完整克隆，包含所有历史
fetch --nohooks chromium
git checkout $VERSION
gclient sync -D --force --reset
```

**问题**:
- 下载完整 Git 历史（数万个 commit）
- 同步所有依赖的完整历史
- 占用大量磁盘空间
- 克隆时间长

### 优化后

```bash
# 配置 gclient
gclient root
gclient config --spec 'solutions = [
  {
    "name": "src",
    "url": "https://github.com/chromium/chromium.git",
    "custom_deps": {},
    "custom_vars": {},
  },
]'

# 浅克隆指定版本
git clone -b $VERSION --depth=1 --no-tags --single-branch \
  https://github.com/chromium/chromium.git src

# 浅同步依赖
gclient sync -n -p --no-history --shallow
```

**优势**:
- `--depth=1`: 只下载指定版本，不包含历史
- `--no-tags`: 不下载标签
- `--single-branch`: 只下载目标分支
- `--no-history`: 依赖也不下载历史
- `--shallow`: 所有依赖使用浅克隆

## 参数说明

### git clone 参数

- `-b $VERSION`: 检出指定版本/标签
- `--depth=1`: 只克隆最近一次提交
- `--no-tags`: 不获取任何标签
- `--single-branch`: 只克隆指定分支，不克隆其他分支

### gclient sync 参数

- `-n`: 不运行 hooks（后续手动运行）
- `-p`: 并行同步（加速）
- `--no-history`: 依赖使用浅克隆
- `--shallow`: 所有 Git 仓库使用 --depth=1

## 权衡

### 优点

✅ **磁盘占用大幅减少**: 70% 减少，适合 CI 环境
✅ **克隆速度快**: 83% 提速，减少等待时间
✅ **网络传输小**: 75% 减少，降低带宽成本
✅ **构建质量不变**: 最终产物完全一致

### 缺点

❌ **无法访问历史**: 不能 `git log` 或 `git blame`
❌ **无法切换分支**: 只有一个分支
❌ **无法 cherry-pick**: 没有历史记录

### 为什么适合 CI

对于 CI 场景，这些缺点不重要：
- CI 只需要构建当前版本
- 不需要调试或查看历史
- 构建完成后丢弃整个目录

对于本地开发，仍然推荐完整克隆。

## 本地构建

如果你需要本地构建，**不建议**使用浅克隆，因为：

1. 本地磁盘通常足够（100GB 对于开发机不算多）
2. 可能需要查看历史或切换版本
3. 完整克隆可以重用，避免重复下载

本地构建仍然使用标准流程：

```bash
fetch --nohooks chromium
cd src
git checkout $VERSION
gclient sync
```

## GitHub Actions 磁盘管理

GitHub Actions runners 有以下磁盘限制：

- **ubuntu-22.04**: 14GB 系统 + 70GB 工作空间 = 84GB 可用
- **ubuntu-24.04**: 类似配置

浅克隆前：
- Chromium 源码: ~100GB → **超出限制，可能失败**

浅克隆后：
- Chromium 源码: ~25GB
- 构建产物: ~15GB
- 系统工具: ~10GB
- 总计: ~50GB → **安全范围**

## 监控与验证

构建成功后，可以验证：

```bash
# 检查 Git 历史深度
cd src
git log --oneline | wc -l
# 应该输出: 1（只有一个 commit）

# 检查磁盘占用
du -sh src/
# 应该输出: ~20-30GB

# 检查构建产物
ls -lh out/Release/libcronet_minimal.so
# 应该正常生成
```

## 进一步优化

如果未来磁盘仍然不够，可以考虑：

1. **排除更多依赖**
   ```python
   "custom_deps": {
     "src/third_party/android_tools": None,
     "src/third_party/webrtc": None,
     # ... 更多不需要的依赖
   }
   ```

2. **使用 goma/ccache**
   - 分布式编译或缓存
   - 减少重复编译时间

3. **构建产物缓存**
   - 缓存 out/Release 的部分产物
   - 增量构建

4. **自托管 runner**
   - 使用更大磁盘的机器
   - 但增加维护成本

## 相关资源

- [Chromium: Get the Code](https://www.chromium.org/developers/how-tos/get-the-code/)
- [depot_tools documentation](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html)
- [Git shallow clone](https://git-scm.com/docs/git-clone#Documentation/git-clone.txt---depthltdepthgt)
- [GitHub Actions runners](https://docs.github.com/en/actions/using-github-hosted-runners/about-github-hosted-runners)

## 总结

浅克隆是 CI 构建的最佳实践：

- **大幅降低资源消耗**: 磁盘 -70%，时间 -25%
- **保持构建质量**: 产物完全一致
- **适合一次性构建**: CI 场景完美匹配

本地开发仍推荐完整克隆以获得更好的体验。
