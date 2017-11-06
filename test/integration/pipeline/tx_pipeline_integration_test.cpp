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

#include <boost/filesystem.hpp>
#include <cstdio>
#include <cstring>
#include "ametsuchi/config.hpp"
#include "cli/env-vars.hpp"
#include "crypto/hash.hpp"
#include "crypto/keys_manager_impl.hpp"
#include "datetime/time.hpp"
#include "framework/test_subscriber.hpp"
#include "main/application.hpp"
#include "main/raw_block_insertion.hpp"
#include "model/generators/block_generator.hpp"
#include "module/irohad/ametsuchi/ametsuchi_fixture.hpp"
#include "torii/config.hpp"
#include "util/string.hpp"

using iroha::string::parse_env;
using namespace framework::test_subscriber;
using namespace std::chrono_literals;
using ToriiConfig = iroha::torii::config::Torii;
using AmetsuchiConfig = iroha::ametsuchi::config::Ametsuchi;

class TestIrohad : public Application {
 public:
  TestIrohad(const iroha::ametsuchi::config::Ametsuchi &am,
             const iroha::torii::config::Torii &t,
             const iroha::config::Peer &p,
             const iroha::config::OtherOptions &o,
             const iroha::keypair_t kp)
      : Application(am, t, p, o, kp) {}

  auto &getCommandService() { return command_service; }

  auto &getPeerCommunicationService() { return pcs; }

  auto &getCryptoProvider() { return crypto_verifier; }

  void run() override {
    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort(
        peer_.listenAddress(), grpc::InsecureServerCredentials(), &port);

    builder.RegisterService(ordering_init.ordering_gate_transport.get());
    builder.RegisterService(ordering_init.ordering_service_transport.get());
    builder.RegisterService(yac_init.consensus_network.get());
    builder.RegisterService(loader_init.service.get());
    internal_server = builder.BuildAndStart();
    log_->info("===> iroha initialized");
  }
};

using namespace iroha;

class TxPipelineIntegrationTest : public iroha::ametsuchi::AmetsuchiTest {
 public:
  TxPipelineIntegrationTest() {
    // spdlog::set_level(spdlog::level::off);
  }

  ToriiConfig torii{};
  config::Peer peer{};
  config::OtherOptions other{};

  void SetUp() override {
    iroha::ametsuchi::AmetsuchiTest::SetUp();

    {
      peer.host = parse_env(IROHA_PEER_HOST, LOCALHOST);
      peer.port = parse_env(IROHA_PEER_PORT, defaults::peerPort);

      torii.host = parse_env(IROHA_TORII_HOST, LOCALHOST);
      torii.port = parse_env(IROHA_TORII_PORT, defaults::toriiPort);

      auto load_d = parse_env(IROHA_OTHER_LOADDELAY, 5000);
      auto vote_d = parse_env(IROHA_OTHER_VOTEDELAY, 5000);
      auto proposal_d = parse_env(IROHA_OTHER_PROPOSALDELAY, 5000);

      other.load_delay = std::chrono::milliseconds(load_d);
      other.vote_delay = std::chrono::milliseconds(vote_d);
      other.proposal_delay = std::chrono::milliseconds(proposal_d);
      other.max_proposal_size = parse_env(IROHA_OTHER_PROPOSALSIZE, 10);
    }

    genesis_block =
        iroha::model::generators::BlockGenerator().generateGenesisBlock(
            {"0.0.0.0:10001"});
    manager = std::make_shared<iroha::KeysManagerImpl>("node0");
    auto keypair = manager->loadKeys().value();

    irohad = std::make_shared<TestIrohad>(config, torii, peer, other, keypair);

    ASSERT_TRUE(irohad->storage);

    // insert genesis block
    iroha::main::BlockInserter inserter(irohad->storage);
    inserter.applyToLedger({genesis_block});

    // start irohad
    irohad->init();
    irohad->run();
  }

  void TearDown() override {
    iroha::ametsuchi::AmetsuchiTest::TearDown();
    std::remove("node0.pub");
    std::remove("node0.priv");
    std::remove("admin@test.pub");
    std::remove("admin@test.priv");
    std::remove("test@test.pub");
    std::remove("test@test.priv");
  }

  void sendTransactions(std::vector<iroha::model::Transaction> transactions) {
    // generate expected proposal
    expected_proposal = std::make_shared<iroha::model::Proposal>(transactions);
    expected_proposal->height = 2;

    // generate expected block
    expected_block = iroha::model::Block{};
    expected_block.height = expected_proposal->height;
    expected_block.prev_hash = genesis_block.hash;
    expected_block.transactions = transactions;
    expected_block.txs_number = transactions.size();
    expected_block.created_ts = 0;
    expected_block.merkle_root.fill(0);
    expected_block.hash = iroha::hash(expected_block);
    irohad->getCryptoProvider()->sign(expected_block);

    // send transactions to torii
    for (const auto &tx : transactions) {
      auto pb_tx =
          iroha::model::converters::PbTransactionFactory().serialize(tx);

      google::protobuf::Empty response;
      irohad->getCommandService()->ToriiAsync(pb_tx, response);
    }
  }

  void validate() {
    // verify proposal
    auto proposal_wrapper = make_test_subscriber<CallExact>(
        irohad->getPeerCommunicationService()->on_proposal(), 1);
    proposal_wrapper.subscribe(
        [this](auto proposal) { proposals.push_back(proposal); });

    // verify commit and block
    auto commit_wrapper = make_test_subscriber<CallExact>(
        irohad->getPeerCommunicationService()->on_commit(), 1);
    commit_wrapper.subscribe([this](auto commit) {
      auto block_wrapper = make_test_subscriber<CallExact>(commit, 1);
      block_wrapper.subscribe([this](auto block) { blocks.push_back(block); });
    });
    irohad->getPeerCommunicationService()->on_commit().subscribe(
        [this](auto) { cv.notify_one(); });

    // wait for commit
    std::unique_lock<std::mutex> lk(m);

    cv.wait_for(lk, 10s, [this] { return blocks.size() == 1; });

    ASSERT_TRUE(proposal_wrapper.validate());
    ASSERT_EQ(1, proposals.size());
    ASSERT_EQ(*expected_proposal, proposals.front());

    ASSERT_TRUE(commit_wrapper.validate());
    ASSERT_EQ(1, blocks.size());
    ASSERT_EQ(expected_block, blocks.front());
  }

  std::shared_ptr<TestIrohad> irohad;

  std::condition_variable cv;
  std::mutex m;

  std::shared_ptr<iroha::model::Proposal> expected_proposal;
  iroha::model::Block genesis_block, expected_block;

  std::vector<iroha::model::Proposal> proposals;
  std::vector<iroha::model::Block> blocks;

  std::shared_ptr<iroha::KeysManager> manager;
};

TEST_F(TxPipelineIntegrationTest, TxPipelineTest) {
  // generate test command
  auto cmd =
      iroha::model::generators::CommandGenerator().generateAddAssetQuantity(
          "admin@test",
          "coin#test",
          iroha::Amount().createFromString("20.00").value());

  // generate test transaction
  auto tx =
      iroha::model::generators::TransactionGenerator().generateTransaction(
          "admin@test", 1, {cmd});
  iroha::KeysManagerImpl manager("admin@test");
  auto keypair = manager.loadKeys().value();
  iroha::model::ModelCryptoProviderImpl provider(keypair);
  provider.sign(tx);

  sendTransactions({tx});

  validate();
}
