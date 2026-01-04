#pragma once

#include <gmpxx.h>

#include <string>

extern "C" {
#include <relic/relic.h>
};

void Hash_H1(ep_t out, const std::string& in);

void Hash_H2(ep_t out, const std::string& in);

void Hash_G1(ep_t out, const std::string& in);

void Hash_G2(ep_t out, const std::string& in);

void Hash_G2(ep2_t out, const std::string& in);

void Hash_Zn(bn_t out, const std::string& in);
