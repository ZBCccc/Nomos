#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mc-odxt/McOdxtClient.hpp"
#include "mc-odxt/McOdxtGatekeeper.hpp"
#include "mc-odxt/McOdxtServer.hpp"
#include "nomos/Client.hpp"
#include "nomos/Gatekeeper.hpp"
#include "nomos/Server.hpp"
#include "vq-nomos/Client.hpp"
#include "vq-nomos/Gatekeeper.hpp"
#include "vq-nomos/Server.hpp"

extern "C" {
#include <relic/relic.h>
}

namespace {

struct UpdateCase {
  std::string doc_id;
  std::string keyword;
  bool is_add;
};

struct SchemeOutcome {
  bool accepted;
  std::vector<std::string> ids;
};

struct QueryExpectation {
  std::vector<std::string> query;
  std::vector<std::string> expected_ids;
};

struct ReferenceState {
  std::map<std::string, std::map<std::string, int> > net_count_by_keyword;
  std::map<std::string, int> total_updates_by_keyword;
  std::map<std::string, std::set<std::string> > active_docs_by_keyword;
  std::map<std::string, std::set<std::string> > active_keywords_by_doc;
};

struct LargeScaleWorkload {
  std::vector<UpdateCase> updates;
  std::vector<QueryExpectation> expectations;
  size_t qtree_capacity;

