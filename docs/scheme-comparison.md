# 方案对照与论文对应关系

## 文档说明

本文档说明 Nomos 项目中实现的各个方案与博士论文章节的对应关系，以及方案之间的对比维度。

---

## 一、论文章节与方案对应

### 论文结构（HUST-PhD-Thesis-Latex）

```
论文/
├── chapter1/ intro.tex          - 第1章：绪论
├── chapter2/ bf.tex             - 第2章：基础方案（Nomos 基线）
├── chapter3/ commitment.tex     - 第3章：可验证方案（Verifiable Nomos）
├── chapter4/ experiments.tex    - 第4章：复杂度分析与性能评估
└── chapter5/ conclusion.tex     - 第5章：总结与展望
```

### 方案与章节对应表

| 论文章节 | 方案名称 | 代码模块 | 实现状态 | 说明 |
|---------|---------|---------|---------|------|
| 第2章 bf.tex | **Nomos 基线方案** | `nomos/` | ✅ 原型完成<br>⚠️ 需修正 | 多用户多关键词动态 SSE |
| 第2章 bf.tex | **Nomos Simplified Correct** | `nomos/*Correct.*` | ✅ 95% 完成 | 密码学正确的简化版本 |
| 第3章 commitment.tex | **Verifiable Nomos** | `verifiable/` | ✅ 80% 完成 | QTree + 地址承诺 |
| 第4章 experiments.tex | **MC-ODXT** | `mc-odxt/` | ⏳ 待实现 | 对比基准方案 |
| 第4章 experiments.tex | **基准测试框架** | 待创建 | ⏳ 待实现 | 性能评估代码 |

---

## 二、方案详细对照

### 2.1 Nomos 基线方案（第2章）

#### 论文位置
- **文件**: `HUST-PhD-Thesis-Latex/body/chapter2/bf.tex`
- **章节**: 第2章 基础方案
- **算法**: 算法 2.1 (Setup), 算法 2.2 (Update), 算法 2.3 (GenToken), 算法 2.4 (Search)

#### 代码实现
```
include/nomos/
├── Gatekeeper.hpp          - 授权管理方（算法 2.1, 2.2, 2.3）
├── Server.hpp              - 云服务器（算法 2.4）
├── Client.hpp              - 授权用户（算法 2.3, 2.4）
├── types.hpp               - 数据结构定义
└── NomosExperiment.hpp     - 集成测试

src/nomos/
├── Gatekeeper.cpp          - 核心协议实现
├── Server.cpp              - 搜索执行
├── Client.cpp              - 令牌生成与结果解密
└── GatekeeperState.cpp     - 状态管理
```

#### 核心特性
- **多用户支持**: 通过 CP-ABE 实现访问控制
- **多关键词搜索**: 支持 n 个关键词的联合查询
- **动态更新**: 支持 ADD/DEL 操作
- **交叉过滤**: 使用 RBF (ℓ=3, k=2) 减少假阳性

#### 当前问题
- ⚠️ 搜索返回空结果（密码学操作需修正）
- ⚠️ 使用简化哈希代替标准 PRF/OPRF

---

### 2.2 Nomos Simplified Correct（第2章实验版本）

#### 论文位置
- **对应章节**: 第2章 基础方案
- **用途**: 生成第4章实验数据的正确实现

#### 代码实现
```
include/nomos/
├── types_correct.hpp              - 正确的数据结构
├── GatekeeperCorrect.hpp          - 正确的 Gatekeeper
├── ClientCorrect.hpp              - 正确的 Client
├── ServerCorrect.hpp              - 正确的 Server
└── NomosSimplifiedExperiment.hpp  - 简化实验

src/nomos/
├── GatekeeperCorrect.cpp          - 椭圆曲线操作实现
├── ClientCorrect.cpp              - 搜索准备
├── ServerCorrect.cpp              - TSet/XSet 存储与搜索
└── NomosSimplifiedExperiment.cpp  - 集成测试
```

