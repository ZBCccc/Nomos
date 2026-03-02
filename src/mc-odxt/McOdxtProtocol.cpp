#include "mc-odxt/McOdxtTypes.hpp"

#include <iostream>
#include <sstream>

#include "core/Primitive.hpp"

extern "C" {
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
}

namespace mcodxt {

// ============================================
// 辅助函数
// ============================================

static std::string serializePoint(const ep_t point) {
    uint8_t bytes[256];
    int len = ep_size_bin(point, 1);
    ep_write_bin(bytes, len, point, 1);
    return std::string(reinterpret_cast<char*>(bytes), len);
}

static void hashToPoint(ep_t point, const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), 
           input.length(), hash);
    
    // 将哈希值映射到曲线点（简化版本）
    bn_t h;
    bn_new(h);
    bn_read_bin(h, hash, SHA256_DIGEST_LENGTH);
    
    // 使用哈希值作为种子生成点
    ep_map(point, hash, SHA256_DIGEST_LENGTH);
    bn_free(h);
}

// ============================================
// McOdxtClient 实现
// ============================================

McOdxtClient::McOdxtClient(const std::string& user_id)
    : m_user_id(user_id), m_user_type(UserType::SEARCH_USER) {
    bn_null(m_Ks_user);
}

McOdxtClient::~McOdxtClient() {
    bn_free(m_Ks_user);
}

int McOdxtClient::setup() {
    // 生成用户自己的密钥 Ks_user
    bn_t ord;
    bn_new(ord);
    ep_curve_get_ord(ord);
    
    bn_new(m_Ks_user);
    bn_rand_mod(m_Ks_user, ord);
    
    // 生成对称加密密钥 Km
    m_Km.resize(32);
    RAND_bytes(m_Km.data(), 32);
    
    bn_free(ord);
    return 0;
}

SearchToken McOdxtClient::genToken(
    const std::vector<std::string>& query_keywords,
    const std::string& owner_id,
    McOdxtGatekeeper& gatekeeper) {
    // 委托网关生成令牌
    return gatekeeper.genToken(query_keywords, owner_id, m_user_id);
}

McOdxtClient::SearchRequest McOdxtClient::prepareSearch(
    const SearchToken& token,
    const std::vector<std::string>& query_keywords,
    const std::unordered_map<std::string, int>& updateCnt) {
    
    SearchRequest req;
    req.owner_id = token.owner_id;
    req.user_id = token.user_id;
    req.stokenList = token.bstag;
    req.env = token.env;
    
    // 派生 xtokens（与单用户 ODXT 相同）
    int n = query_keywords.size();
    int m = updateCnt.at(query_keywords[0]);  // UpdateCnt[w1]
    
    req.xtokenList.resize(n);
    for (int j = 0; j < n; ++j) {
        std::string kw = query_keywords[j];
        int cnt = (j == 0) ? m : updateCnt.at(kw);
        
        req.xtokenList[j].resize(cnt);
        for (int i = 0; i < cnt; ++i) {
            // bxtrap[j][i][t] = H(wj||i||t)^Ks (简化版)
            req.xtokenList[j][i] = token.bxtrap[j][i];
        }
    }
    
    return req;
}

std::vector<std::string> McOdxtClient::decryptResults(
    const std::vector<SearchResultEntry>& results,
    const SearchToken& token) {

    std::vector<std::string> decrypted;

    // Paper: ODXT.Search decryption (Figure 9)
    // Decrypt: value ⊕ K_m → plaintext → parse (id||op) → filter by ADD

    for (const auto& result : results) {
        // Decrypt using XOR with Km
        std::string plaintext;
        plaintext.resize(result.sval.size());

        for (size_t i = 0; i < result.sval.size(); ++i) {
            plaintext[i] = result.sval[i] ^ m_Km[i % m_Km.size()];
        }

        // Parse plaintext: "doc_id|ADD" or "doc_id|DEL"
        size_t pipe_pos = plaintext.find('|');
        if (pipe_pos != std::string::npos) {
            std::string doc_id = plaintext.substr(0, pipe_pos);
            std::string op = plaintext.substr(pipe_pos + 1);

            // Filter by operation type (only return ADD operations)
            if (op == "ADD") {
                decrypted.push_back(doc_id);
            }
        }
    }

    return decrypted;
}

// ============================================
// McOdxtDataOwner 实现
// ============================================

