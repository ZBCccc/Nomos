# 三个方案当前实现方式

## 文档目的

这份文档用于说明 `Nomos/` 中三个实验方案当前的真实实现方式，作为后续实验代码重构的基线说明。

本文档只以当前实际编译和实际调用的代码为准，不以旧文档、旧命名或历史计划为准。

三个方案分别是：

1. `Nomos`
2. `VQNomos`
3. `MC-ODXT`

---

## 1. 当前总入口

### 1.1 可执行入口

统一入口在：

- `src/main.cpp`

当前注册的实验名包括：

- `nomos-simplified`
- `mc-odxt`
- `verifiable`
- `benchmark`
- `comparative-benchmark`
- `chapter4-client-search-fixed-w1`

### 1.2 当前构建入口

当前 CMake 主入口在：

- `CMakeLists.txt`

当前实际会被编译进 `nomos_core` 的相关文件是：

- `src/nomos/Gatekeeper.cpp`
- `src/nomos/Client.cpp`
- `src/nomos/Server.cpp`
- `src/nomos/NomosSimplifiedExperiment.cpp`
- `src/verifiable/QTree.cpp`
- `src/verifiable/AddressCommitment.cpp`
- `src/benchmark/ClientSearchFixedW1Experiment.cpp`
- `include/mc-odxt/McOdxtProtocol.cpp`
- `include/mc-odxt/McOdxtExperiment.cpp`

这里有一个需要特别注意的事实：

- `MC-ODXT` 当前编译的是 `include/mc-odxt/*.cpp`
- `src/mc-odxt/*.cpp` 也存在，但不是当前生效实现

因此，后续重构必须先统一 `MC-ODXT` 的源码位置，否则很容易改错文件。

---

## 2. Nomos 当前实现方式

## 2.1 当前核心代码

核心实现位于：

- `include/nomos/Gatekeeper.hpp`
- `include/nomos/Client.hpp`
- `include/nomos/Server.hpp`
- `include/nomos/types.hpp`
- `src/nomos/Gatekeeper.cpp`
- `src/nomos/Client.cpp`
- `src/nomos/Server.cpp`

### 2.2 当前实现结构

Nomos 当前是一个三方结构：

1. `Gatekeeper`
   - 负责密钥生成与维护
   - 负责更新元数据生成
   - 负责 OPRF 的 Gatekeeper 侧计算
   - 也保留了一个简化版 `genTokenSimplified`

2. `Client`
   - 负责 OPRF 客户端第 1 阶段 `genTokenPhase1`
   - 负责 OPRF 客户端第 2 阶段 `genTokenPhase2`
   - 负责 `prepareSearch`
   - 负责 `decryptResults`
   - 也保留了一个简化版 `genTokenSimplified`

3. `Server`
   - 负责维护 `TSet`
   - 负责维护 `XSet`
   - 负责执行 `search`

### 2.3 当前协议实现分成两条路径

#### 路径 A：`nomos-simplified`

运行入口：

- `src/nomos/NomosSimplifiedExperiment.cpp`

这条路径调用的是：

- `Client::genTokenSimplified(...)`
- `Gatekeeper::genTokenSimplified(...)`
- `Client::prepareSearch(...)`
- `Server::search(...)`
- `Client::decryptResults(...)`

它的特点是：

- 更新过程是真实的椭圆曲线实现
- 搜索流程使用“简化 token 生成”
- 跳过了完整交互式 OPRF 盲化
- 更接近“协议正确性演示/集成自测”

也就是说，`nomos-simplified` 不是当前最完整的实验口径，而是一个便于单独跑通协议的小型集成实验。

#### 路径 B：Chapter 4 实验路径

运行入口：

- `src/benchmark/ClientSearchFixedW1Experiment.cpp`

在这条路径里，Nomos 当前走的是完整客户端两阶段搜索令牌流程：

1. `Client::genTokenPhase1(...)`
2. `Gatekeeper::genTokenGatekeeper(...)`
3. `Client::genTokenPhase2(...)`
4. `Client::prepareSearch(...)`
5. `Server::search(...)`
6. `Client::decryptResults(...)`

这条路径是当前第四章实验里最接近“真实搜索流程”的 Nomos 实现。