#### 核心特性
- ✅ **密码学正确**: 使用椭圆曲线操作（非 SHA256 哈希）
- ✅ **密钥数组**: Kt[1..d], Kx[1..d]
- ✅ **正确的算法**: 严格按照论文算法 2.2, 2.4 实现
- ⚠️ **简化 OPRF**: Gatekeeper 直接计算 token（跳过盲化）

#### 与基线方案的区别
| 维度 | Nomos 基线 | Nomos Simplified Correct |
|------|-----------|-------------------------|
| 密码学正确性 | ⚠️ 使用简化哈希 | ✅ 使用椭圆曲线 |
| OPRF 协议 | ⚠️ 未实现 | ⚠️ 简化版（直接计算）|
| 密钥结构 | ⚠️ 单密钥 | ✅ 密钥数组 Kt[d], Kx[d] |
| 搜索正确性 | ⚠️ 返回空结果 | ✅ 预期正确 |
| 用途 | 原型验证 | 实验数据生成 |

---

### 2.3 Verifiable Nomos（第3章）

#### 论文位置
- **文件**: `HUST-PhD-Thesis-Latex/body/chapter3/commitment.tex`
- **章节**: 第3章 可验证方案
- **算法**: 算法 3.1 (VerifiableUpdate), 算法 3.2 (VerifiableSearch)

#### 代码实现
```
include/verifiable/
├── QTree.hpp                  - Merkle Hash Tree（算法 3.1）
├── AddressCommitment.hpp      - 地址承诺机制（算法 3.2）
├── VerifiableScheme.hpp       - 可验证方案（待完善）
└── VerifiableExperiment.hpp   - 集成测试

src/verifiable/
├── QTree.cpp                  - MHT 实现
├── AddressCommitment.cpp      - 承诺生成与验证
└── VerifiableExperiment.cpp   - 实验代码
```

#### 核心特性
- **QTree (Merkle Hash Tree)**:
  - 容量 M = 1024
  - 支持单比特更新
  - 生成认证路径（正判定 k 条，负判定 1 条）
  - 根承诺 R_X^(t)

- **AddressCommitment**:
  - 承诺计算: Cm_{w,id} = H_c(xtag_1||...||xtag_ℓ)
  - 承诺验证: 检查开封正确性
  - 子集归属检查: 验证 xtag 是否在承诺中

#### 与基线方案的区别
| 维度 | Nomos 基线 | Verifiable Nomos |
|------|-----------|-----------------|
| 可验证性 | ❌ 无 | ✅ 有（QTree + Commitment）|
| 更新开销 | O(ℓ) | O(ℓ log M) |
| 搜索开销 | O(N_+ · k + N_-) | O((N_+ · k + N_-) log M) |
| 通信开销 | 小 | 大（包含证明）|
| 客户端验证 | 无 | 需要验证路径 |

#### 当前问题
- ⚠️ QTree 路径验证失败（索引映射需修正）

---

### 2.4 MC-ODXT（第4章对比方案）

#### 论文位置
- **文件**: `HUST-PhD-Thesis-Latex/body/chapter4/experiments.tex`
- **章节**: 第4章 复杂度分析与性能评估
- **用途**: 作为对比基准

#### 原始方案
- **论文**: Cash et al., "Highly-Scalable Searchable Symmetric Encryption with Support for Boolean Queries" (CRYPTO 2013)
- **特点**: 单客户端、支持布尔查询、高度可扩展

#### 改造需求
- **多客户端支持**: 添加密钥管理和访问控制
- **与 Nomos 对比**: 相同场景下的性能对比

#### 代码实现（待完成）
```
include/mc-odxt/
├── OdxtScheme.hpp             - ODXT 基础协议
├── MultiClientOdxt.hpp        - 多客户端改造
└── McOdxtExperiment.hpp       - 实验框架

src/mc-odxt/
├── OdxtScheme.cpp             - 协议实现
├── MultiClientOdxt.cpp        - 多客户端逻辑
└── McOdxtExperiment.cpp       - 实验代码
```

#### 实现优先级
- **P1**: 实现 ODXT 基础协议
- **P2**: 添加多客户端支持
- **P3**: 集成到基准测试框架

---

## 三、方案对比维度

### 3.1 功能对比

