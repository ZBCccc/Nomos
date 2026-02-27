# Nomos Implementation Status Report

## 项目概述

本项目实现了三个可搜索加密方案用于论文实验评估：
1. **Nomos** - 基线方案（多用户多关键词动态SSE）
2. **Verifiable Nomos** - 可验证方案（QTree + 嵌入式承诺）
3. **MC-ODXT** - 多客户端ODXT改造（待实现）

## 已完成功能

### 1. Nomos 基线方案 ✅

**核心组件**：
- `Gatekeeper`: 密钥管理与令牌生成
- `Server`: 加密索引存储与检索
- `Client`: 查询发起与结果解密

**已实现算法**：
- ✅ `Setup()`: 系统初始化，生成主密钥 Ks, Ky, Km
- ✅ `Update(op, id, keyword)`: 动态更新协议
  - 计算 TSet 地址与加密载荷
  - 生成 ℓ=3 个交叉标签 xtag
  - 维护更新计数器 UpdateCnt
- ✅ `GenToken(query_keywords)`: 令牌生成
  - 选择主关键词 ws（最低更新频次）
  - 生成 stokens 用于候选枚举
  - 生成 xtokens 用于交叉过滤（k=2 采样）
- ✅ `Search(trapdoor)`: 服务器端检索
  - 候选枚举（基于 TSet）
  - 交叉过滤（基于 XSet）

**参数设置**：
- ℓ = 3 (插入侧哈希展开次数)
- k = 2 (查询侧采样次数)
- 安全参数 λ = 256 bits

**测试状态**：
- ✅ 编译通过
- ✅ 基本流程运行
- ⚠️ 搜索结果为空（密码学操作需精修）

### 2. 可验证方案 (QTree + Commitment) ✅

**核心组件**：
- `QTree`: Merkle Hash Tree 实现
- `AddressCommitment`: 地址承诺机制
- `VerifiableExperiment`: 集成测试

**QTree 功能**：
- ✅ `initialize()`: 初始化 MHT，容量 1024
- ✅ `updateBit()`: 单比特更新，自动维护路径
- ✅ `generateProof()`: 生成认证路径
- ✅ `generatePositiveProof()`: k 条路径（正判定）
- ✅ `generateNegativeProof()`: 1 条路径（负判定）
- ✅ `getRootHash()`: 获取根承诺 R_X^(t)
- ✅ `verifyPath()`: 静态验证函数

**AddressCommitment 功能**：
- ✅ `commit()`: 计算 Cm_{w,id} = H_c(xtag_1||...||xtag_ℓ)
- ✅ `verify()`: 验证承诺开封
- ✅ `checkSubsetMembership()`: 子集归属检查

**测试结果**：
- ✅ QTree 初始化成功
- ✅ 比特更新与版本递增正常
- ✅ 承诺生成与验证通过
- ✅ 子集归属检查通过
- ⚠️ 路径验证失败（索引计算需修正）

### 3. 密码学原语 ✅

**已实现哈希函数** (`core/Primitive.cpp`):
- `Hash_H1()`: SHA256 → 椭圆曲线点
- `Hash_H2()`: SHA384 → 椭圆曲线点
- `Hash_G1()`: SHA512 → 椭圆曲线点
- `Hash_G2()`: SHA224/SHA384 → 椭圆曲线点
- `Hash_Zn()`: SHA512 → 模 p 大整数

**依赖库**：
- RELIC: 椭圆曲线运算
- OpenSSL: 哈希与随机数
- GMP: 大整数运算

## 待完成功能

### 1. MC-ODXT 多客户端改造 ⏳

**需要实现**：
- [ ] ODXT 基础协议（单客户端）
- [ ] 多客户端密钥管理
- [ ] 访问控制策略
- [ ] 令牌派生协议

**参考文献**：
- Cash et al., "Highly-Scalable Searchable Symmetric Encryption with Support for Boolean Queries" (CRYPTO 2013)

### 2. 基准测试框架 ⏳

**需要实现**：
- [ ] 时间测量（Setup/Update/Search/Verify）
- [ ] 参数扫描（N, M, ℓ, k, n）
- [ ] 对比实验（Nomos vs Verifiable vs MC-ODXT）
- [ ] 数据导出（CSV/JSON）
- [ ] 图表生成（Python matplotlib）

**目标指标**（对应论文第4章）：
- 更新开销：O(ℓ log M) vs O(ℓ)
- 搜索开销：O((N_+ · k + N_-) log M)
- 通信开销：证明大小
- 验证开销：客户端计算量