McOdxtDataOwner::McOdxtDataOwner(const std::string& owner_id)
    : m_owner_id(owner_id), m_Kt(nullptr), m_Kx(nullptr), m_d(0) {
    bn_null(m_Ks_owner);
    bn_null(m_Ky);
}

McOdxtDataOwner::~McOdxtDataOwner() {
    bn_free(m_Ks_owner);
    bn_free(m_Ky);
    
    if (m_Kt) {
        for (int i = 0; i < m_d; ++i) bn_free(m_Kt[i]);
        delete[] m_Kt;
    }
    if (m_Kx) {
        for (int i = 0; i < m_d; ++i) bn_free(m_Kx[i]);
        delete[] m_Kx;
    }
}

int McOdxtDataOwner::setup() {
    m_d = 10;  // 默认密钥数组大小
    
    bn_t ord;
    bn_new(ord);
    ep_curve_get_ord(ord);
    
    // 生成 Ks_owner
    bn_new(m_Ks_owner);
    bn_rand_mod(m_Ks_owner, ord);
    
    // 生成 Kt[1..d]
    m_Kt = new bn_t[m_d];
    for (int i = 0; i < m_d; ++i) {
        bn_new(m_Kt[i]);
        bn_rand_mod(m_Kt[i], ord);
    }
    
    // 生成 Kx[1..d]
    m_Kx = new bn_t[m_d];
    for (int i = 0; i < m_d; ++i) {
        bn_new(m_Kx[i]);
        bn_rand_mod(m_Kx[i], ord);
    }
    
    // 生成 Ky
    bn_new(m_Ky);
    bn_rand_mod(m_Ky, ord);
    
    // 生成 Km
    m_Km.resize(32);
    RAND_bytes(m_Km.data(), 32);
    
    bn_free(ord);
    return 0;
}

int McOdxtDataOwner::indexFunction(const std::string& keyword) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(keyword.c_str()),
           keyword.length(), hash);
    uint32_t index = 0;
    for (int i = 0; i < 4; ++i) {
        index = (index << 8) | hash[i];
    }
    return index % m_d;
}

void McOdxtDataOwner::computeKz(bn_t kz, const std::string& keyword, const bn_t& Ks) {
    // Kz = F((H(w))^Ks, 1)
    ep_t hw;
    ep_new(hw);
    hashToPoint(hw, keyword);
    
    ep_t hwk;
    ep_new(hwk);
    ep_mul(hwk, hw, Ks);  // hwk = H(w)^Ks
    
    // F(hwk, "1")
    std::string input = "1" + serializePoint(hwk);
    computeFp(kz, m_Ky, input);
    
    ep_free(hw);
    ep_free(hwk);
}

void McOdxtDataOwner::computeFp(bn_t result, bn_t key, const std::string& input) {
    // 简化的伪随机函数
    unsigned char hash[SHA256_DIGEST_LENGTH];
    std::string key_str;

    // 将 key 转换为字符串
    int key_size = bn_size_bin(key);
    std::vector<uint8_t> key_bytes(key_size);
    bn_write_bin(key_bytes.data(), key_size, key);
    key_str = std::string(reinterpret_cast<char*>(key_bytes.data()), key_size);

    std::string data = key_str + input;
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()),
           data.length(), hash);

    bn_read_bin(result, hash, std::min<int>(32, SHA256_DIGEST_LENGTH));
}