| 功能 | Nomos 基线 | Verifiable Nomos | MC-ODXT |
|------|-----------|-----------------|---------|
| 多用户 | ✅ | ✅ | ⏳ 待改造 |
| 多关键词 | ✅ | ✅ | ✅ |
| 动态更新 | ✅ | ✅ | ✅ |
| 可验证性 | ❌ | ✅ | ❌ |
| 布尔查询 | ❌ | ❌ | ✅ |

### 3.2 性能对比（理论复杂度）

#### Update 操作
| 方案 | 计算复杂度 | 通信复杂度 | 存储复杂度 |
|------|-----------|-----------|-----------|
| Nomos | O(ℓ) | O(ℓ) | O(N · ℓ) |
| Verifiable Nomos | O(ℓ log M) | O(ℓ log M) | O(N · ℓ + M) |
| MC-ODXT | O(1) | O(1) | O(N) |

#### Search 操作
| 方案 | 计算复杂度 | 通信复杂度 | 客户端验证 |
|------|-----------|-----------|-----------|
| Nomos | O(N_+ · k + N_-) | O(N_+ · k) | - |
| Verifiable Nomos | O((N_+ · k + N_-) log M) | O((N_+ · k + N_-) log M) | O((N_+ · k + N_-) log M) |
| MC-ODXT | O(N_+) | O(N_+) | - |

**符号说明**:
- N: 总文档数
- N_+: 匹配文档数
- N_-: 不匹配文档数
- ℓ: 插入侧哈希展开次数（=3）
- k: 查询侧采样次数（=2）
- M: QTree 容量（=1024）

### 3.3 安全性对比

| 安全属性 | Nomos 基线 | Verifiable Nomos | MC-ODXT |
|---------|-----------|-----------------|---------|
| 搜索模式隐藏 | ✅ | ✅ | ✅ |
| 访问模式隐藏 | ⚠️ 部分 | ⚠️ 部分 | ⚠️ 部分 |
| 结果完整性 | ❌ | ✅ | ❌ |
| 结果正确性 | ❌ | ✅ | ❌ |
| 前向安全 | ✅ | ✅ | ✅ |
| 后向安全 | ⚠️ 弱 | ⚠️ 弱 | ⚠️ 弱 |

---

## 四、实验评估计划（第4章）

### 4.1 实验参数

#### 固定参数
- ℓ = 3 (插入侧哈希展开)
- k = 2 (查询侧采样)
- λ = 256 bits (安全参数)
- M = 1024 (QTree 容量)

#### 变化参数
- N ∈ {100, 500, 1000, 5000, 10000} (文档数)
- n ∈ {1, 2, 3, 4, 5} (查询关键词数)
- |DB(w)| ∈ {10, 50, 100, 500} (关键词匹配文档数)

### 4.2 评估指标

#### 时间开销
1. **Setup 时间**: 系统初始化
2. **Update 时间**: 单次更新操作
3. **GenToken 时间**: 令牌生成
4. **Search 时间**: 服务器搜索
5. **Verify 时间**: 客户端验证（仅 Verifiable）

#### 空间开销
1. **索引大小**: TSet + XSet + QTree
2. **令牌大小**: 搜索令牌
3. **证明大小**: 认证路径（仅 Verifiable）

#### 通信开销
1. **Update 通信量**: 客户端 → 服务器
2. **Search 通信量**: 双向总量

### 4.3 对比实验

#### 实验 1: 扩展性测试
- **变量**: N (文档数)
- **固定**: n=2, |DB(w)|=100
- **对比**: Nomos vs Verifiable vs MC-ODXT
- **指标**: Update 时间, Search 时间

#### 实验 2: 查询复杂度测试
- **变量**: n (关键词数)
- **固定**: N=1000, |DB(w)|=100
- **对比**: Nomos vs Verifiable vs MC-ODXT
- **指标**: GenToken 时间, Search 时间

#### 实验 3: 匹配度测试
- **变量**: |DB(w)| (匹配文档数)
- **固定**: N=1000, n=2
- **对比**: Nomos vs Verifiable vs MC-ODXT
- **指标**: Search 时间, 通信开销

