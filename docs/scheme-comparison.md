# 方案对照与论文对应关系

## 文档说明

本文档按当前代码现状说明三类方案与论文结构的对应关系，并明确它们各自的实现边界。

本文档只描述当前主线，不再沿用历史上的 `*Correct` 命名。

---

## 一、论文章节与方案对应

### 论文结构（HUST-PhD-Thesis-Latex）

```
论文/
├── chapter1/ intro.tex
├── chapter2/ bf.tex
├── chapter3/ commitment.tex
├── chapter4/ experiments.tex
└── chapter5/ conclusion.tex
```

### 当前对应关系

| 论文章节 | 方案名称 | 当前代码模块 | 当前状态 | 说明 |
|---------|---------|-------------|---------|------|
| 第2章 `bf.tex` | Nomos | `nomos/` | ✅ 可运行 | 主线协议实现 |
| 第3章 `commitment.tex` | Verifiable Nomos | `verifiable/` | ⚠️ 组件完成，协议未接线 | `QTree + AddressCommitment` |
| 第4章 `experiments.tex` | MC-ODXT | `mc-odxt/` | ✅ 原型可运行 | 对比方案原型 |
| 第4章 `experiments.tex` | 基准测试 | `benchmark/` | ✅ 可运行 | 已有 Chapter 4 实验入口 |

---

## 二、方案说明

### 2.1 Nomos

#### 当前代码

- `include/nomos/Gatekeeper.hpp`
- `include/nomos/Client.hpp`
- `include/nomos/Server.hpp`
- `include/nomos/types.hpp`
- `src/nomos/Gatekeeper.cpp`
- `src/nomos/Client.cpp`
- `src/nomos/Server.cpp`

#### 当前两条调用路径

1. **完整 OPRF 路径**
   - `genTokenPhase1 -> genTokenGatekeeper -> genTokenPhase2`
   - 用于主线协议与严格实验口径

2. **非 OPRF 快捷路径**
   - `genTokenSimplified`
   - 用于 `nomos-simplified`

#### 当前特点

- 使用椭圆曲线群运算
- 使用统一的 `F / F_p`
- 已支持完整 OPRF 盲化协议
- `nomos-simplified` 仍保留为快捷实验路径

---

### 2.2 Verifiable Nomos

#### 当前代码

- `include/verifiable/QTree.hpp`
- `include/verifiable/AddressCommitment.hpp`
- `src/verifiable/QTree.cpp`
- `src/verifiable/AddressCommitment.cpp`
- `include/verifiable/VerifiableExperiment.hpp`

#### 当前特点

- `QTree` 已可独立运行
- `AddressCommitment` 已可独立运行
- `verifiable` 实验入口当前做的是组件级验证

#### 当前边界

- 还没有完整的 `VerifiableScheme.cpp`
- `VerifiableScheme.hpp` 仍是草稿接口
- Chapter 4 中的 `VQNomos` 仍是 `Nomos + 开销估计`

---

### 2.3 MC-ODXT

#### 当前代码

当前参与运行的主角包括：

- `McOdxtGatekeeper`
- `McOdxtClient`
- `McOdxtServer`

#### 当前实现位置

当前生效实现主要编译自：

- `include/mc-odxt/McOdxtTypes.hpp`
- `include/mc-odxt/McOdxtExperiment.hpp`
- `src/mc-odxt/McOdxtProtocol.cpp`
- `src/mc-odxt/McOdxtExperiment.cpp`

#### 当前特点

- 已可运行
- 已接入 Chapter 4 实验
- 与 `Nomos` 当前简化链路同构
- 唯一保留的工程差异是 `MC-ODXT` 仅使用 map 维护索引

---

## 三、实现状态对比

| 维度 | Nomos | Verifiable Nomos | MC-ODXT |
|------|-------|------------------|---------|
| 主体实现是否可运行 | ✅ | ⚠️ 组件可运行 | ✅ |
| 是否接入 Chapter 4 | ✅ | ⚠️ 估算方式接入 | ✅ |
| 是否有完整协议主链路 | ✅ | ❌ | ⚠️ 原型级 |
| 是否保留历史双轨命名 | ❌ | ⚠️ 草稿接口仍有历史残留 | ⚠️ `include/`/`src/` 双轨 |

---

## 四、工程结构

### 当前目录结构

```
Nomos/
├── include/
│   ├── core/
│   ├── nomos/
│   ├── verifiable/
│   └── mc-odxt/
├── src/
│   ├── core/
│   ├── nomos/
│   ├── verifiable/
│   ├── benchmark/
│   └── mc-odxt/
├── tests/
├── scripts/
├── results/
└── CMakeLists.txt
```

### 当前关键入口

- 主程序入口：`src/main.cpp`
- Chapter 4 实验：`src/benchmark/ClientSearchFixedW1Experiment.cpp`
- `Nomos` 协议演示：`src/nomos/NomosSimplifiedExperiment.cpp`
- `Verifiable` 组件实验：`include/verifiable/VerifiableExperiment.hpp`
- `MC-ODXT` 原型实验：`include/mc-odxt/McOdxtExperiment.cpp`

---

## 五、运行命令

```bash
cd /Users/cyan/code/paper/Nomos
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build

cd build
./Nomos nomos-simplified
./Nomos verifiable
./Nomos mc-odxt
./Nomos chapter4-client-search-fixed-w1
./tests/nomos_test
```

---

## 六、当前重构重点

### P0

1. 统一 `MC-ODXT` 的源码位置
2. 完整接线 Verifiable 协议
3. 统一实验口径与图表管线

### P1

1. 清理历史文档残留
2. 收敛 `Nomos` 快捷路径与主线路径的命名
3. 将 `VQNomos` 从估算值升级为协议实测

---

**最后更新**: 2026-03-13
