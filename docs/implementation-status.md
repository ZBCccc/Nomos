# Nomos Implementation Status

## 项目概述

本项目当前围绕三类方案展开：

1. **Nomos**
   - 多用户、多关键词、动态 SSE 主线实现
   - 当前仅保留简化版 `genToken` 实验路径
2. **Verifiable Nomos**
   - 可验证扩展组件
   - 当前已实现 `QTree + AddressCommitment`
   - 尚未完整接入 Nomos 主链路
3. **MC-ODXT**
   - 多客户端 ODXT 对比方案原型
   - 已可运行并已接入 Chapter 4 实验

---

## 当前实现状态

| 模块 | 当前状态 | 说明 |
|------|----------|------|
| Nomos 主线 | ✅ 可运行 | `Gatekeeper / Client / Server` 已统一为默认实现 |
| Simplified `genToken` | ✅ 可运行 | 保留 Hash/PRF 推导，直接生成搜索令牌 |
| Verifiable 组件 | ⚠️ 部分完成 | `QTree` 与 `AddressCommitment` 可独立运行，协议级接线未完成 |
| MC-ODXT 原型 | ✅ 可运行 | 已接入实验，但实现仍带原型化简化 |
| Chapter 4 基准 | ✅ 运行中 | 已有 `chapter4-client-search-fixed-w1` 数据驱动实验入口 |

---

## Nomos

### 当前主线代码

- `include/nomos/Gatekeeper.hpp`
- `include/nomos/Client.hpp`
- `include/nomos/Server.hpp`
- `include/nomos/types.hpp`
- `src/nomos/Gatekeeper.cpp`
- `src/nomos/Client.cpp`
- `src/nomos/Server.cpp`

### 当前搜索路径

当前只保留一条简化搜索路径：

1. `Client::genTokenSimplified(...)`
2. `Gatekeeper::genTokenSimplified(...)`
3. `Client::prepareSearch(...)`
4. `Server::search(...)`
5. `Client::decryptResults(...)`

这条路径的定位是：

- 保留椭圆曲线与 `F / F_p` 推导
- 不引入额外 OPRF 盲化开销
- 适合论文实验、局部调试和轻量演示

### 当前结论

Nomos 已不是“Correct 版本覆盖旧版本”的双轨结构。  
现在的 `Gatekeeper / Client / Server / types` 本身就是主线实现。

---

## Verifiable Nomos

### 当前已实现部分

- `include/verifiable/QTree.hpp`
- `include/verifiable/AddressCommitment.hpp`
- `src/verifiable/QTree.cpp`
- `src/verifiable/AddressCommitment.cpp`
- `include/verifiable/VerifiableExperiment.hpp`

### 当前状态

- `QTree` 可独立更新、生成路径、做验证
- `AddressCommitment` 可独立承诺、开封验证、做子集归属检查
- `verifiable` 实验入口当前是组件级测试，不是完整的协议级 `VQNomos`

### 当前未完成部分

- `VerifiableScheme.hpp` 仍是未接线接口草稿
- 当前还没有 `VerifiableScheme.cpp`
- 也没有完整打通 `Nomos Update/Search -> Verifiable Update/Search`

### 与 Chapter 4 的关系

当前 Chapter 4 中的 `VQNomos` 仍然是：

- 先真实测 `Nomos`
- 再叠加可验证开销估计

也就是说，Chapter 4 的 `VQNomos` 还不是完整协议实测。

---

## MC-ODXT

### 当前主线状态

`MC-ODXT` 已不是“待实现”空壳，而是一套可运行原型。

当前参与运行的主要角色有：

- `McOdxtGatekeeper`
- `McOdxtDataOwner`
- `McOdxtClient`
- `McOdxtServer`

### 当前生效源码路径

这里有一个重要工程事实：

- 当前 CMake 编译的是 `include/mc-odxt/McOdxtProtocol.cpp`
- `src/mc-odxt/McOdxtProtocol.cpp` 也存在，但不是当前生效主线

因此后续重构必须先统一 `MC-ODXT` 的源码位置。

### 当前实现特征

- 已支持 owner / search user / gatekeeper / server 四方原型
- 已有授权与 per-owner 状态
- 已接入 Chapter 4 实验
- 仍保留若干原型化简化，不应直接视为论文最终实现版本

---

## 密码学原语

当前统一原语入口位于：

- `include/core/Primitive.hpp`
- `src/core/Primitive.cpp`

### 命名约定

- `Hash_*`：无密钥哈希 / hash-to-curve / hash-to-field
- `F`：普通字符串型 PRF
- `F_p`：有密钥标量派生
- `OPRF`：历史协议路径，当前实验代码不再使用

### 当前已实现

- `Hash_H1()`: SHA256 → 曲线点
- `Hash_H2()`: SHA384 → 曲线点
- `Hash_G1()`: SHA512 → 曲线点
- `Hash_G2()`: SHA224/SHA384 → 曲线点
- `Hash_Zn()`: SHA512 → 标量域
- `F()`: `HMAC-SHA256`
- `F_p()`: `HMAC-SHA256 -> Hash_Zn`

---

## 构建与运行

### 编译

```bash
cd /Users/cyan/code/paper/Nomos
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
```

### 运行

```bash
cd /Users/cyan/code/paper/Nomos/build

./Nomos nomos-simplified
./Nomos verifiable
./Nomos mc-odxt
./Nomos chapter4-client-search-fixed-w1
./tests/nomos_test
```

---

## 当前主要缺口

### P0

1. 统一 `MC-ODXT` 的有效源码位置
2. 完整打通 Verifiable 协议级接线
3. 继续清理历史文档与旧命名残留

### P1

1. 收敛 Chapter 4 各实验的计时边界
2. 统一实验输出格式与绘图管线
3. 将 `VQNomos` 从估算层升级为协议实测层

### P2

1. 完善访问控制检查
2. 做性能与内存优化
3. 继续收口实验与论文表述的一致性

---

## 技术债务

1. `MC-ODXT` 仍有 `include/` 与 `src/` 双轨实现并存
2. Verifiable 仍停留在组件级，而非协议级整合
3. `Nomos` 已明确收缩为实验用简化搜索路径
4. 一部分文档仍带有历史快照性质，需要持续收口

---

**最后更新**: 2026-03-13  
**项目路径**: `/Users/cyan/code/paper/Nomos`
