# MC-ODXT 多用户方案设计

> 版本：v3.0（基于 NOMOS 论文 + ODXT 论文）  
> 日期：2026-03-01

---

## 1. 概述

### 1.1 设计目标

基于 NOMOS 论文实现，将 ODXT 扩展为多用户模式：

- **一个** Data Owner（你）
- **多个** Search User（审稿人/合作者）
- 保持前向私有 + 后向私有

### 1.2 核心创新（来自 NOMOS）

使用 **布隆过滤器 (Bloom Filter)** 实现 XSet：

```
原 ODXT: XSet = map<xtag, bool>  (精确匹配)
NOMOS:   XSet = BloomFilter         (概率匹配，节省空间)
```

---

## 2. 系统架构

### 2.1 角色

| 角色 | 数量 | 职责 |
|------|------|------|
| **Data Owner** | 1 | 生成密钥、上传文档、授权用户 |
| **Search User** | N | 被授权后搜索 |
| **Gatekeeper** | 1 | 密钥管理、授权、令牌生成 |
| **Server** | 1 | 存储 TSet + XSet(Bloom Filter) |

### 2.2 数据结构

```
TSet (Traditional Map):
    addr → (val, alpha)
    用于：候选文档枚举

XSet (Bloom Filter):
    用于：交叉过滤 (xtag membership test)
    优势：空间效率高、支持批量查询
```

---

## 3. 密钥管理

### 3.1 Owner 密钥

```cpp
struct OwnerKeys {
    bn_t Ks;           // OPRF 密钥：地址计算
    bn_t* Kt;         // 地址加密密钥数组
    bn_t* Kx;         // xtag 生成密钥数组  
    bn_t Ky;           // payload 加密密钥
    bn_t Km;           // xtag 生成密钥
};
```

### 3.2 User 密钥

```cpp
struct UserKeys {
    bn_t Ks_user;     // 用户自己的 OPRF 密钥
};
```

---

## 4. 更新协议

### 4.1 更新流程

```
Data Owner              Gatekeeper              Server
    |                      |                     |
    |--- Update(w, id) --> |                     |
    |                      |                     |
    |                  genToken()                |
    |                      |                     |
    |                      |--- update -------> |
    |                      |   {addr, val}      |
    |                      |   {bloom_filter}   |
    |                      |                     |
```

### 4.2 更新算法

**Gatekeeper.GenUpdateToken()**
```
Input: op (ADD/DELETE), keyword, doc_id, owner_id
Output: UpdateToken

1. cnt = getUpdateCount(keyword) + 1
2. addr = H(keyword || cnt)^Ks
3. val = Enc_Ky(op || doc_id)
4. For i = 1 to ℓ:
       xtag[i] = H(keyword || doc_id || i)^Km
5. 返回 {addr, val, xtag[1..ℓ]}
```

**Server.Update()**
```
Input: UpdateToken

1. TSet[addr] = val
2. For each xtag:
       if op == ADD:
           XSet.add(xtag)      // 布隆过滤器添加
       else if op == DELETE:
           XSet.remove(xtag)   // 布隆过滤器删除（标记）
```

---

## 5. 搜索协议

### 5.1 搜索流程

```
Search User            Gatekeeper              Server
    |                     |                     |
    |--- Search(w) --> |                     |
    |                     |                     |
    |                 verifyAuth()            |
    |                 genToken()              |
    |                     |                     |
    |<-- token ------ |                     |
    |                     |                     |
    |--- search -----> |                     |
    |                     |                     |
    |                 search()                |
    |                     |                     |
    |<-- results --- |                     |
    |                     |                     |
```

### 5.2 令牌生成（核心：用户隔离）

