#pragma once

#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include <relic/relic.h>
}

namespace mcodxt {

enum class OpType : uint8_t {
    ADD = 0,
    DEL = 1
};

struct UpdateMetadata {
    ep_t addr;
    std::vector<uint8_t> val;
    bn_t alpha;
    std::vector<std::string> xtags;

    UpdateMetadata() {
        ep_null(addr);
        bn_null(alpha);
    }

    UpdateMetadata(UpdateMetadata&& other) noexcept
        : val(std::move(other.val)),
          xtags(std::move(other.xtags)) {
        std::memcpy(addr, other.addr, sizeof(ep_t));
        std::memcpy(alpha, other.alpha, sizeof(bn_t));
        ep_null(other.addr);
        bn_null(other.alpha);
    }

    UpdateMetadata& operator=(UpdateMetadata&& other) noexcept {
        if (this != &other) {
            ep_free(addr);
            bn_free(alpha);
            val = std::move(other.val);
            xtags = std::move(other.xtags);
            std::memcpy(addr, other.addr, sizeof(ep_t));
            std::memcpy(alpha, other.alpha, sizeof(bn_t));
            ep_null(other.addr);
            bn_null(other.alpha);
        }
        return *this;
    }

    ~UpdateMetadata() {
        ep_free(addr);
        bn_free(alpha);
    }

    UpdateMetadata(const UpdateMetadata&) = delete;
    UpdateMetadata& operator=(const UpdateMetadata&) = delete;
};

struct SearchToken {
    ep_t strap;
    std::vector<std::string> bstag;
    std::vector<std::string> delta;
    std::vector<std::vector<std::string>> bxtrap;

    SearchToken() {
        ep_null(strap);
    }

    SearchToken(SearchToken&& other) noexcept
        : bstag(std::move(other.bstag)),
          delta(std::move(other.delta)),
          bxtrap(std::move(other.bxtrap)) {
        std::memcpy(strap, other.strap, sizeof(ep_t));
        ep_null(other.strap);
    }

    SearchToken& operator=(SearchToken&& other) noexcept {
        if (this != &other) {
            ep_free(strap);
            bstag = std::move(other.bstag);
            delta = std::move(other.delta);
            bxtrap = std::move(other.bxtrap);
            std::memcpy(strap, other.strap, sizeof(ep_t));
            ep_null(other.strap);
        }
        return *this;
    }

    ~SearchToken() {
        ep_free(strap);
    }

    SearchToken(const SearchToken&) = delete;
    SearchToken& operator=(const SearchToken&) = delete;
};

struct SearchResultEntry {
    int j;
    std::vector<uint8_t> sval;
    int cnt;
};

struct TSetEntry {
    std::vector<uint8_t> val;
    bn_t alpha;

    TSetEntry() {
        bn_null(alpha);
    }

    TSetEntry(TSetEntry&& other) noexcept
        : val(std::move(other.val)) {
        std::memcpy(alpha, other.alpha, sizeof(bn_t));
        bn_null(other.alpha);
    }

    TSetEntry& operator=(TSetEntry&& other) noexcept {
        if (this != &other) {
            bn_free(alpha);
            val = std::move(other.val);
            std::memcpy(alpha, other.alpha, sizeof(bn_t));
            bn_null(other.alpha);
        }
        return *this;
    }

    ~TSetEntry() {
        bn_free(alpha);
    }

    TSetEntry(const TSetEntry&) = delete;
    TSetEntry& operator=(const TSetEntry&) = delete;
};

class McOdxtGatekeeper;

class McOdxtClient {
public:
    McOdxtClient();
    ~McOdxtClient();

    int setup();

    SearchToken genTokenSimplified(
        const std::vector<std::string>& query_keywords,
        McOdxtGatekeeper& gatekeeper);

    struct SearchRequest {
        std::vector<std::string> stokenList;
        std::vector<std::vector<std::vector<std::string>>> xtokenList;
    };

    SearchRequest prepareSearch(
        const SearchToken& token,
        const std::vector<std::string>& query_keywords,
        const std::unordered_map<std::string, int>& updateCnt);

    std::vector<std::string> decryptResults(
        const std::vector<SearchResultEntry>& results,
        const SearchToken& token);
};

class McOdxtGatekeeper {
public:
    McOdxtGatekeeper();
    ~McOdxtGatekeeper();

    int setup(int d = 10);

    UpdateMetadata update(
        OpType op,
        const std::string& id,
        const std::string& keyword);

    int getUpdateCount(const std::string& keyword) const;

    const std::unordered_map<std::string, int>& getUpdateCounts() const {
        return m_updateCnt;
    }

    void setUpdateCountForBenchmark(const std::string& keyword, int count);

    const std::vector<uint8_t>& getKm() const {
        return m_Km;
    }

    SearchToken genTokenSimplified(
        const std::vector<std::string>& query_keywords);

private:
    bn_t m_Ks;
    bn_t* m_Kt;
    bn_t* m_Kx;
    bn_t m_Ky;
    std::vector<uint8_t> m_Km;
    std::unordered_map<std::string, int> m_updateCnt;
    int m_d;

    int indexFunction(const std::string& keyword) const;
    std::string computeKz(const std::string& keyword);
    void computeF_p(
        bn_t result,
        const bn_t key,
        const std::string& input);
    void computeF_p(
        bn_t result,
        const std::string& key,
        const std::string& input);
};

class McOdxtServer {
public:
    McOdxtServer();
    ~McOdxtServer();

    void setup(const std::vector<uint8_t>& Km);
    void update(const UpdateMetadata& meta);
    std::vector<SearchResultEntry> search(
        const McOdxtClient::SearchRequest& req);

    size_t getTSetSize() const { return m_TSet.size(); }
    size_t getXSetSize() const { return m_XSet.size(); }

private:
    std::map<std::string, TSetEntry> m_TSet;
    std::map<std::string, bool> m_XSet;

    std::string serializePoint(const ep_t point) const;
};

}  // namespace mcodxt