### 2.4 当前实验口径下的 Nomos

在 `chapter4-client-search-fixed-w1` 里，Nomos 当前采用的是“严格纯客户端搜索耗时”口径：

- 计时包含：
  - `genTokenPhase1`
  - `genTokenPhase2`
  - `prepareSearch`
  - `decryptResults`
- 不计入：
  - `Gatekeeper::genTokenGatekeeper`
  - `Server::search`

因此，当前 CSV 中 `Nomos` 列对应的不是全链路搜索时间，而是客户端本地时间。

### 2.5 当前 Nomos 的结论

Nomos 当前已经具备：

- 可独立运行的核心协议实现
- 可用于第四章实验的搜索流程
- 两套 token 路径：完整两阶段 OPRF 路径、简化路径

Nomos 当前的主要问题不是“没有实现”，而是：

- 同时存在“简化路径”和“实验路径”，语义容易混淆
- 不同实验类对 Nomos 的调用口径不统一

---

## 3. VQNomos 当前实现方式

## 3.1 当前真正存在的代码

当前已实现并编译的可验证组件只有：

- `include/verifiable/QTree.hpp`
- `include/verifiable/AddressCommitment.hpp`
- `src/verifiable/QTree.cpp`
- `src/verifiable/AddressCommitment.cpp`
- `include/verifiable/VerifiableExperiment.hpp`

### 3.2 当前可运行入口

可执行入口是：

- `verifiable`

对应实现是：

- `include/verifiable/VerifiableExperiment.hpp`

这个实验当前只做两件事：

1. 单独测试 `QTree`
2. 单独测试 `AddressCommitment`

它不是一个已经接入 Nomos 更新/搜索主链路的完整 `VQNomos` 协议。

### 3.3 当前没有真正打通的部分

仓库里还存在：

- `include/verifiable/VerifiableScheme.hpp`

但它当前不是可落地的完整实现，原因包括：

1. 只有头文件，没有对应 `src/verifiable/VerifiableScheme.cpp`
2. 该头文件中仍然引用了 `nomos::GateKeeper`
   - 当前真实类名已经是 `nomos::Gatekeeper`
3. 相关类没有被 `CMakeLists.txt` 编译进主库
4. 代码中也没有其他地方真正调用 `updateVerifiable(...)` 或 `searchVerifiable(...)`

因此，`VerifiableScheme.hpp` 当前更接近“未完成接口草稿”，不是当前有效实现。

### 3.4 当前 Chapter 4 里的 VQNomos 是怎么来的

在当前第四章实验里，`VQNomos` 不是独立真实执行出来的协议实现，而是：

- 先真实测 `Nomos` 的客户端时间
- 再调用 `estimateVQNomosClientSearchTime(...)`
- 在 `Nomos` 时间上叠加一个固定验证开销估计

当前估计项包括：

- QTree 路径哈希开销
- 地址承诺验证开销

对应代码在：

- `src/benchmark/ClientSearchFixedW1Experiment.cpp`

CSV 中也已经通过 `measurement_mode=estimated_over_nomos` 显式标记了这一点。

### 3.5 当前 VQNomos 的结论

当前所谓 `VQNomos` 实际上分成两层：

1. 底层组件层
   - `QTree`
   - `AddressCommitment`
   - 这两部分是真实代码

2. 第四章实验层
   - 不是完整协议
   - 而是 `Nomos + 固定验证开销估计`

因此，当前 VQNomos 的问题不是“完全没有代码”，而是“缺少完整协议级接线”。

---

## 4. MC-ODXT 当前实现方式

## 4.1 当前真正生效的代码位置

当前 `MC-ODXT` 的有效实现主要位于：

- `include/mc-odxt/McOdxtTypes.hpp`
- `include/mc-odxt/McOdxtProtocol.cpp`
- `include/mc-odxt/McOdxtExperiment.cpp`

注意：

- `src/mc-odxt/McOdxtProtocol.cpp`
- `src/mc-odxt/McOdxtExperiment.cpp`

这两份文件目前存在，但不是当前 CMake 实际编译的版本。

从当前代码组织看，`MC-ODXT` 处于“实现已写在 include 目录，src 目录保留旧壳子”的状态。

### 4.2 当前实现结构

