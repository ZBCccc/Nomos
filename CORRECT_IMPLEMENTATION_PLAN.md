# Nomos 协议正确实现计划

## 当前实现的问题

❌ **严重错误**：当前实现完全偏离了 Nomos 论文规范

### 错误点

1. **密钥结构错误**：
   - 论文：$K_S \in \mathbb{Z}_p^*$, $K_T = \{K_T^1, \ldots, K_T^d\}$, $K_X = \{K_X^1, \ldots, K_X^d\}$
   - 当前：只有三个 bn_t 类型的密钥

2. **Update 协议错误**：
   - 论文：$\mathsf{addr} \leftarrow (H(w\|\mathsf{UpdateCnt}[w]\|0))^{K_T[I(w)]}$ （椭圆曲线点）
   - 当前：`addr = PRF(Kz, cnt)` （SHA256 哈希）

3. **缺少 OPRF 协议**：
   - 论文：Client 和 Gate-keeper 之间有完整的盲化协议
   - 当前：完全没有实现 OPRF

4. **令牌结构错误**：
   - 论文：$\tau = (\mathsf{strap}, \{\mathsf{bstag}_j\}, \{\delta_j\}, \{\overline{\mathsf{bxtrap}}_j\}, \mathsf{env})$
   - 当前：简化的字符串列表

5. **搜索协议错误**：
   - 论文：Server 使用 $\gamma_j$ 和 $\rho_i$ 解盲，计算 $\mathsf{xtag}$
   - 当前：直接字符串匹配

## 正确实现方案

### 阶段 1：数据结构定义

```cpp
// 密钥结构
struct NomosKeys {
    bn_t Ks;                    // OPRF 密钥
    std::vector<bn_t> Kt;       // TSet 密钥数组
    std::vector<bn_t> Kx;       // XSet 密钥数组
    bn_t Ky;                    // PRF 密钥
    std::string Km;             // AE 密钥
};

// TSet 条目
struct TSetEntry {
    std::string val;            // 加密的 (id||op)
    ep_t alpha;                 // 椭圆曲线点
};

// XSet 位数组
std::unordered_map<std::string, bool> XSet;  // xtag -> bit

// 令牌结构
struct NomosToken {
    ep_t strap;                              // H(w1)^Ks
    std::vector<ep_t> bstag;                 // 盲化的 stag
    std::vector<ep_t> delta;                 // 解密掩码
    std::vector<std::vector<ep_t>> bxtrap;   // 盲化的 xtrap
    std::string env;                         // 加密的随机数
};
```

### 阶段 2：Update 协议（算法 2）

```cpp
Metadata Update(OP op, const std::string& id, const std::string& keyword) {
    // Step 1: Kz = F((H(w))^Ks, 1)
    ep_t hw;
    ep_new(hw);
    Hash_H1(hw, keyword);
    ep_mul(hw, hw, Ks);
    bn_t kz;
    bn_new(kz);
    // PRF: F(hw, 1) -> kz

    // Step 2: UpdateCnt[w]++
    if (UpdateCnt.find(keyword) == UpdateCnt.end()) {
        UpdateCnt[keyword] = 0;
    }
    UpdateCnt[keyword]++;
    int cnt = UpdateCnt[keyword];

    // Step 3: addr = (H(w||cnt||0))^Kt[I(w)]
    ep_t addr_point;
    ep_new(addr_point);
    std::string addr_input = keyword + "|" + std::to_string(cnt) + "|0";
    Hash_H1(addr_point, addr_input);
    int index = I(keyword);  // 哈希到索引
    ep_mul(addr_point, addr_point, Kt[index]);

    // Step 4: val = (id||op) ⊕ (H(w||cnt||1))^Kt[I(w)]
    ep_t mask;
    ep_new(mask);
    std::string mask_input = keyword + "|" + std::to_string(cnt) + "|1";
    Hash_H1(mask, mask_input);
    ep_mul(mask, mask, Kt[index]);
    // XOR encryption

    // Step 5: alpha = Fp(Ky, id||op) · (Fp(Kz, w||cnt))^{-1}
    bn_t alpha_bn;
    // OPRF computation

    // Step 6: xtag_i = H(w)^{Kx[I(w)] · Fp(Ky, id||op) · i}
    for (int i = 1; i <= ell; ++i) {
        ep_t xtag;
        // Compute xtag
    }
}
```

### 阶段 3：GenToken 协议（算法 3）

需要实现完整的 Client-Gate-keeper 交互：

1. **Client 盲化**：
   - 采样随机数 $r_1, \ldots, r_n$
   - 计算 $a_j = (H(w_j))^{r_j}$
   - 发送给 Gate-keeper

2. **Gate-keeper 签名**：
   - 计算 $\mathsf{strap}' = (a_1)^{K_S}$
   - 计算 $\mathsf{bxtrap}'_j = (a_j)^{K_X[I_j] \cdot \rho_j}$
   - 采样 RBF 索引 $\beta_1, \ldots, \beta_k$
   - 返回盲化令牌

3. **Client 解盲**：
   - $\mathsf{strap} = (\mathsf{strap}')^{r_1^{-1}}$
   - $\overline{\mathsf{bxtrap}}_j = \{(\overline{\mathsf{bxtrap}}'_j[l])^{r_j^{-1}}\}$

### 阶段 4：Search 协议（算法 4）

1. **Client 计算 xtoken**：
   - $K_Z \leftarrow F(\mathsf{strap}, 1)$
   - $e_j \leftarrow F_p(K_Z, w_1\|j)$
   - $\mathsf{xtoken}_{i,j}^{(t)} \leftarrow (\overline{\mathsf{bxtrap}}_i[t])^{e_j}$

2. **Server 验证**：
   - 解密 $\mathsf{env}$ 获取 $\rho_i, \gamma_j$
   - $\mathsf{stag}_j \leftarrow (\mathsf{stokenList}[j])^{1/\gamma_j}$
   - $\mathsf{xtag}_{i,j} \leftarrow (\mathsf{xtokenSet}_{i,j}[t])^{\alpha_j/\rho_i}$
   - 检查 $\mathsf{XSet}[\mathsf{xtag}_{i,j}]$

## 实现难点

1. **椭圆曲线运算**：所有地址、令牌都是椭圆曲线点
2. **OPRF 协议**：需要实现完整的盲化/解盲流程
3. **密钥数组**：$K_T$ 和 $K_X$ 是数组，需要索引函数 $I(w)$
4. **RBF 采样**：需要实现 Randomized Bloom Filter 的索引采样
5. **认证加密**：$\mathsf{env}$ 需要使用 AE 加密随机数

## 建议

由于 Nomos 协议极其复杂，建议：

1. **先实现简化版本用于测试**：
   - 跳过 OPRF 盲化（直接传递明文）
   - 使用哈希代替椭圆曲线点
   - 简化令牌结构

2. **逐步完善**：
   - 第一版：功能正确但不安全（明文传递）
   - 第二版：添加椭圆曲线运算
   - 第三版：完整 OPRF 协议

3. **或者使用现有实现**：
   - 查找 Nomos 论文的开源实现
   - 参考 OXT 的实现（Nomos 基于 OXT）

## 当前状态

❌ 当前实现：完全错误，需要重写
✅ 测试框架：可以保留
✅ 实验结构：可以保留

## 下一步

**选项 A**：完整实现 Nomos（需要 2-3 天）
**选项 B**：实现简化版本用于论文实验（需要 1 天）
**选项 C**：寻找 Nomos 开源实现并集成（推荐）

---

**结论**：我之前的实现是错误的，向您道歉。需要根据论文算法完全重写。
