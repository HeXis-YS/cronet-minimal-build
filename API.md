# Cronet Minimal API 参考

本文档列出了提取的 Cronet C API 子集。

## 设计原则

- **最小充分**: 只包含 XHTTP 所需的核心功能
- **向后兼容**: API 应该跨 Chromium 版本相对稳定
- **易绑定**: 适合生成 Rust/Go/其他语言的 bindings

## API 概览

### 1. Engine (引擎管理)

引擎是 Cronet 的全局实例，管理网络栈生命周期。

```c
// 创建引擎
Cronet_EnginePtr Cronet_Engine_Create();

// 启动引擎
Cronet_RESULT Cronet_Engine_StartWithParams(
    Cronet_EnginePtr engine,
    Cronet_EngineParamsPtr params);

// 关闭引擎
void Cronet_Engine_Shutdown(Cronet_EnginePtr engine);

// 销毁引擎
void Cronet_Engine_Destroy(Cronet_EnginePtr engine);
```

**使用模式**:
```c
Cronet_EnginePtr engine = Cronet_Engine_Create();
Cronet_EngineParamsPtr params = Cronet_EngineParams_Create();
Cronet_EngineParams_user_agent_set(params, "MyApp/1.0");
Cronet_Engine_StartWithParams(engine, params);
Cronet_EngineParams_Destroy(params);

// ... 使用引擎 ...

Cronet_Engine_Shutdown(engine);
Cronet_Engine_Destroy(engine);
```

### 2. EngineParams (引擎配置)

配置引擎行为，包括 User-Agent、HTTP/2、缓存等。

```c
// 创建参数对象
Cronet_EngineParamsPtr Cronet_EngineParams_Create();
void Cronet_EngineParams_Destroy(Cronet_EngineParamsPtr params);

// User-Agent
void Cronet_EngineParams_user_agent_set(
    Cronet_EngineParamsPtr params,
    Cronet_String user_agent);

// HTTP/2 支持
void Cronet_EngineParams_enable_http2_set(
    Cronet_EngineParamsPtr params,
    bool enable);

// QUIC 支持 (H3)
void Cronet_EngineParams_enable_quic_set(
    Cronet_EngineParamsPtr params,
    bool enable);

// 公钥固定 (Certificate Pinning)
void Cronet_EngineParams_enable_public_key_pinning_bypass_for_local_trust_anchors_set(
    Cronet_EngineParamsPtr params,
    bool enable);

// 网络线程优先级
void Cronet_EngineParams_network_thread_priority_set(
    Cronet_EngineParamsPtr params,
    double priority);
```

### 3. UrlRequest (HTTP 请求)

发起 HTTP/HTTPS 请求的核心 API。

```c
// 创建请求
Cronet_UrlRequestPtr Cronet_UrlRequest_Create();
void Cronet_UrlRequest_Destroy(Cronet_UrlRequestPtr request);

// 初始化请求
Cronet_RESULT Cronet_UrlRequest_InitWithParams(
    Cronet_UrlRequestPtr request,
    Cronet_EnginePtr engine,
    Cronet_String url,
    Cronet_UrlRequestParamsPtr params,
    Cronet_UrlRequestCallbackPtr callback);

// 启动请求
Cronet_RESULT Cronet_UrlRequest_Start(Cronet_UrlRequestPtr request);

// 取消请求
void Cronet_UrlRequest_Cancel(Cronet_UrlRequestPtr request);

// 读取响应体
void Cronet_UrlRequest_Read(
    Cronet_UrlRequestPtr request,
    Cronet_BufferPtr buffer);

// 跟随重定向
void Cronet_UrlRequest_FollowRedirect(Cronet_UrlRequestPtr request);
```

### 4. UrlRequestParams (请求配置)

配置单个请求的参数。

```c
// 创建参数
Cronet_UrlRequestParamsPtr Cronet_UrlRequestParams_Create();
void Cronet_UrlRequestParams_Destroy(Cronet_UrlRequestParamsPtr params);

// HTTP 方法
void Cronet_UrlRequestParams_http_method_set(
    Cronet_UrlRequestParamsPtr params,
    Cronet_String method);

// 请求头
void Cronet_UrlRequestParams_request_headers_add(
    Cronet_UrlRequestParamsPtr params,
    Cronet_HttpHeaderPtr header);

// 禁用缓存
void Cronet_UrlRequestParams_disable_cache_set(
    Cronet_UrlRequestParamsPtr params,
    bool disable);

// 优先级
void Cronet_UrlRequestParams_priority_set(
    Cronet_UrlRequestParamsPtr params,
    Cronet_UrlRequestParams_REQUEST_PRIORITY priority);

// 上传数据
void Cronet_UrlRequestParams_upload_data_provider_set(
    Cronet_UrlRequestParamsPtr params,
    Cronet_UploadDataProviderPtr provider);

// 请求注释 (用于日志)
void Cronet_UrlRequestParams_request_finished_listener_set(
    Cronet_UrlRequestParamsPtr params,
    Cronet_RequestFinishedInfoListenerPtr listener);
```