### 3. 密码学操作精修 ⏳

**当前问题**：
- Nomos 搜索返回空结果
- QTree 路径验证失败

**需要修复**：
- [ ] TSet 地址计算与查找逻辑
- [ ] XSet 成员测试（xtag 匹配）
- [ ] 载荷加密/解密（XOR mask）
- [ ] QTree 索引映射（地址 → 叶节点）

## 代码结构

```
Nomos/
├── include/
│   ├── core/
│   │   ├── Experiment.hpp          # 实验基类
│   │   ├── ExperimentFactory.hpp   # 工厂模式
│   │   ├── Primitive.hpp           # 密码学原语
│   │   └── CpABE.hpp              # CP-ABE（未使用）
│   ├── nomos/
│   │   ├── Gatekeeper.hpp         # 授权管理方
│   │   ├── Server.hpp             # 云服务器
│   │   ├── Client.hpp             # 授权用户
│   │   ├── types.hpp              # 数据结构
│   │   ├── GatekeeperState.hpp   # 状态管理
│   │   └── NomosExperiment.hpp   # Nomos 实验
│   ├── verifiable/
│   │   ├── QTree.hpp              # Merkle Hash Tree
│   │   ├── AddressCommitment.hpp  # 地址承诺
│   │   ├── VerifiableScheme.hpp   # 可验证方案（待完善）
│   │   └── VerifiableExperiment.hpp # 可验证实验
│   └── mc-odxt/
│       └── McOdxtExperiment.hpp   # MC-ODXT 实验（待实现）
├── src/
│   ├── core/
│   │   ├── Primitive.cpp
│   │   └── CpABE.cpp
│   ├── nomos/
│   │   ├── Gatekeeper.cpp         # 核心协议实现
│   │   ├── Server.cpp
│   │   ├── Client.cpp
│   │   └── GatekeeperState.cpp
│   ├── verifiable/
│   │   ├── QTree.cpp              # MHT 实现
│   │   └── AddressCommitment.cpp  # 承诺实现
│   ├── mc-odxt/
│   │   └── McOdxtExperiment.cpp   # 空实现
│   └── main.cpp                   # 主程序入口
├── tests/
│   ├── main_test.cpp
│   ├── cp_abe_test.cpp
│   └── relic_test.cpp
├── CMakeLists.txt
└── README.md
```

## 编译与运行

### 编译
```bash
cd build
cmake ..
make
```

### 运行实验
```bash
# Nomos 基线方案
./Nomos nomos

# 可验证方案
./Nomos verifiable

# MC-ODXT（待实现）
./Nomos mc-odxt
```

## 下一步工作

### 优先级 P0（本周完成）
1. 修复 Nomos 搜索逻辑（TSet/XSet 匹配）
2. 修复 QTree 路径验证（索引映射）
3. 实现基准测试框架骨架

### 优先级 P1（下周完成）
1. 实现 MC-ODXT 基础协议
2. 完成参数扫描实验
3. 生成论文第4章所需数据

### 优先级 P2（后续优化）
1. 性能优化（批量更新、缓存）
2. 内存优化（大规模数据集）
3. 并行化（多线程搜索）

## 论文对应关系

| 论文章节 | 代码模块 | 实现状态 |
|---------|---------|---------|
| 第2章 bf.tex | `nomos/` + `verifiable/QTree` | ✅ 80% |
| 第3章 commitment.tex | `verifiable/AddressCommitment` | ✅ 90% |
| 第4章 experiments.tex | 基准测试框架 | ⏳ 20% |
| 对比方案 MC-ODXT | `mc-odxt/` | ⏳ 0% |

## 技术债务

1. **密码学正确性**：当前实现为原型，PRF/OPRF 使用简化哈希，需替换为标准实现
2. **错误处理**：缺少异常处理与边界检查
3. **内存管理**：RELIC 大数未完全释放，存在潜在泄漏
4. **测试覆盖**：单元测试不足，仅有集成测试
5. **文档**：代码注释不完整，缺少 API 文档

## 参考文献

1. Bag et al., "Nomos: Scalable Multi-Client Searchable Encryption" (2024)
2. Cash et al., "Highly-Scalable Searchable Symmetric Encryption" (CRYPTO 2013)
3. Merkle, "A Digital Signature Based on a Conventional Encryption Function" (CRYPTO 1987)

---

**最后更新**: 2026-02-27
**作者**: Cyan
**项目路径**: `/Users/cyan/code/C++/Nomos`