UpdateMetadata McOdxtDataOwner::update(
    OpType op,
    const std::string& doc_id,
    const std::string& keyword,
    McOdxtGatekeeper& gatekeeper) {

    std::cerr << "DEBUG: update() called for owner=" << m_owner_id << " keyword=" << keyword << " doc_id=" << doc_id << std::endl;
    std::cerr.flush();

    UpdateMetadata meta;
    meta.owner_id = m_owner_id;

    // Get keys from Gatekeeper (MC-ODXT: Gatekeeper manages all keys)
    const bn_t* Kt = gatekeeper.getKt(m_owner_id);
    const bn_t* Kx = gatekeeper.getKx(m_owner_id);
    const bn_t& Ky = gatekeeper.getKy(m_owner_id);
    const std::vector<uint8_t>& Km = gatekeeper.getKm(m_owner_id);

    std::cout << "DEBUG: Got keys from gatekeeper, Km.size()=" << Km.size() << std::endl;

    if (!Kt || !Kx || Km.empty()) {
        throw std::runtime_error("Owner not registered with Gatekeeper");
    }

    // 计算 addr = H(w)^Kt[I(w)]
    int idx = indexFunction(keyword);
    ep_t hw;
    ep_new(hw);
    hashToPoint(hw, keyword);

    ep_new(meta.addr);
    ep_mul(meta.addr, hw, Kt[idx]);  // addr = H(w)^Kt[I(w)]

    // Paper: ODXT.Update (Figure 8) - Encrypt payload
    // value = Enc(K_y, id||op) ⊕ F(K_z, w||c)
    std::string plaintext = doc_id + "|" +
        (op == OpType::ADD ? "ADD" : "DEL");

    // Compute encryption mask using Km (simplified symmetric encryption)
    meta.val.resize(plaintext.size());
    for (size_t i = 0; i < plaintext.size(); ++i) {
        meta.val[i] = plaintext[i] ^ Km[i % Km.size()];
    }

    // 计算 alpha = F(Ky, id)
    bn_new(meta.alpha);
    bn_t ky_copy;
    bn_new(ky_copy);
    bn_copy(ky_copy, Ky);
    computeFp(meta.alpha, ky_copy, doc_id);
    bn_free(ky_copy);

    // Paper: ODXT.Update (Figure 8) - Compute xtags
    // xtag = g^{F(K_x, w||id)}
    // MC-ODXT: xtag_i = H(w||id||i)^{K_x[I(w)]} for i=1..ℓ
    const int ell = 3;  // ℓ=3 cross-tags per insertion
    meta.xtags.resize(ell);

    std::cout << "DEBUG: Computing " << ell << " xtags for keyword=" << keyword << " doc_id=" << doc_id << std::endl;

    for (int i = 0; i < ell; ++i) {
        // Compute H(w||id||i)
        std::string xtag_input = keyword + doc_id + std::to_string(i);
        ep_t h_xtag;
        ep_new(h_xtag);
        hashToPoint(h_xtag, xtag_input);

        // Exponentiate: xtag = H(w||id||i)^{K_x[I(w)]}
        ep_t xtag;
        ep_new(xtag);
        ep_mul(xtag, h_xtag, Kx[idx]);

        // Serialize and store
        meta.xtags[i] = serializePoint(xtag);
        std::cout << "DEBUG: xtag[" << i << "] size=" << meta.xtags[i].size() << std::endl;

        ep_free(h_xtag);
        ep_free(xtag);
    }

    std::cout << "DEBUG: Before return, meta.xtags.size()=" << meta.xtags.size() << std::endl;

    ep_free(hw);
    return meta;
}

void McOdxtDataOwner::authorize(
    const std::string& user_id,
    const std::vector<std::string>& allowed_keywords,
    McOdxtGatekeeper& gatekeeper) {
    gatekeeper.grantAuthorization(m_owner_id, user_id, allowed_keywords);
}

// ============================================
// McOdxtGatekeeper 实现
// ============================================

McOdxtGatekeeper::McOdxtGatekeeper() : m_d(0) {}

McOdxtGatekeeper::~McOdxtGatekeeper() {
    // Clean up user keys
    for (auto& pair : m_user_keys) {
        if (pair.second) {
            bn_free(*pair.second);
            delete[] pair.second;
        }
    }
}

int McOdxtGatekeeper::setup(int d) {
    m_d = d;
    return 0;
}

int McOdxtGatekeeper::registerDataOwner(const std::string& owner_id) {
    if (m_owner_keys.find(owner_id) != m_owner_keys.end()) {
        return -1;  // 已存在
    }
    
    OwnerKeys keys;
    
    bn_t ord;
    bn_new(ord);
    ep_curve_get_ord(ord);
    
    // 生成所有者密钥
    bn_new(keys.Ks);
    bn_rand_mod(keys.Ks, ord);
    
    keys.Kt = new bn_t[m_d];
    for (int i = 0; i < m_d; ++i) {
        bn_new(keys.Kt[i]);
        bn_rand_mod(keys.Kt[i], ord);
    }
    
    keys.Kx = new bn_t[m_d];
    for (int i = 0; i < m_d; ++i) {
        bn_new(keys.Kx[i]);
        bn_rand_mod(keys.Kx[i], ord);
    }
    
    bn_new(keys.Ky);
    bn_rand_mod(keys.Ky, ord);
    
    keys.Km.resize(32);
    RAND_bytes(keys.Km.data(), 32);
    
    m_owner_keys[owner_id] = std::move(keys);
    bn_free(ord);
    
    return 0;
}

