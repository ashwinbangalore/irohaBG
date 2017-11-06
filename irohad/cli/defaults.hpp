/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IROHA_CLI_DEFAULTS_HPP_
#define IROHA_CLI_DEFAULTS_HPP_

#include <chrono>
#include "ametsuchi/config.hpp"
#include "cli/env-vars.hpp"

namespace defaults {
  using std::literals::chrono_literals::operator""ms;  // milliseconds

  constexpr auto LOCALHOST = "localhost";
  constexpr auto ALLHOST = "0.0.0.0";

  constexpr auto postgresHost = LOCALHOST;
  constexpr auto postgresPort = 6379;

  constexpr auto peerHost = LOCALHOST;
  constexpr auto peerPort = 11111;

  constexpr auto redisHost = LOCALHOST;
  constexpr auto redisPort = 5432;

  constexpr auto blockStoragePath = "blocks";

  constexpr auto toriiHost = LOCALHOST;
  constexpr auto toriiPort = 50051;

  constexpr auto loadDelay = 5000ms;
  constexpr auto voteDelay = 5000ms;
  constexpr auto proposalDelay = 5000ms;
  constexpr auto proposalSize = 10;
}
#endif  //  IROHA_CLI_DEFAULTS_HPP_