  LargeScaleWorkload() : qtree_capacity(1024) {}
};

std::vector<std::string> sorted(std::vector<std::string> ids) {
  std::sort(ids.begin(), ids.end());
  return ids;
}

size_t recommendQTreeCapacity(size_t update_count) {
  if (update_count >= 10000) {
    return static_cast<size_t>(1) << 20;  // 1,048,576
  }
  if (update_count >= 1000) {
    return static_cast<size_t>(1) << 18;  // 262,144
  }
  return 1024;
}

std::vector<std::vector<std::string> > buildDocKeywordPool(size_t doc_count) {
  std::vector<std::vector<std::string> > keyword_pool(doc_count);
  for (size_t doc_index = 0; doc_index < doc_count; ++doc_index) {
    std::vector<std::string> keywords;
    std::stringstream ss_topic;
    ss_topic << "topic_" << (doc_index % 24);
    keywords.push_back(ss_topic.str());

    std::stringstream ss_bucket;
    ss_bucket << "bucket_" << ((doc_index / 2) % 40);
    keywords.push_back(ss_bucket.str());

    std::stringstream ss_band;
    ss_band << "band_" << ((doc_index / 7) % 16);
    keywords.push_back(ss_band.str());

    std::stringstream ss_ring;
    ss_ring << "ring_" << ((doc_index * 17) % 64);
    keywords.push_back(ss_ring.str());

    keyword_pool[doc_index] = keywords;
  }
  return keyword_pool;
}

std::string makePairKey(const std::string& doc_id, const std::string& keyword) {
  return doc_id + "\n" + keyword;
}

ReferenceState buildReferenceState(const std::vector<UpdateCase>& updates) {
  ReferenceState state;
  for (size_t i = 0; i < updates.size(); ++i) {
    const UpdateCase& update = updates[i];
    const int delta = update.is_add ? 1 : -1;
    state.total_updates_by_keyword[update.keyword]++;
    state.net_count_by_keyword[update.keyword][update.doc_id] += delta;
  }

  for (std::map<std::string, std::map<std::string, int> >::const_iterator
           keyword_it = state.net_count_by_keyword.begin();
       keyword_it != state.net_count_by_keyword.end(); ++keyword_it) {
    for (std::map<std::string, int>::const_iterator doc_it =
             keyword_it->second.begin();
         doc_it != keyword_it->second.end(); ++doc_it) {
      if (doc_it->second <= 0) {
        continue;
      }
      state.active_docs_by_keyword[keyword_it->first].insert(doc_it->first);
      state.active_keywords_by_doc[doc_it->first].insert(keyword_it->first);
    }
  }
  return state;
}

std::vector<std::string> evaluateReferenceQuery(
    const ReferenceState& state, const std::vector<std::string>& query) {
  if (query.empty()) {
    return std::vector<std::string>();
  }

  std::map<std::string, std::set<std::string> >::const_iterator keyword_it =
      state.active_docs_by_keyword.find(query[0]);
  if (keyword_it == state.active_docs_by_keyword.end()) {
    return std::vector<std::string>();
  }

  std::set<std::string> intersection = keyword_it->second;
  for (size_t i = 1; i < query.size(); ++i) {
    keyword_it = state.active_docs_by_keyword.find(query[i]);
    if (keyword_it == state.active_docs_by_keyword.end()) {
      return std::vector<std::string>();
    }

    std::set<std::string> next_intersection;
    std::set_intersection(intersection.begin(), intersection.end(),
                          keyword_it->second.begin(), keyword_it->second.end(),
                          std::inserter(next_intersection,
                                        next_intersection.begin()));
    intersection.swap(next_intersection);
    if (intersection.empty()) {
      break;
    }
  }

  return std::vector<std::string>(intersection.begin(), intersection.end());
}

std::vector<std::string> selectSingleKeywordQuery(const ReferenceState& state) {
  std::vector<std::string> candidates;
  for (std::map<std::string, std::set<std::string> >::const_iterator it =
           state.active_docs_by_keyword.begin();
       it != state.active_docs_by_keyword.end(); ++it) {
    if (!it->second.empty()) {
      candidates.push_back(it->first);
    }
  }
  if (candidates.empty()) {
    throw std::runtime_error("No active keyword available for large-scale test");
  }

  std::sort(candidates.begin(), candidates.end(),
            [&state](const std::string& lhs, const std::string& rhs) {
              const int lhs_updates =
                  state.total_updates_by_keyword.find(lhs)->second;
              const int rhs_updates =
                  state.total_updates_by_keyword.find(rhs)->second;
              if (lhs_updates != rhs_updates) {
                return lhs_updates < rhs_updates;
              }
              return lhs < rhs;
            });

  return std::vector<std::string>(
      1, candidates[static_cast<size_t>(candidates.size() / 4)]);
}

std::vector<std::string> selectIntersectionQuery(const ReferenceState& state,
                                                 size_t arity) {
  std::vector<std::string> best_query;
  int best_cost = std::numeric_limits<int>::max();
  bool found_strict_order = false;

  for (std::map<std::string, std::set<std::string> >::const_iterator doc_it =
           state.active_keywords_by_doc.begin();
       doc_it != state.active_keywords_by_doc.end(); ++doc_it) {
    if (doc_it->second.size() < arity) {
      continue;
    }

    std::vector<std::string> keywords(doc_it->second.begin(), doc_it->second.end());
    std::sort(keywords.begin(), keywords.end(),
              [&state](const std::string& lhs, const std::string& rhs) {
                const int lhs_updates =
                    state.total_updates_by_keyword.find(lhs)->second;
                const int rhs_updates =
                    state.total_updates_by_keyword.find(rhs)->second;
                if (lhs_updates != rhs_updates) {
                  return lhs_updates > rhs_updates;
                }
                return lhs < rhs;
              });

    std::vector<std::string> candidate(keywords.begin(),
                                       keywords.begin() + arity);
    const int max_updates =
        state.total_updates_by_keyword.find(candidate.front())->second;
    const int min_updates =
        state.total_updates_by_keyword.find(candidate.back())->second;
    const bool strict_order = max_updates > min_updates;
    const int candidate_cost = min_updates;

    if (strict_order) {
      if (!found_strict_order || candidate_cost < best_cost) {
        best_query = candidate;
        best_cost = candidate_cost;
        found_strict_order = true;
      }
      continue;
    }

    if (!found_strict_order && candidate_cost < best_cost) {
      best_query = candidate;
      best_cost = candidate_cost;
    }
  }

  if (best_query.empty()) {
    throw std::runtime_error("No intersecting keyword set available");
  }
  return best_query;
}

std::vector<std::string> selectEmptyQuery(const ReferenceState& state,
                                          size_t update_count) {
  std::vector<std::string> query = selectSingleKeywordQuery(state);
  std::stringstream ss;
  ss << "__absent_keyword_" << update_count << "__";
  query.push_back(ss.str());
  return query;
}

LargeScaleWorkload buildLargeScaleWorkload(size_t update_count, uint32_t seed) {
  LargeScaleWorkload workload;
  workload.qtree_capacity = recommendQTreeCapacity(update_count);

  const size_t doc_count =
      std::max(static_cast<size_t>(96),
               std::min(static_cast<size_t>(1200), update_count / 6));
  const std::vector<std::vector<std::string> > keyword_pool =
      buildDocKeywordPool(doc_count);

  std::unordered_map<std::string, int> pair_net_count;
  std::vector<std::string> doc_ids(doc_count);
  for (size_t doc_index = 0; doc_index < doc_count; ++doc_index) {
    std::stringstream ss;
    ss << "doc" << doc_index;
    doc_ids[doc_index] = ss.str();
  }

  workload.updates.reserve(update_count);
  for (size_t doc_index = 0; doc_index < doc_count &&
                             workload.updates.size() < update_count;
       ++doc_index) {
    for (size_t keyword_index = 0; keyword_index < 2 &&
                                   workload.updates.size() < update_count;
         ++keyword_index) {
      const UpdateCase update = {doc_ids[doc_index],
                                 keyword_pool[doc_index][keyword_index], true};
      workload.updates.push_back(update);
      pair_net_count[makePairKey(update.doc_id, update.keyword)]++;
    }

    if ((doc_index % 3) == 0 && workload.updates.size() < update_count) {
      const UpdateCase update = {doc_ids[doc_index], keyword_pool[doc_index][2],
                                 true};
      workload.updates.push_back(update);
      pair_net_count[makePairKey(update.doc_id, update.keyword)]++;
    }
  }

  std::mt19937 rng(seed);
  while (workload.updates.size() < update_count) {
    const size_t doc_index = static_cast<size_t>(rng() % doc_count);
    const std::vector<std::string>& keywords = keyword_pool[doc_index];
    const size_t keyword_index =
        static_cast<size_t>((rng() + workload.updates.size()) % keywords.size());

    UpdateCase update;
    update.doc_id = doc_ids[doc_index];
    update.keyword = keywords[keyword_index];

    const std::string pair_key = makePairKey(update.doc_id, update.keyword);
    const int current_net_count = pair_net_count[pair_key];
    update.is_add =
        (current_net_count == 0) || ((static_cast<int>(rng() % 5)) != 0);

    workload.updates.push_back(update);
    pair_net_count[pair_key] += update.is_add ? 1 : -1;
  }

  const ReferenceState state = buildReferenceState(workload.updates);

  QueryExpectation single_query;
  single_query.query = selectSingleKeywordQuery(state);
  single_query.expected_ids = evaluateReferenceQuery(state, single_query.query);
  workload.expectations.push_back(single_query);

  QueryExpectation pair_query;
  pair_query.query = selectIntersectionQuery(state, 2);
  pair_query.expected_ids = evaluateReferenceQuery(state, pair_query.query);
  workload.expectations.push_back(pair_query);

  QueryExpectation triple_query;
  triple_query.query = selectIntersectionQuery(state, 3);
  triple_query.expected_ids = evaluateReferenceQuery(state, triple_query.query);
  workload.expectations.push_back(triple_query);

  QueryExpectation empty_query;
  empty_query.query = selectEmptyQuery(state, update_count);
  empty_query.expected_ids = evaluateReferenceQuery(state, empty_query.query);
  workload.expectations.push_back(empty_query);

  return workload;
}

class NomosHarness {
 public:
  explicit NomosHarness(const std::vector<UpdateCase>& updates) {
    if (m_gatekeeper.setup(10) != 0) {
      throw std::runtime_error("Nomos gatekeeper setup failed");
    }
    if (m_client.setup() != 0) {
      throw std::runtime_error("Nomos client setup failed");
    }
    m_server.setup(m_gatekeeper.getKm());

    for (size_t i = 0; i < updates.size(); ++i) {
      const nomos::OP op = updates[i].is_add ? nomos::OP_ADD : nomos::OP_DEL;
      m_server.update(
          m_gatekeeper.update(op, updates[i].doc_id, updates[i].keyword));
    }
  }