**Gatekeeper.GenSearchToken()**
```
Input: keywords[], user_id
Output: SearchToken

1. 验证授权

2. 获取密钥: Ks_owner, Kt[], Kx[], Km, Ks_user

3. 生成 stoken (地址枚举):
   For each keyword w:
       For i = 0 to UpdateCnt[w]:
           stoken[i] = H(w || i)^Ks_owner

4. 生成 xtoken (布隆过滤器查询密钥):
   For each keyword w:
       采样 k 个索引
       For j = 1 to k:
           // 关键：结合用户密钥
           xtoken[j] = H(w || sampled_idx)^{Ks_user * Km}
           
5. 返回 SearchToken {stoken_list, xtoken_list}
```

### 5.3 服务器搜索

**Server.Search()**
```
Input: SearchToken

1. 候选枚举 (使用 stoken):
   For each keyword w:
       candidates[w] = TSet.lookup(stoken[w])

2. 交叉过滤 (使用布隆过滤器):
   // XSet 是布隆过滤器，支持批量查询
   results = candidates[w1]
   For each other keyword w:
       results = results ∩ XSet.test_batch(xtoken[w])
       
3. 返回 results
```

---

## 6. 布隆过滤器实现

### 6.1 XSet 布隆过滤器

```cpp
class XSetBloomFilter {
private:
    std::vector<bool> m_bits;     // 位数组
    std::vector<size_t> m_seeds;  // 哈希函数种子
    
public:
    // 添加 xtag
    void add(const std::string& xtag);
    
    // 查询 xtag（可能 false positive）
    bool test(const std::string& xtag) const;
    
    // 批量查询
    std::vector<bool> test_batch(
        const std::vector<std::string>& xtokens) const;
};
```

### 6.2 哈希函数

使用多个哈希函数：
- h_i(xtag) = hash(xtag, seed_i) % m_size

### 6.3 特性

| 特性 | 说明 |
|------|------|
| **空间** | O(n) bits vs O(n) entries |
| **查询** | O(k) 其中 k = 哈希函数数量 |
| **False Positive** | 可接受（不会漏，但可能误判） |
| **删除** | 需要 Countting Bloom Filter |

---

## 7. 安全性分析

### 7.1 隐私属性

| 属性 | 保护机制 |
|------|----------|
| **前向私有** | 动态 xtag，每次更新生成新的 |
| **后向私有** | DELETE 操作更新布隆过滤器 |
| **搜索隐私** | OPRF 盲化 |
| **用户隔离** | xtoken 需要用户密钥 Ks_user |

### 7.2 用户隔离机制

```
原 ODXT (单用户):
    xtoken = H(w || i)^Km

MC-ODXT (多用户):
    xtoken = H(w || i)^{Ks_user * Km}
```

- Server 不知道 Ks_user，无法伪造 xtoken
- 只有授权用户才能生成有效的 xtoken

---

## 8. 与 NOMOS/ODXT 对比

| 特性 | ODXT | NOMOS | MC-ODXT |
|------|-------|-------|---------|
| 用户数 | 1 | 1 | N |
| XSet | map | **Bloom Filter** | Bloom Filter |
| 用户隔离 | 无 | 无 | **有 (Ks_user)** |
| 前向私有 | ✅ | ✅ | ✅ |
| 后向私有 | ✅ | ✅ | ✅ |

---

## 9. 实现计划

### Phase 1: 框架 ✅
- [x] McOdxtTypes.hpp
- [x] McOdxtProtocol.cpp (stub)

### Phase 2: 布隆过滤器
- [ ] XSetBloomFilter 类
- [ ] add/test/test_batch 方法

### Phase 3: 核心协议
- [ ] Owner 密钥生成
- [ ] User 注册/密钥分发
- [ ] 授权表管理
- [ ] 更新协议（支持布隆过滤器）
- [ ] 搜索协议（用户隔离）

### Phase 4: 测试
- [ ] 单元测试
- [ ] 集成测试

---

## 10. 参考文献

1. Patranabis S., Mukhopadhyay D. "Forward and Backward Private Conjunctive Searchable Encryption", NDSS 2021.
2. Bag S., et al. "Tokenised Multi-client Provisioning for Dynamic Searchable Encryption with Forward and Backward Privacy", ACM AsiaCCS 2024.

---

*版本：v3.0*
*最后更新：2026-03-01*
