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

## 2026-02-27 更新：Nomos Simplified Correct Implementation

### 实现目标
创建密码学正确但简化的 Nomos 实现，用于博士论文实验数据生成。

### 实现状态：~95% 完成

#### ✅ 已完成组件

1. **GatekeeperCorrect** (`include/nomos/GatekeeperCorrect.hpp`, `src/nomos/GatekeeperCorrect.cpp`)
   - 完整实现算法 2（Update）
   - 简化版 token 生成（genTokenSimplified）
   - 正确的密钥数组管理：Kt[1..d], Kx[1..d]
   - 椭圆曲线操作：地址计算、alpha 计算、xtag 计算
   - ~550 行代码

2. **ClientCorrect** (`include/nomos/ClientCorrect.hpp`, `src/nomos/ClientCorrect.cpp`)
   - 简化版 token 生成（委托给 Gatekeeper）
   - 搜索准备（prepareSearch）- 算法 4
   - 结果解密（decryptResults）
   - ~150 行代码

3. **ServerCorrect** (`include/nomos/ServerCorrect.hpp`, `src/nomos/ServerCorrect.cpp`)
   - TSet/XSet 存储（update）
   - 搜索执行（search）- 算法 4
   - 交叉过滤验证
   - ~120 行代码

4. **NomosSimplifiedExperiment** (`include/nomos/NomosSimplifiedExperiment.hpp`, `src/nomos/NomosSimplifiedExperiment.cpp`)
   - 集成测试框架
   - 5 个测试用例
   - ~100 行代码

5. **types_correct.hpp** (`include/nomos/types_correct.hpp`)
   - 正确的数据结构定义
   - 使用序列化字符串存储椭圆曲线点

#### ⚠️ 待解决问题

1. **编译错误**：main.cpp 存在小的链接或包含问题（最后一步）
2. **未测试**：代码尚未运行验证

### 关键技术决策

#### 1. 简化策略
- **不使用交互式 OPRF 盲化**：Gatekeeper 直接计算所有 token
- **保持密码学正确性**：所有椭圆曲线操作与论文一致
- **可升级性**：预留了完整 OPRF 协议的接口（已注释）

#### 2. RELIC 数组类型处理
**问题**：`bn_t` 和 `ep_t` 是数组类型（`bn_st[1]`, `ep_st[1]`），无法直接用于 `std::vector`

**解决方案**：
```cpp
// 方案 1：序列化存储（用于需要存储的数据）
std::vector<std::string> bstag;  // 存储序列化后的椭圆曲线点

// 方案 2：手动内存管理（用于固定大小数组）
bn_t* m_Kt = new bn_t[d];  // 密钥数组
for (int i = 0; i < d; ++i) {
    bn_new(m_Kt[i]);
    bn_rand_mod(m_Kt[i], ord);
}

// 方案 3：临时数组（用于局部变量）
bn_t* e = new bn_t[m];
// ... 使用 ...
for (int j = 0; j < m; ++j) bn_free(e[j]);
delete[] e;
```

#### 3. 序列化辅助函数
```cpp
// 椭圆曲线点序列化
static std::string serializePoint(const ep_t point) {
    uint8_t bytes[256];
    int len = ep_size_bin(point, 1);
    ep_write_bin(bytes, len, point, 1);
    return std::string(reinterpret_cast<char*>(bytes), len);
}

// 椭圆曲线点反序列化
static void deserializePoint(ep_t point, const std::string& str) {
    ep_read_bin(point, reinterpret_cast<const uint8_t*>(str.data()), str.length());
}
```

### 密码学正确性验证

#### ✅ 保持的论文算法
- 椭圆曲线点：`addr = H(w||cnt||0)^Kt[I(w)]`
- 密钥数组：`Kt[1..d]`, `Kx[1..d]`
- Alpha 计算：`alpha = Fp(Ky, id||op) · (Fp(Kz, w||cnt))^-1`
- Xtag 计算：`xtag_i = H(w)^{Kx[I(w)] · Fp(Ky, id||op) · i}`
- Update 协议（算法 2）
- Search 协议（算法 4）

#### ⚠️ 简化的部分
- OPRF 盲化：Gatekeeper 直接计算而非交互式协议
- Token 隐私：Client 接收预计算的 token

### 测试计划

#### 集成测试用例
```cpp
// Test 1: 添加文档
doc1 -> [crypto, security]
doc2 -> [security, privacy]
doc3 -> [crypto, blockchain]

// Test 2: 多关键词搜索
Query [crypto, security] → 期望: doc1

// Test 3: 多关键词搜索
Query [security, privacy] → 期望: doc2

// Test 4: 单关键词搜索
Query [crypto] → 期望: doc1, doc3

// Test 5: 不存在的关键词
Query [nonexistent] → 期望: 空结果
```

#### 运行命令
```bash
cd build
cmake --build .
./Nomos nomos-simplified
```

### 下一步工作

#### 立即任务（P0）
1. 修复 main.cpp 编译错误
2. 完成构建
3. 运行集成测试
4. 验证搜索结果正确性

#### 后续任务（P1）
1. 性能测量（Update/Search 时间）
2. 与错误实现对比
3. 生成论文实验数据

#### 可选升级（P2）
1. 实现完整 OPRF 协议
2. 添加单元测试
3. 性能优化

### 文件清单

#### 新增文件
1. `include/nomos/types_correct.hpp` - 正确的数据结构
2. `include/nomos/GatekeeperCorrect.hpp` - Gatekeeper 接口
3. `src/nomos/GatekeeperCorrect.cpp` - Gatekeeper 实现
4. `include/nomos/ClientCorrect.hpp` - Client 接口
5. `src/nomos/ClientCorrect.cpp` - Client 实现
6. `include/nomos/ServerCorrect.hpp` - Server 接口
7. `src/nomos/ServerCorrect.cpp` - Server 实现
8. `include/nomos/NomosSimplifiedExperiment.hpp` - 实验接口
9. `src/nomos/NomosSimplifiedExperiment.cpp` - 实验实现

#### 修改文件
1. `CMakeLists.txt` - 添加新源文件
2. `src/main.cpp` - 注册 "nomos-simplified" 实验

### 代码统计
- 新增代码：~1220 行
- GatekeeperCorrect: ~550 行
- ClientCorrect: ~150 行
- ServerCorrect: ~120 行
- NomosSimplifiedExperiment: ~100 行
- Headers: ~300 行

### 技术挑战与解决

#### 挑战 1：RELIC 数组类型
- **问题**：无法在 `std::vector` 中存储 `bn_t` 和 `ep_t`
- **解决**：序列化 + 手动内存管理

#### 挑战 2：内存管理
- **问题**：RELIC 类型需要显式 `bn_new()` 和 `bn_free()`
- **解决**：在构造/析构函数中统一管理

#### 挑战 3：编译错误调试
- **问题**：多次遇到模板实例化错误
- **解决**：逐步消除 `std::vector<bn_t>` 的使用

### 参考资料
- Nomos 论文：算法 2, 3, 4
- RELIC 文档：https://github.com/relic-toolkit/relic
- 原有（错误）实现：`src/nomos/Client.cpp`, `Server.cpp`, `Gatekeeper.cpp`

---

**最后更新**: 2026-02-27 (新增 Simplified Correct Implementation)
**作者**: Cyan
**项目路径**: `/Users/cyan/code/C++/Nomos`