  SchemeOutcome search(const std::vector<std::string>& query) {
    const nomos::TokenRequest token_request =
        m_client.genToken(query, m_gatekeeper.getUpdateCounts());
    const nomos::SearchToken token = m_gatekeeper.genToken(token_request);
    const nomos::Client::SearchRequest request =
        m_client.prepareSearch(token, token_request);
    const std::vector<nomos::SearchResultEntry> results =
        m_server.search(request);

    SchemeOutcome outcome;
    outcome.accepted = true;
    outcome.ids = sorted(m_client.decryptResults(results, token));
    return outcome;
  }

 private:
  nomos::Gatekeeper m_gatekeeper;
  nomos::Client m_client;
  nomos::Server m_server;
};

class McOdxtHarness {
 public:
  explicit McOdxtHarness(const std::vector<UpdateCase>& updates) {
    if (m_gatekeeper.setup(10) != 0) {
      throw std::runtime_error("MC-ODXT gatekeeper setup failed");
    }
    if (m_client.setup() != 0) {
      throw std::runtime_error("MC-ODXT client setup failed");
    }
    m_server.setup(m_gatekeeper.getKm());

    for (size_t i = 0; i < updates.size(); ++i) {
      const mcodxt::OpType op =
          updates[i].is_add ? mcodxt::OpType::ADD : mcodxt::OpType::DEL;
      m_server.update(
          m_gatekeeper.update(op, updates[i].doc_id, updates[i].keyword));
    }
  }

