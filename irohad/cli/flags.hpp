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

#ifndef IROHA_FLAGS_HPP_
#define IROHA_FLAGS_HPP_

#include <CLI/CLI.hpp>
#include <boost/assert.hpp>
#include "ametsuchi/config.hpp"
#include "common.hpp"
#include "env-vars.hpp"
#include "torii/config.hpp"
#include "util/network.hpp"

using std::literals::string_literals::operator""s;

#define IROHA_PORT_MIN 1
#define IROHA_PORT_MAX 65535
#define IROHA_DELAY_MIN 1
#define IROHA_DELAY_MAX 100000
#define IROHA_SIZE_MIN 1
#define IROHA_SIZE_MAX 100000

inline void addPeerFlags(CLI::App *p,
                         iroha::config::Peer *peer,
                         iroha::torii::config::Torii *torii,
                         iroha::config::Cryptography *crypto) {
  BOOST_ASSERT(p);
  BOOST_ASSERT(torii);
  BOOST_ASSERT(crypto);
  BOOST_ASSERT(peer);

  // both flags can be used for the same thing
  p->add_option("--api-host,--torii-host"s,        /* option's name */
                torii->host,                       /* bind to this variable */
                "Client API (torii) listen host"s, /* description */
                true /* use initial value as default */)
      ->envname(IROHA_TORII_HOST)
      ->group("Peer"s)
      ->check(iroha::network::is_host_valid);

  p->add_option("--api-port,--torii-port"s,
                torii->port,
                "Client API (torii) listen port"s,
                true)
      ->envname(IROHA_TORII_PORT)
      ->check(CLI::Range(IROHA_PORT_MIN, IROHA_PORT_MAX))
      ->group("Peer"s);

  p->add_option("--host"s, peer->host, "Peer's listen address"s, true)
      ->envname(IROHA_TORII_HOST)
      ->group("Peer"s)
      ->check(iroha::network::is_host_valid);

  p->add_option("--port"s, peer->port, "Peer's listen port"s, true)
      ->envname(IROHA_TORII_PORT)
      ->check(CLI::Range(IROHA_PORT_MIN, IROHA_PORT_MAX))
      ->group("Peer"s);

  p->add_option(
       "--pubkey"s, crypto->public_key, "Path to peer's public key"s, false)
      ->required()
      ->envname(IROHA_PEER_PUBKEY)
      ->group("Peer"s)
      ->check(CLI::ExistingFile);

  p->add_option(
       "--privkey"s, crypto->private_key, "Path to peer's private key"s, false)
      ->required()
      ->envname(IROHA_PEER_PRIVKEY)
      ->group("Peer"s)
      ->check(CLI::ExistingFile);
}

inline void addPostgresFlags(CLI::App *p,
                             iroha::ametsuchi::config::Postgres *postgres) {
  BOOST_ASSERT(p);
  BOOST_ASSERT(postgres);
  p->add_option(
       "--pghost"s, postgres->host, "PostgreSQL database host. "s, true)
      ->envname(IROHA_PGHOST)
      ->group("PostgreSQL"s)
      ->check(iroha::network::is_host_valid);

  p->add_option("--pgport"s, postgres->port, "PostgreSQL database port."s, true)
      ->envname(IROHA_PGPORT)
      ->check(CLI::Range(IROHA_PORT_MIN, IROHA_PORT_MAX))
      ->group("PostgreSQL"s);

  p->add_option(
       "--pgdatabase"s, postgres->database, "PostgreSQL database name"s, true)
      ->envname(IROHA_PGDATABASE)
      ->group("PostgreSQL"s);

  p->add_option("--pguser"s, postgres->username, "PostgreSQL username"s, false)
      ->required()
      ->envname(IROHA_PGUSER)
      ->group("PostgreSQL"s);

  p->add_option(
       "--pgpassword"s, postgres->password, "PostgreSQL password"s, false)
      ->required()
      ->envname(IROHA_PGPASSWORD)
      ->group("PostgreSQL"s);
}

inline void addRedisFlags(CLI::App *p, iroha::ametsuchi::config::Redis *redis) {
  BOOST_ASSERT(p);
  BOOST_ASSERT(redis);
  p->add_option("--rdhost"s, redis->host, "Redis database host"s, true)
      ->envname(IROHA_RDHOST)
      ->group("Redis"s)
      ->check(iroha::network::is_host_valid);

  p->add_option("--rdport"s, redis->port, "Redis database port"s, true)
      ->envname(IROHA_RDPORT)
      ->check(CLI::Range(IROHA_PORT_MIN, IROHA_PORT_MAX))
      ->group("Redis"s);
}

inline void addBlockStorageFlags(
    CLI::App *p, iroha::ametsuchi::config::BlockStorage *storage) {
  BOOST_ASSERT(p);
  BOOST_ASSERT(storage);
  p->add_option("--blockspath"s,
                storage->path,
                "Path to the folder, where blocks are saved",
                true)
      ->envname(IROHA_BLOCKSPATH)
      ->group("Block Storage");
}

inline void addCreateLedgerFlags(CLI::App *p, std::string *genesis) {
  BOOST_ASSERT(p);
  BOOST_ASSERT(genesis);
  p->add_option("genesis-block", *genesis, "Path to the genesis block"s)
      ->required()
      ->check(CLI::ExistingFile);
}

inline void addOtherOptionsFlags(CLI::App *p,
                                 iroha::config::OtherOptions *options) {
  BOOST_ASSERT(p);
  BOOST_ASSERT(options);

  // copy default values, otherwise they will be equal to 0
  auto load_d = static_cast<size_t>(options->load_delay.count());
  auto vote_d = static_cast<size_t>(options->vote_delay.count());
  auto proposal_d = static_cast<size_t>(options->proposal_delay.count());

  p->add_option(
       "--load-delay",
       load_d,
       "Waiting time before loading committed block from next, milliseconds"s,
       true)
      ->group("Other")
      ->check(CLI::Range(IROHA_DELAY_MIN, IROHA_DELAY_MAX))
      ->envname(IROHA_OTHER_LOADDELAY);
  options->load_delay = std::chrono::milliseconds(load_d);

  p->add_option("--vote-delay",
                vote_d,
                "Waiting time before sending vote to next peer, milliseconds"s,
                true)
      ->group("Other")
      ->check(CLI::Range(IROHA_DELAY_MIN, IROHA_DELAY_MAX))
      ->envname(IROHA_OTHER_VOTEDELAY);
  options->vote_delay = std::chrono::milliseconds(vote_d);

  p->add_option("--proposal-delay",
                proposal_d,
                "maximum waiting time util emitting new proposal"s,
                true)
      ->group("Other")
      ->check(CLI::Range(IROHA_DELAY_MIN, IROHA_DELAY_MAX))
      ->envname(IROHA_OTHER_PROPOSALDELAY);
  options->proposal_delay = std::chrono::milliseconds(proposal_d);

  p->add_option("--proposal-size",
                options->max_proposal_size,
                "Maximum transactions in one proposal"s,
                true)
      ->group("Other")
      ->check(CLI::Range(IROHA_SIZE_MIN, IROHA_SIZE_MAX))
      ->envname(IROHA_OTHER_PROPOSALSIZE);
}

#endif  //  IROHA_FLAGS_HPP_
