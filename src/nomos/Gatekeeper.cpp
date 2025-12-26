#include "nomos/Gatekeeper.hpp"

#include "relic/relic_ep.h"
extern "C" {
#include <openssl/rand.h>
}
using namespace nomos;

GateKeeper::GateKeeper() {
  // Constructor implementation (if any)
  bn_new(Ks);
  bn_new(Ky);
  bn_new(Km);
  state = std::make_unique<GatekeeperState>();
}

GateKeeper::~GateKeeper() {
  // Destructor implementation (if any)
  bn_free(Ks);
  bn_free(Ky);
  bn_free(Km);
}

int GateKeeper::Setup() {
  // Setup implementation
  bn_t ord;  // 大数类型，椭圆曲线的阶

  bn_new(ord);
  ep_curve_get_ord(ord);  // 获取当前椭圆曲线的阶
  bn_rand_mod(Ks, ord);
  bn_rand_mod(Ky, ord);
  bn_rand_mod(Km, ord);

  this->state->Clear();

  bn_clean(ord);
  return 0;  // Return 0 on success
}

Metadata GateKeeper::Update(OP op, const std::string& id,
                            const std::string& keyword) {
  return Metadata();
}