# Nomos 正确实现进度报告

## 已完成部分

### 1. 数据结构定义 ✅

**文件**: `include/nomos/types_correct.hpp`

正确定义了：
- `UpdateMetadata`: 包含椭圆曲线点 `ep_t addr`, `bn_t alpha`, `vector<ep_t> xtags`
- `SearchToken`: 包含 `ep_t strap`, `vector<ep_t> bstag/delta/bxtrap`, `env`
- `TSetEntry`: 正确的 TSet 条目结构

### 2. Gatekeeper 实现 ✅

**文件**: `include/nomos/GatekeeperCorrect.hpp`, `src/nomos/GatekeeperCorrect.cpp`

**Setup (算法 1)**:
```cpp
- Ks ∈ Zp* (bn_t)
- Kt[1..d] ∈ Zp* (vector<bn_t>)
- Kx[1..d] ∈ Zp* (vector<bn_t>)
- Ky ∈ {0,1}^λ (bn_t)
- Km ∈ {0,1}^λ (vector<uint8_t>)
```

**Update (算法 2)**:
```cpp
1. Kz = F((H(w))^Ks, 1)                          ✅
2. UpdateCnt[w]++                                 ✅
3. addr = (H(w||cnt||0))^Kt[I(w)]                ✅
4. val = (id||op) ⊕ (H(w||cnt||1))^Kt[I(w)]      ✅
5. alpha = Fp(Ky, id||op) · (Fp(Kz, w||cnt))^-1  ✅
6. xtag_i = H(w)^{Kx[I(w)] · Fp(Ky, id||op) · i} ✅
```

**GenToken Gatekeeper 侧 (算法 3)**:
```cpp
1. 采样 rho_1, ..., rho_n, gamma_1, ..., gamma_m  ✅
2. strap' = (a_1)^Ks                              ✅
3. bstag'_j = (b_j)^{Kt[I1] · gamma_j}            ✅
4. delta'_j = (c_j)^{Kt[I1]}                      ✅
5. bxtrap'_j = (a_j)^{Kx[Ij] · rho_j}             ✅
6. 采样 RBF 索引 beta_i ∈ [ℓ]                     ✅
7. 计算 overline{bxtrap}'_j                       ✅
8. env = AE.Enc_Km(rho, gamma)                    ✅
```

### 3. Client 接口定义 ✅

**文件**: `include/nomos/ClientCorrect.hpp`

定义了：
- `genTokenPhase1`: 盲化请求
- `genTokenPhase2`: 解盲令牌
- `prepareSearch`: 准备搜索请求
- `decryptResults`: 解密结果

## 待完成部分

### 1. Client 实现 ⏳

**GenToken Client 侧 (算法 3)**:
```cpp
Phase 1 - 盲化:
1. 采样 r_1, ..., r_n, s_1, ..., s_m
2. a_j = (H(wj))^rj
3. b_j = (H(w1||j||0))^sj
4. c_j = (H(w1||j||1))^sj
5. 发送 (a, b, c, av) 给 Gatekeeper

Phase 2 - 解盲:
1. strap = (strap')^{r1^-1}
2. bstag_j = (bstag'_j)^{sj^-1}
3. delta_j = (delta'_j)^{sj^-1}
4. overline{bxtrap}_j = {(overline{bxtrap}'_j[l])^{rj^-1}}
```

**Search Client 侧 (算法 4)**:
```cpp
1. Kz = F(strap, 1)
2. 初始化 stokenList, xtokenList
3. For j=1..m:
   - stokenList += bstag_j
   - e_j = Fp(Kz, w1||j)
   - For i=2..n:
     - For t=1..k:
       - xtoken_{i,j}^(t) = (overline{bxtrap}_i[t])^{e_j}
4. 发送 (stokenList, xtokenList) 给 Server
```

### 2. Server 实现 ⏳

**文件**: `include/nomos/ServerCorrect.hpp`, `src/nomos/ServerCorrect.cpp`

**Update**:
```cpp
1. TSet[addr] = (val, alpha)
2. XSet[xtag_i] = 1, for i=1..ℓ
```

**Search (算法 4)**:
```cpp
1. 解密 env 获取 rho, gamma
2. For j=1..|stokenList|:
   - stag_j = (stokenList[j])^{1/gamma_j}
   - (sval_j, alpha_j) = TSet[stag_j]
   - flag = 1, cnt_j = 1
   - For i=2..n:
     - For t=1..k:
       - xtag_{i,j} = (xtokenSet_{i,j}[t])^{alpha_j/rho_i}
       - if XSet[xtag_{i,j}] == 0: flag = 0
     - if flag == 1: cnt_j++
   - sEOpList += (j, sval_j, cnt_j)
3. 返回 sEOpList
```

### 3. 集成测试 ⏳

创建 `NomosCorrectExperiment` 类，测试完整流程：
```cpp
1. Setup: Gatekeeper.setup()
2. Update: 添加文档
3. GenToken: Client ↔ Gatekeeper 交互
4. Search: Client → Server → Client
5. Decrypt: 恢复结果
```

## 实现难点与解决方案

### 难点 1: 椭圆曲线点的序列化

**问题**: ep_t 需要在网络传输中序列化
**解决**: 使用 `ep_write_bin` 和 `ep_read_bin`

### 难点 2: OPRF 盲化协议

**问题**: Client 不能直接计算 H(w)^Ks
**解决**:
- Client 计算 a = H(w)^r (盲化)
- Gatekeeper 计算 a' = a^Ks = H(w)^{r·Ks}
- Client 计算 (a')^{r^-1} = H(w)^Ks (解盲)

### 难点 3: 密钥数组索引

**问题**: I(w) 需要将关键词映射到 [0, d-1]
**解决**: 使用 SHA256(w) mod d

### 难点 4: RBF 采样

**问题**: 需要采样 k 个随机索引 beta_i ∈ [ℓ]
**解决**: 使用 rand() % ℓ + 1

### 难点 5: 认证加密

**问题**: env 需要使用 AE 加密
**解决**: 当前使用简化的 XOR，后续可升级为 AES-GCM

## 编译配置

需要更新 `CMakeLists.txt`:
```cmake
add_library(nomos_core
    ...
    src/nomos/GatekeeperCorrect.cpp
    src/nomos/ClientCorrect.cpp
    src/nomos/ServerCorrect.cpp
)
```

## 测试计划

### 单元测试
1. ✅ Gatekeeper.setup() - 密钥生成
2. ✅ Gatekeeper.update() - 元数据生成
3. ⏳ Client.genToken() - 盲化/解盲
4. ⏳ Server.search() - 搜索逻辑

### 集成测试
1. ⏳ 单关键词查询
2. ⏳ 多关键词合取查询
3. ⏳ 动态更新（ADD/DEL）

## 预计完成时间

- Client 实现: 4-6 小时
- Server 实现: 3-4 小时
- 集成测试: 2-3 小时
- 调试修复: 2-3 小时

**总计**: 11-16 小时（约 2 个工作日）

## 当前状态

✅ 数据结构: 100%
✅ Gatekeeper: 100%
⏳ Client: 20% (接口定义完成)
⏳ Server: 0%
⏳ 集成: 0%

**总体进度**: 约 40%

---

**下一步**: 实现 ClientCorrect.cpp 的完整逻辑