int McOdxtGatekeeper::registerSearchUser(const std::string& user_id) {
    if (m_user_keys.find(user_id) != m_user_keys.end()) {
        return -1;
    }

    bn_t ord;
    bn_new(ord);
    ep_curve_get_ord(ord);

    bn_t* Ks_user = new bn_t[1];
    bn_new(*Ks_user);
    bn_rand_mod(*Ks_user, ord);

    m_user_keys[user_id] = Ks_user;

    bn_free(ord);

    return 0;
}

int McOdxtGatekeeper::grantAuthorization(
    const std::string& owner_id,
    const std::string& user_id,
    const std::vector<std::string>& allowed_keywords) {
    
    Authorization auth;
    auth.owner_id = owner_id;
    auth.user_id = user_id;
    auth.allowed_keywords = allowed_keywords;
    auth.expiry = 0;  // 永不过期
    
    m_authorizations[owner_id][user_id] = auth;
    return 0;
}

int McOdxtGatekeeper::revokeAuthorization(
    const std::string& owner_id,
    const std::string& user_id) {
    
    auto owner_it = m_authorizations.find(owner_id);
    if (owner_it == m_authorizations.end()) {
        return -1;
    }
    
    owner_it->second.erase(user_id);
    return 0;
}

bool McOdxtGatekeeper::isAuthorized(
    const std::string& owner_id, 
    const std::string& user_id) const {
    
    auto owner_it = m_authorizations.find(owner_id);
    if (owner_it == m_authorizations.end()) {
        return false;
    }
    
    return owner_it->second.find(user_id) != owner_it->second.end();
}

int McOdxtGatekeeper::indexFunction(const std::string& keyword) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(keyword.c_str()),
           keyword.length(), hash);
    uint32_t index = 0;
    for (int i = 0; i < 4; ++i) {
        index = (index << 8) | hash[i];
    }
    return index % m_d;
}

void McOdxtGatekeeper::computeKz(bn_t kz, const std::string& keyword, const bn_t& Ks) {
    // 简化实现
    ep_t hwk;
    ep_new(hwk);
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(keyword.c_str()),
           keyword.length(), hash);
    ep_map(hwk, hash, SHA256_DIGEST_LENGTH);
    
    // 实际上需要: hwk = H(w)^Ks, 然后 F(hwk, "1")
    // 简化处理
    bn_t ord;
    bn_new(ord);
    ep_curve_get_ord(ord);
    
    bn_t key;
    bn_new(key);
    bn_rand_mod(key, ord);
    
    std::string input = "1" + serializePoint(hwk);
    unsigned char fp_hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.length(), fp_hash);
    bn_read_bin(kz, fp_hash, 32);
    
    ep_free(hwk);
    bn_free(ord);
    bn_free(key);
}

void McOdxtGatekeeper::computeFp(bn_t result, bn_t key, const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];

    // 将 key 转换为字符串
    int key_size = bn_size_bin(key);
    std::vector<uint8_t> key_bytes(key_size);
    bn_write_bin(key_bytes.data(), key_size, key);
    std::string key_str(reinterpret_cast<char*>(key_bytes.data()), key_size);

    std::string data = key_str + input;
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()),
           data.length(), hash);
    bn_read_bin(result, hash, 32);
}