当前 `MC-ODXT` 包含四类角色：

1. `McOdxtGatekeeper`
   - 管理 owner 密钥和 user 密钥
   - 管理授权关系
   - 维护每个 owner 的 `updateCnt`
   - 负责生成搜索 token

2. `McOdxtDataOwner`
   - 负责生成更新元数据
   - 调用 Gatekeeper 注册更新关系
   - 生成 `addr / val / alpha / xtags`

3. `McOdxtClient`
   - 请求 Gatekeeper 生成 token
   - 本地执行 `prepareSearch`
   - 本地执行 `decryptResults`

4. `McOdxtServer`
   - 维护按 `owner_id` 分区的 `TSet`
   - 维护按 `owner_id` 分区的 `XSet`
   - 执行搜索

### 4.3 当前实现特征

当前 `MC-ODXT` 已经不是“待实现”状态，而是有一套可运行原型。

但它有几个非常重要的现状特征：

1. 这是一个“多客户端改造版 ODXT 原型”
   - 有 owner / search user / gatekeeper / server 四方角色
   - 有授权逻辑
   - 有 per-owner 状态

2. 实现中仍含有明显的简化处理
   - 若干 PRF/映射逻辑是简化版本
   - `env` 在搜索中基本为空或弱化处理
   - 与论文 ODXT 的逐行对应还不完全严格

3. 当前 `mc-odxt` 命令跑的是一个简化 benchmark
   - 入口在 `include/mc-odxt/McOdxtExperiment.cpp`
   - 用固定用户数和固定文档数做一次演示型测试

### 4.4 当前 Chapter 4 里的 MC-ODXT 是怎么跑的

在 `chapter4-client-search-fixed-w1` 中，`MC-ODXT` 使用的是：

- `McOdxtGatekeeper`
- `McOdxtServer`
- `McOdxtClient`
- `McOdxtDataOwner`

也就是说，第四章实验调用的是当前这套原型实现，而不是伪造数据。

不过它的“严格纯客户端搜索耗时”口径和 Nomos 不完全同构：

- 计时包含：
  - `McOdxtClient::prepareSearch`
  - `McOdxtClient::decryptResults`
- 不计入：
  - `McOdxtClient::genToken(...)`
    - 因为这个函数本身会直接委托给 `Gatekeeper::genToken(...)`
  - 服务器搜索

所以，当前实验里的 `MC-ODXT` 客户端时间，本质上是“客户端本地 prepare/decrypt 时间”，而不是“客户端发起查询到拿到 token 的完整本地时间”。

### 4.5 当前 MC-ODXT 的结论

当前 `MC-ODXT` 已有可运行原型，也已接入第四章实验。

它的主要问题不是“没有实现”，而是：

- 生效源码路径不直观
- `include/` 和 `src/` 双份实现并存
- 当前实验计时边界与 Nomos 并不完全对称

---

## 5. 当前三个方案的真实状态总结

| 方案 | 当前是否有真实代码 | 当前是否接入 Chapter 4 实验 | 当前实验值是否真实运行得到 | 备注 |
|------|--------------------|-----------------------------|----------------------------|------|
| `Nomos` | 是 | 是 | 是 | 当前 Chapter 4 中使用完整客户端两阶段 token 流程 |
| `VQNomos` | 部分有 | 是 | 否，当前为估算值 | 只有 `QTree + AddressCommitment` 组件真实实现，协议级尚未打通 |
| `MC-ODXT` | 是 | 是 | 是 | 真实运行的是多客户端 ODXT 原型，但当前生效实现位于 `include/mc-odxt/*.cpp` |

---

## 6. 对后续重构最重要的含义

如果后续要做实验代码重构，当前最需要先解决的不是性能，而是“实现边界不统一”。

按优先级，当前最关键的三个重构点是：

1. 统一 `MC-ODXT` 的源码位置
   - 结束 `include/` 和 `src/` 双轨并存状态

2. 明确 `Nomos` 的两条调用路径
   - 哪条是协议演示
   - 哪条是实验基线

3. 明确 `VQNomos` 的定位
   - 是继续保留“估算层”
   - 还是真正补全协议级集成

在这三点没梳理清楚之前，继续叠加新实验，后面会越来越难维护。
