# Archived: Old Nomos Implementation

## 归档原因

这些文件是 Nomos 协议的早期实现版本，存在密码学正确性问题，已被 `*Correct.*` 版本替代。

## 归档时间

2026-02-28

## 主要问题

1. **密钥结构错误**：未使用密钥数组 Kt[1..d], Kx[1..d]
2. **Update 协议错误**：使用 SHA256 哈希而非椭圆曲线点
3. **缺少 OPRF 协议**：未实现盲化协议
4. **令牌结构错误**：简化的字符串列表而非椭圆曲线点
5. **搜索协议错误**：直接字符串匹配而非椭圆曲线运算

## 替代实现

正确的实现位于：
- `include/nomos/GatekeeperCorrect.hpp`
- `include/nomos/ServerCorrect.hpp`
- `include/nomos/ClientCorrect.hpp`
- `include/nomos/types_correct.hpp`
- `include/nomos/NomosSimplifiedExperiment.hpp`

## 归档文件列表

### Headers
- `Client.hpp` - 旧客户端接口
- `Server.hpp` - 旧服务器接口
- `Gatekeeper.hpp` - 旧授权管理方接口
- `types.hpp` - 旧数据结构定义
- `NomosExperiment.hpp` - 旧实验框架

### Implementation
- `Client.cpp` - 旧客户端实现
- `Server.cpp` - 旧服务器实现
- `Gatekeeper.cpp` - 旧授权管理方实现

## 参考

详见 `docs/implementation-status.md` 了解正确实现的详细信息。