#### 实验 4: 验证开销测试
- **变量**: N_+ (匹配文档数)
- **固定**: N=1000, n=2
- **对比**: Verifiable Nomos (有验证 vs 无验证)
- **指标**: 客户端验证时间, 证明大小

---

## 五、代码组织结构

### 5.1 当前目录结构

```
Nomos/
├── include/
│   ├── core/                    # 核心组件
│   │   ├── Experiment.hpp       # 实验基类
│   │   ├── ExperimentFactory.hpp
│   │   └── Primitive.hpp        # 密码学原语
│   ├── nomos/                   # Nomos 基线方案
│   │   ├── Gatekeeper.hpp       # 原始实现（有问题）
│   │   ├── Server.hpp
│   │   ├── Client.hpp
│   │   ├── types.hpp
│   │   ├── GatekeeperCorrect.hpp    # 正确实现
│   │   ├── ClientCorrect.hpp
│   │   ├── ServerCorrect.hpp
│   │   ├── types_correct.hpp
│   │   └── NomosSimplifiedExperiment.hpp
│   ├── verifiable/              # 可验证方案
│   │   ├── QTree.hpp
│   │   ├── AddressCommitment.hpp
│   │   └── VerifiableExperiment.hpp
│   └── mc-odxt/                 # MC-ODXT 方案
│       └── McOdxtExperiment.hpp
├── src/
│   ├── core/
│   │   └── Primitive.cpp
│   ├── nomos/
│   │   ├── Gatekeeper.cpp       # 原始实现
│   │   ├── Server.cpp
│   │   ├── Client.cpp
│   │   ├── GatekeeperCorrect.cpp    # 正确实现
│   │   ├── ClientCorrect.cpp
│   │   ├── ServerCorrect.cpp
│   │   └── NomosSimplifiedExperiment.cpp
│   ├── verifiable/
│   │   ├── QTree.cpp
│   │   ├── AddressCommitment.cpp
│   │   └── VerifiableExperiment.cpp
│   ├── mc-odxt/
│   │   └── McOdxtExperiment.cpp
│   └── main.cpp
├── tests/
├── CMakeLists.txt
├── IMPLEMENTATION_STATUS.md     # 实现状态
├── SCHEME_COMPARISON.md         # 本文档
└── README.md
```

### 5.2 运行命令

```bash
# 编译
cd build
cmake ..
make

# 运行 Nomos 基线（原始实现）
./Nomos nomos

# 运行 Nomos Simplified Correct（正确实现）
./Nomos nomos-simplified

# 运行 Verifiable Nomos
./Nomos verifiable

# 运行 MC-ODXT（待实现）
./Nomos mc-odxt
```

---

## 六、开发优先级

### P0 - 立即完成（本周）
1. ✅ 完成 Nomos Simplified Correct 实现
2. ⏳ 修复编译错误并运行测试
3. ⏳ 验证搜索结果正确性

### P1 - 近期完成（下周）
1. ⏳ 修复 Verifiable Nomos 的 QTree 路径验证
2. ⏳ 实现基准测试框架骨架
3. ⏳ 实现 MC-ODXT 基础协议

### P2 - 后续完成（两周内）
1. ⏳ 完成所有对比实验
2. ⏳ 生成论文第4章所需数据
3. ⏳ 性能优化与调优

---

## 七、参考文献

### 论文方案
1. **Nomos**: Bag et al., "Nomos: Scalable Multi-Client Searchable Encryption" (2024)
2. **ODXT**: Cash et al., "Highly-Scalable Searchable Symmetric Encryption with Support for Boolean Queries" (CRYPTO 2013)
3. **Merkle Tree**: Merkle, "A Digital Signature Based on a Conventional Encryption Function" (CRYPTO 1987)

### 密码学库
1. **RELIC**: https://github.com/relic-toolkit/relic (椭圆曲线)
2. **OpenSSL**: https://www.openssl.org/ (哈希与随机数)
3. **GMP**: https://gmplib.org/ (大整数运算)

---

**文档版本**: 1.0
**最后更新**: 2026-02-27
**维护者**: Cyan
**项目路径**: `/Users/cyan/code/C++/Nomos`