  SchemeOutcome search(const std::vector<std::string>& query) {
    const mcodxt::TokenRequest token_request =
        m_client.genToken(query, m_gatekeeper.getUpdateCounts());
    const mcodxt::SearchToken token = m_gatekeeper.genToken(token_request);
    const mcodxt::McOdxtClient::SearchRequest request =
        m_client.prepareSearch(token, token_request);
    const std::vector<mcodxt::SearchResultEntry> results =
        m_server.search(request);

    SchemeOutcome outcome;
    outcome.accepted = true;
    outcome.ids = sorted(m_client.decryptResults(results, token));
    return outcome;
  }

 private:
  mcodxt::McOdxtGatekeeper m_gatekeeper;
  mcodxt::McOdxtClient m_client;
  mcodxt::McOdxtServer m_server;
};

class VQNomosHarness {
 public:
  VQNomosHarness(const std::vector<UpdateCase>& updates, size_t qtree_capacity) {
    if (m_gatekeeper.setup(10, qtree_capacity) != 0) {
      throw std::runtime_error("VQNomos gatekeeper setup failed");
    }
    const vqnomos::Anchor initial_anchor = m_gatekeeper.getCurrentAnchor();
    if (m_client.setup(m_gatekeeper.getPublicKeyPem(), initial_anchor,
                       qtree_capacity, 10) != 0) {
      throw std::runtime_error("VQNomos client setup failed");
    }
    m_server.setup(m_gatekeeper.getKm(), initial_anchor, qtree_capacity);

    for (size_t i = 0; i < updates.size(); ++i) {
      const vqnomos::OP op =
          updates[i].is_add ? vqnomos::OP_ADD : vqnomos::OP_DEL;
      m_server.update(
          m_gatekeeper.update(op, updates[i].doc_id, updates[i].keyword));
    }
  }

  SchemeOutcome search(const std::vector<std::string>& query) {
    const vqnomos::TokenRequest token_request =
        m_client.genToken(query, m_gatekeeper.getUpdateCounts());
    const vqnomos::SearchToken token = m_gatekeeper.genToken(token_request);
    const vqnomos::SearchRequest request =
        m_client.prepareSearch(token, token_request);
    const vqnomos::SearchResponse response = m_server.search(request, token);
    const vqnomos::VerificationResult verification =
        m_client.decryptAndVerify(response, token, token_request);

    SchemeOutcome outcome;
    outcome.accepted = verification.accepted;
    outcome.ids = sorted(verification.ids);
    return outcome;
  }

 private:
  vqnomos::Gatekeeper m_gatekeeper;
  vqnomos::Client m_client;
  vqnomos::Server m_server;
};

void expectAllSchemesMatch(
    const std::vector<UpdateCase>& updates,
    const std::vector<QueryExpectation>& expectations,
    size_t qtree_capacity = 1024) {
  NomosHarness nomos(updates);
  McOdxtHarness mc_odxt(updates);
  VQNomosHarness vqnomos(updates, qtree_capacity);

  for (size_t query_index = 0; query_index < expectations.size();
       ++query_index) {
    const QueryExpectation& expectation = expectations[query_index];

    std::string query_label;
    for (size_t i = 0; i < expectation.query.size(); ++i) {
      if (i > 0) {
        query_label += ",";
      }
      query_label += expectation.query[i];
    }
    SCOPED_TRACE("query[" + std::to_string(query_index) + "]=" + query_label);

    const SchemeOutcome nomos_outcome = nomos.search(expectation.query);
    const SchemeOutcome mc_odxt_outcome = mc_odxt.search(expectation.query);
    const SchemeOutcome vqnomos_outcome = vqnomos.search(expectation.query);

    const std::vector<std::string> expected = sorted(expectation.expected_ids);

    EXPECT_TRUE(nomos_outcome.accepted);
    EXPECT_TRUE(mc_odxt_outcome.accepted);
    EXPECT_TRUE(vqnomos_outcome.accepted);

    EXPECT_EQ(nomos_outcome.ids, expected);
    EXPECT_EQ(mc_odxt_outcome.ids, expected);
    EXPECT_EQ(vqnomos_outcome.ids, expected);

    EXPECT_EQ(nomos_outcome.ids, mc_odxt_outcome.ids);
    EXPECT_EQ(nomos_outcome.ids, vqnomos_outcome.ids);
  }
}

}  // namespace

class ThreeSchemeCorrectnessTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if (core_get() == NULL) {
      if (core_init() != RLC_OK) {
        FAIL() << "Failed to initialize RELIC";
      }
      if (pc_param_set_any() != RLC_OK) {
        core_clean();
        FAIL() << "Failed to set pairing parameters";
      }
    }
  }
};

TEST_F(ThreeSchemeCorrectnessTest,
       RepresentativeQueriesProduceSameResultsAcrossSchemes) {
  const std::vector<UpdateCase> updates = {
      {"doc1", "crypto", true},   {"doc1", "security", true},
      {"doc2", "crypto", true},   {"doc2", "privacy", true},
      {"doc3", "crypto", true},   {"doc4", "privacy", true}};

  std::vector<QueryExpectation> expectations;
  QueryExpectation single_crypto;
  single_crypto.query = std::vector<std::string>(1, "crypto");
  single_crypto.expected_ids = std::vector<std::string>{"doc1", "doc2", "doc3"};
  expectations.push_back(single_crypto);

  QueryExpectation pair_query;
  pair_query.query = std::vector<std::string>{"crypto", "security"};
  pair_query.expected_ids = std::vector<std::string>{"doc1"};
  expectations.push_back(pair_query);

  QueryExpectation single_privacy;
  single_privacy.query = std::vector<std::string>(1, "privacy");
  single_privacy.expected_ids = std::vector<std::string>{"doc2", "doc4"};
  expectations.push_back(single_privacy);

  QueryExpectation empty_query;
  empty_query.query = std::vector<std::string>{"crypto", "nonexistent"};
  expectations.push_back(empty_query);

  expectAllSchemesMatch(updates, expectations);
}

TEST_F(ThreeSchemeCorrectnessTest,
       DeleteAndReaddSemanticsProduceSameResultsAcrossSchemes) {
  const std::vector<UpdateCase> updates = {
      {"doc1", "crypto", true},   {"doc2", "crypto", true},
      {"doc1", "crypto", false},  {"doc1", "crypto", true},
      {"doc3", "crypto", true},   {"doc3", "crypto", false},
      {"doc1", "security", true}, {"doc2", "security", true},
      {"doc1", "security", false}};

  std::vector<QueryExpectation> expectations;
  QueryExpectation single_query;
  single_query.query = std::vector<std::string>(1, "crypto");
  single_query.expected_ids = std::vector<std::string>{"doc1", "doc2"};
  expectations.push_back(single_query);

  QueryExpectation pair_query;
  pair_query.query = std::vector<std::string>{"crypto", "security"};
  pair_query.expected_ids = std::vector<std::string>{"doc2"};
  expectations.push_back(pair_query);

  expectAllSchemesMatch(updates, expectations);
}

TEST_F(ThreeSchemeCorrectnessTest,
       LargeScale1000UpdatesProduceSameResultsAcrossSchemes) {
  const LargeScaleWorkload workload =
      buildLargeScaleWorkload(1000, 20260315u);
  ASSERT_EQ(workload.updates.size(), 1000u);
  ASSERT_EQ(workload.expectations.size(), 4u);
  expectAllSchemesMatch(workload.updates, workload.expectations,
                        workload.qtree_capacity);
}

TEST_F(ThreeSchemeCorrectnessTest,
       LargeScale10000UpdatesProduceSameResultsAcrossSchemes) {
  const LargeScaleWorkload workload =
      buildLargeScaleWorkload(10000, 20260316u);
  ASSERT_EQ(workload.updates.size(), 10000u);
  ASSERT_EQ(workload.expectations.size(), 4u);
  expectAllSchemesMatch(workload.updates, workload.expectations,
                        workload.qtree_capacity);
}