SearchToken McOdxtGatekeeper::genToken(
    const std::vector<std::string>& query_keywords,
    const std::string& owner_id,
    const std::string& user_id) {
    
    // 1. 验证授权
    if (!isAuthorized(owner_id, user_id)) {
        throw std::runtime_error("User not authorized to search this owner's data");
    }
    
    // 2. 获取数据所有者密钥
    auto owner_it = m_owner_keys.find(owner_id);
    if (owner_it == m_owner_keys.end()) {
        throw std::runtime_error("Owner not found");
    }
    const OwnerKeys& owner_keys = owner_it->second;
    
    // 3. 获取用户密钥（用于令牌派生）
    auto user_it = m_user_keys.find(user_id);
    if (user_it == m_user_keys.end()) {
        throw std::runtime_error("User not found");
    }
    bn_t Ks_user;
    bn_new(Ks_user);
    bn_copy(Ks_user, *user_it->second);
    
    SearchToken token;
    token.owner_id = owner_id;
    token.user_id = user_id;
    
    // 4. 核心多用户令牌派生逻辑
    // strap = H(w1)^Ks_owner
    ep_t hw1;
    ep_new(hw1);
    unsigned char hash1[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(query_keywords[0].c_str()),
           query_keywords[0].length(), hash1);
    ep_map(hw1, hash1, SHA256_DIGEST_LENGTH);
    
    ep_new(token.strap);
    ep_mul(token.strap, hw1, owner_keys.Ks);  // strap = H(w1)^Ks_owner
    
    // 5. 生成 bstag, delta, bxtrap（使用所有者密钥）
    int n = query_keywords.size();
    
    // 更新计数
    auto& updateCnt = const_cast<OwnerKeys&>(owner_keys).updateCnt;
    
    for (const auto& kw : query_keywords) {
        if (updateCnt.find(kw) == updateCnt.end()) {
            updateCnt[kw] = 0;
        }
    }
    
    token.bstag.resize(n);
    token.delta.resize(n);
    token.bxtrap.resize(n);
    
    for (int j = 0; j < n; ++j) {
        const std::string& kw = query_keywords[j];
        int idx = indexFunction(kw);
        
        // bstag[j] = H(wj)^Kt[I(wj)]
        ep_t hwj, bstag;
        ep_new(hwj);
        ep_new(bstag);
        
        unsigned char hash_j[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(kw.c_str()),
               kw.length(), hash_j);
        ep_map(hwj, hash_j, SHA256_DIGEST_LENGTH);
        
        ep_mul(bstag, hwj, owner_keys.Kt[idx]);
        token.bstag[j] = serializePoint(bstag);

        // delta[j] = H(wj)^Kx[I(wj)] (简化)
        ep_t delta;
        ep_new(delta);
        ep_mul(delta, hwj, owner_keys.Kx[idx]);
        token.delta[j] = serializePoint(delta);
        
        // bxtrap[j][t] - 根据更新计数
        int cnt = updateCnt[kw];
        token.bxtrap[j].resize(cnt);
        for (int t = 0; t < cnt; ++t) {
            ep_t bxtrap;
            ep_new(bxtrap);
            
            // bxtrap = H(wj||t)^Ks_owner
            std::string xtag_input = kw + std::to_string(t);
            unsigned char xtag_hash[SHA256_DIGEST_LENGTH];
            SHA256(reinterpret_cast<const unsigned char*>(xtag_input.c_str()),
                   xtag_input.length(), xtag_hash);
            ep_map(bxtrap, xtag_hash, SHA256_DIGEST_LENGTH);
            
            ep_t result;
            ep_new(result);
            ep_mul(result, bxtrap, owner_keys.Ks);
            token.bxtrap[j][t] = serializePoint(result);
            
            ep_free(bxtrap);
            ep_free(result);
        }
        
        ep_free(hwj);
        ep_free(bstag);
        ep_free(delta);
    }
    
    // Paper: ODXT.Search (Figure 9) - Token generation
    // Token = (stag_1, ..., stag_n, xtoken_1, ..., xtoken_n)
    // MC-ODXT: strap and bxtrap already computed correctly above
    // env field is optional metadata (not used in paper's ODXT protocol)
    token.env.clear();  // Leave empty for now

    ep_free(hw1);
    bn_free(Ks_user);
    return token;
}

int McOdxtGatekeeper::getUpdateCount(
    const std::string& owner_id, 
    const std::string& keyword) const {
    
    auto owner_it = m_owner_keys.find(owner_id);
    if (owner_it == m_owner_keys.end()) {
        return 0;
    }
    
    auto it = owner_it->second.updateCnt.find(keyword);
    return (it == owner_it->second.updateCnt.end()) ? 0 : it->second;
}

const std::unordered_map<std::string, int>& McOdxtGatekeeper::getUpdateCounts(
    const std::string& owner_id) const {
    
    static std::unordered_map<std::string, int> empty;
    
    auto owner_it = m_owner_keys.find(owner_id);
    if (owner_it == m_owner_keys.end()) {
        return empty;
    }
    
    return owner_it->second.updateCnt;
}

const bn_t& McOdxtGatekeeper::getKsOwner(const std::string& owner_id) const {
    static bn_t null_key;
    bn_null(null_key);
    
    auto it = m_owner_keys.find(owner_id);
    if (it == m_owner_keys.end()) {
        return null_key;
    }
    return it->second.Ks;
}

const bn_t* McOdxtGatekeeper::getKt(const std::string& owner_id) const {
    auto it = m_owner_keys.find(owner_id);
    if (it == m_owner_keys.end()) {
        return nullptr;
    }
    return it->second.Kt;
}