### 5. UrlRequestCallback (回调接口)

处理请求生命周期的回调。

```c
// 创建回调
Cronet_UrlRequestCallbackPtr Cronet_UrlRequestCallback_CreateWith(
    Cronet_UrlRequestCallback_OnRedirectReceivedFunc OnRedirectReceivedFunc,
    Cronet_UrlRequestCallback_OnResponseStartedFunc OnResponseStartedFunc,
    Cronet_UrlRequestCallback_OnReadCompletedFunc OnReadCompletedFunc,
    Cronet_UrlRequestCallback_OnSucceededFunc OnSucceededFunc,
    Cronet_UrlRequestCallback_OnFailedFunc OnFailedFunc,
    Cronet_UrlRequestCallback_OnCanceledFunc OnCanceledFunc);

void Cronet_UrlRequestCallback_Destroy(Cronet_UrlRequestCallbackPtr callback);
```

**回调函数签名**:
```c
// 收到重定向
void OnRedirectReceived(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info,
    Cronet_String newLocationUrl);

// 响应开始
void OnResponseStarted(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info);

// 读取完成
void OnReadCompleted(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info,
    Cronet_BufferPtr buffer,
    uint64_t bytes_read);

// 请求成功
void OnSucceeded(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info);

// 请求失败
void OnFailed(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info,
    Cronet_ErrorPtr error);

// 请求取消
void OnCanceled(
    Cronet_UrlRequestCallbackPtr self,
    Cronet_UrlRequestPtr request,
    Cronet_UrlResponseInfoPtr info);
```

### 6. UrlResponseInfo (响应信息)

包含 HTTP 响应的元数据。

```c
// 获取 HTTP 状态码
int32_t Cronet_UrlResponseInfo_http_status_code_get(
    Cronet_UrlResponseInfoPtr info);

// 获取 HTTP 状态文本
Cronet_String Cronet_UrlResponseInfo_http_status_text_get(
    Cronet_UrlResponseInfoPtr info);

// 获取响应头
Cronet_String Cronet_UrlResponseInfo_all_headers_list_at(
    Cronet_UrlResponseInfoPtr info,
    uint32_t index);

// 获取协议协商结果
Cronet_String Cronet_UrlResponseInfo_negotiated_protocol_get(
    Cronet_UrlResponseInfoPtr info);

// 获取接收字节数
int64_t Cronet_UrlResponseInfo_received_byte_count_get(
    Cronet_UrlResponseInfoPtr info);
```

### 7. Buffer (内存管理)

管理请求和响应的缓冲区。

```c
// 创建缓冲区
Cronet_BufferPtr Cronet_Buffer_Create();
void Cronet_Buffer_Destroy(Cronet_BufferPtr buffer);

// 初始化缓冲区
void Cronet_Buffer_InitWithAlloc(
    Cronet_BufferPtr buffer,
    uint64_t size);

// 获取数据指针
Cronet_RawDataPtr Cronet_Buffer_GetData(Cronet_BufferPtr buffer);

// 获取缓冲区大小
uint64_t Cronet_Buffer_GetSize(Cronet_BufferPtr buffer);
```

### 8. UploadDataProvider (上传数据)

用于 POST/PUT 请求的数据提供者。

```c
// 创建上传数据提供者
Cronet_UploadDataProviderPtr Cronet_UploadDataProvider_CreateWith(
    Cronet_UploadDataProvider_GetLengthFunc GetLengthFunc,
    Cronet_UploadDataProvider_ReadFunc ReadFunc,
    Cronet_UploadDataProvider_RewindFunc RewindFunc,
    Cronet_UploadDataProvider_CloseFunc CloseFunc);

void Cronet_UploadDataProvider_Destroy(Cronet_UploadDataProviderPtr provider);
```

**回调函数签名**:
```c
// 获取数据长度 (-1 表示 chunked)
int64_t GetLength(Cronet_UploadDataProviderPtr self);

// 读取数据
void Read(
    Cronet_UploadDataProviderPtr self,
    Cronet_UploadDataSinkPtr sink,
    Cronet_BufferPtr buffer);

// 重置读取位置 (用于重试)
void Rewind(
    Cronet_UploadDataProviderPtr self,
    Cronet_UploadDataSinkPtr sink);

// 关闭
void Close(Cronet_UploadDataProviderPtr self);
```

### 9. Error (错误信息)

描述请求失败的原因。

```c
// 获取错误码
Cronet_Error_ERROR_CODE Cronet_Error_error_code_get(Cronet_ErrorPtr error);

// 获取错误消息
Cronet_String Cronet_Error_message_get(Cronet_ErrorPtr error);

// 获取内部错误码
int32_t Cronet_Error_internal_error_code_get(Cronet_ErrorPtr error);
```

## 典型使用流程

### 简单 GET 请求

```c
// 1. 创建并启动引擎
Cronet_EnginePtr engine = Cronet_Engine_Create();
Cronet_EngineParamsPtr params = Cronet_EngineParams_Create();
Cronet_EngineParams_user_agent_set(params, "MyApp/1.0");
Cronet_Engine_StartWithParams(engine, params);
Cronet_EngineParams_Destroy(params);

// 2. 创建请求参数
Cronet_UrlRequestParamsPtr req_params = Cronet_UrlRequestParams_Create();

// 3. 创建回调
Cronet_UrlRequestCallbackPtr callback = Cronet_UrlRequestCallback_CreateWith(
    OnRedirectReceived,
    OnResponseStarted,
    OnReadCompleted,
    OnSucceeded,
    OnFailed,
    OnCanceled);

// 4. 创建并初始化请求
Cronet_UrlRequestPtr request = Cronet_UrlRequest_Create();
Cronet_UrlRequest_InitWithParams(
    request,
    engine,
    "https://example.com",
    req_params,
    callback);

// 5. 启动请求
Cronet_UrlRequest_Start(request);

// 6. 在回调中处理响应
// OnResponseStarted -> 分配 buffer -> Cronet_UrlRequest_Read
// OnReadCompleted -> 处理数据 -> 继续 Read 或完成
// OnSucceeded -> 清理资源

// 7. 清理
Cronet_UrlRequest_Destroy(request);
Cronet_UrlRequestParams_Destroy(req_params);
Cronet_UrlRequestCallback_Destroy(callback);
Cronet_Engine_Shutdown(engine);
Cronet_Engine_Destroy(engine);
```

### POST 请求

```c
// 创建上传数据提供者
Cronet_UploadDataProviderPtr provider = Cronet_UploadDataProvider_CreateWith(
    MyGetLength,
    MyRead,
    MyRewind,
    MyClose);

// 设置请求方法和上传数据
Cronet_UrlRequestParamsPtr params = Cronet_UrlRequestParams_Create();
Cronet_UrlRequestParams_http_method_set(params, "POST");
Cronet_UrlRequestParams_upload_data_provider_set(params, provider);

// 添加 Content-Type 头
Cronet_HttpHeaderPtr header = Cronet_HttpHeader_Create();
Cronet_HttpHeader_name_set(header, "Content-Type");
Cronet_HttpHeader_value_set(header, "application/octet-stream");
Cronet_UrlRequestParams_request_headers_add(params, header);

// ... 其余步骤同 GET 请求
```

## 不包含的 API

为了最小化代码和维护成本，以下功能**不包含**在此子集中：

- ❌ **Cookie 管理**: `CookieManager` 相关 API
- ❌ **HTTP 缓存**: Cache 控制 API
- ❌ **HTTP Auth**: 认证相关 API
- ❌ **代理配置**: Proxy 规则 API
- ❌ **BidirectionalStream**: gRPC 支持（v154 已删除）
- ❌ **详细指标**: 完整的 `RequestFinishedInfo` 和 `Metrics`
- ❌ **测试工具**: Mock 和测试辅助 API

如果 hray-core 确实需要这些功能，可以：
1. 评估是否能在应用层实现（如自行管理 Cookie）
2. 扩展此 API 子集（需重新评估维护成本）
3. 使用完整的 Cronet 或 naiveproxy

## API 稳定性

### 核心 API（高稳定性）

- `Cronet_Engine_*` - 引擎管理
- `Cronet_UrlRequest_*` - 请求管理
- `Cronet_Buffer_*` - 缓冲区

这些 API 在 Chromium 中相对稳定，破坏性变更较少。

### 参数 API（中等稳定性）

- `Cronet_EngineParams_*` - 引擎配置
- `Cronet_UrlRequestParams_*` - 请求配置

可能会增加新参数，但已有参数通常保持兼容。

### 回调 API（低稳定性）

- `Cronet_UrlRequestCallback_*` - 回调接口
- `Cronet_UploadDataProvider_*` - 上传接口

回调签名变化较少，但内部实现可能调整。

## 版本兼容性

| Chromium 版本 | API 兼容性 | 备注 |
|---------------|-----------|------|
| 119 | ✅ 完全兼容 | 提取来源 |
| 120-153 | ⚠️ 未测试 | 预计兼容 |
| 154 | 🔄 需适配 | 移除 grpc_support |
| 155+ | ❓ 待评估 | 季度跟踪 |

## 扩展 API

如果需要添加新 API：

1. **从 v154 源码提取** - 确保与目标版本一致
2. **最小提取** - 只添加必需的函数和类型
3. **更新 BUILD.gn** - 添加新文件到构建
4. **更新文档** - 在本文档记录新 API
5. **验证编译** - 确保新 API 可以链接
6. **评估维护成本** - 新 API 是否会增加跟踪负担

## 参考资源

- **完整 API 文档**: https://chromium.googlesource.com/chromium/src/+/refs/heads/main/components/cronet/
- **IDL 定义**: `generated/cronet.idl_c.h`
- **实现**: `src/engine.cc`, `src/url_request.cc`
- **使用示例**: INTEGRATION.md 中的 Rust bindings

## 反馈

如果发现 API 缺失或不足，请：
1. 描述具体使用场景
2. 说明为什么现有 API 无法满足
3. 评估添加新 API 的维护成本
4. 在 GitHub Issues 中讨论