const bn_t* McOdxtGatekeeper::getKx(const std::string& owner_id) const {
    auto it = m_owner_keys.find(owner_id);
    if (it == m_owner_keys.end()) {
        return nullptr;
    }
    return it->second.Kx;
}

const bn_t& McOdxtGatekeeper::getKy(const std::string& owner_id) const {
    static bn_t null_key;
    bn_null(null_key);

    auto it = m_owner_keys.find(owner_id);
    if (it == m_owner_keys.end()) {
        return null_key;
    }
    return it->second.Ky;
}

const std::vector<uint8_t>& McOdxtGatekeeper::getKm(const std::string& owner_id) const {
    static std::vector<uint8_t> empty;

    auto it = m_owner_keys.find(owner_id);
    if (it == m_owner_keys.end()) {
        return empty;
    }
    return it->second.Km;
}

// ============================================
// McOdxtServer 实现
// ============================================

McOdxtServer::McOdxtServer() {}

McOdxtServer::~McOdxtServer() {}

std::string McOdxtServer::serializePoint(const ep_t point) const {
    uint8_t bytes[256];
    int len = ep_size_bin(point, 1);
    ep_write_bin(bytes, len, point, 1);
    return std::string(reinterpret_cast<char*>(bytes), len);
}

void McOdxtServer::update(const UpdateMetadata& meta) {
    std::string addr_str = serializePoint(meta.addr);
    std::pair<std::string, std::string> key = {meta.owner_id, addr_str};
    
    TSetEntry entry;
    entry.val = meta.val;
    bn_new(entry.alpha);
    bn_copy(entry.alpha, meta.alpha);
    entry.owner_id = meta.owner_id;
    
    m_TSet[key] = entry;
    
    // 更新 XSet
    for (const auto& xtag_str : meta.xtags) {
        std::pair<std::string, std::string> xkey = {meta.owner_id, xtag_str};
        m_XSet[xkey] = true;
    }
}

std::vector<SearchResultEntry> McOdxtServer::search(
    const McOdxtClient::SearchRequest& req) {

    std::vector<SearchResultEntry> results;

    // Paper: ODXT.Search (Figure 9)
    // Phase 1: Candidate enumeration using first keyword
    // Phase 2: Cross-filtering with other keywords
    // Phase 3: Return matching results

    if (req.stokenList.empty() || req.xtokenList.empty()) {
        return results;
    }

    const std::string& owner_id = req.owner_id;
    int n = req.stokenList.size();  // Number of keywords

    // Phase 1: Enumerate candidates using first keyword's stoken
    std::vector<std::pair<std::string, TSetEntry*>> candidates;

    for (auto& kv : m_TSet) {
        if (kv.first.first == owner_id) {
            // Check if this entry matches the first keyword's stoken
            // In simplified version, we collect all entries for this owner
            candidates.push_back({kv.first.second, &kv.second});
        }
    }

    // Phase 2: Cross-filtering
    const int k = 2;  // Sample k=2 cross-tags per keyword

    for (auto& candidate : candidates) {
        bool match = true;

        // For each additional keyword (j = 1..n-1, skipping first keyword)
        for (int j = 1; j < n && match; ++j) {
            if (j >= static_cast<int>(req.xtokenList.size())) {
                match = false;
                break;
            }

            const auto& xtokens_j = req.xtokenList[j];
            if (xtokens_j.empty()) {
                match = false;
                break;
            }

            // Sample k cross-tags for this keyword
            int cnt_j = xtokens_j.size();
            int matches_found = 0;

            for (int t = 0; t < std::min(k, cnt_j); ++t) {
                if (t >= static_cast<int>(xtokens_j.size())) break;

                const auto& xtoken_list = xtokens_j[t];
                if (xtoken_list.empty()) continue;

                // Use first xtoken (simplified)
                const std::string& xtoken = xtoken_list[0];

                // Check if this xtag exists in XSet
                std::pair<std::string, std::string> xkey = {owner_id, xtoken};
                if (m_XSet.find(xkey) != m_XSet.end()) {
                    matches_found++;
                }
            }

            // Require at least one match (simplified from k matches)
            if (matches_found == 0) {
                match = false;
            }
        }

        // Phase 3: Add to results if all cross-tags matched
        if (match) {
            SearchResultEntry entry;
            entry.j = 0;
            entry.sval = candidate.second->val;
            entry.cnt = 1;
            results.push_back(entry);
        }
    }

    return results;
}

}  // namespace mcodxt
