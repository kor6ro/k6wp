// engine_logic_dump.cpp - the G5 golden fixtures for the engine's PURE logic.
//
// It writes (or checks) tests/golden/engine_logic.json: the frozen output of
// the engine's pure decisions on a fixed input table. The C++ side checks it
// (the --check mode, wired as the ctest "engine_logic_test") and the Rust
// side replays it (k6wp-core's engine_logic golden test), so a port that
// differs in any way fails on both sides.
//
// Usage:
//   engine_logic_dump <path>            write the fixture
//   engine_logic_dump --check <path>    compare against the fixture
//
// The case table and the evaluator live in engine_logic_cases.hpp so the
// writer and the checker cannot drift apart.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "engine_logic_cases.hpp"

namespace {

std::string Serialize(const k6wp_test::json& doc) {
  return doc.dump(2) + "\n";
}

k6wp_test::json BuildDocument() {
  const auto cases = k6wp_test::EngineLogicCases();
  k6wp_test::json doc = k6wp_test::json::object();
  doc["version"] = 1;
  doc["note"] =
      "The G5 golden fixtures for the engine's PURE logic (task G5). The C++ "
      "side writes them and checks them; the Rust side replays them. A "
      "change in any of these decisions is a behaviour change on both sides.";
  k6wp_test::json arr = k6wp_test::json::array();
  for (const auto& c : cases) {
    k6wp_test::json entry = k6wp_test::json::object();
    entry["module"] = c.module;
    entry["name"] = c.name;
    entry["input"] = c.input;
    entry["output"] = k6wp_test::EvaluateEngineLogicCase(c);
    arr.push_back(std::move(entry));
  }
  doc["cases"] = std::move(arr);
  return doc;
}

}  // namespace

int main(int argc, char** argv) {
  const k6wp_test::json doc = BuildDocument();

  if (argc >= 3 && std::string(argv[1]) == "--check") {
    const std::filesystem::path golden = argv[2];
    std::ifstream in(golden, std::ios::binary);
    if (!in) {
      std::cerr << "engine_logic_dump: cannot open " << golden << "\n";
      return 2;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string expected = ss.str();
    const std::string actual = Serialize(doc);
    if (expected == actual) {
      std::cout << "engine_logic_dump: OK (" << doc["cases"].size()
                << " cases match)\n";
      return 0;
    }
    // Report the FIRST differing case so the failure points at one behaviour.
    k6wp_test::json want =
        k6wp_test::json::parse(expected, nullptr, false);
    if (!want.is_discarded() && want.contains("cases") &&
        want["cases"].is_array() &&
        want["cases"].size() == doc["cases"].size()) {
      for (std::size_t i = 0; i < want["cases"].size(); ++i) {
        if (want["cases"][i] != doc["cases"][i]) {
          std::cerr << "engine_logic_dump: DRIFT in case '"
                    << doc["cases"][i]["module"].get<std::string>() << "/"
                    << doc["cases"][i]["name"].get<std::string>() << "'\n";
          std::cerr << "  expected: " << want["cases"][i]["output"].dump()
                    << "\n";
          std::cerr << "  actual:   " << doc["cases"][i]["output"].dump()
                    << "\n";
          break;
        }
      }
    } else {
      std::cerr << "engine_logic_dump: DRIFT (document shape changed)\n";
    }
    return 1;
  }

  if (argc >= 2) {
    const std::filesystem::path out = argv[1];
    if (out.has_parent_path()) {
      std::error_code ec;
      std::filesystem::create_directories(out.parent_path(), ec);
    }
    std::ofstream file(out, std::ios::binary);
    if (!file) {
      std::cerr << "engine_logic_dump: cannot write " << out << "\n";
      return 2;
    }
    file << Serialize(doc);
    std::cerr << "engine_logic_dump: wrote " << out << " ("
              << doc["cases"].size() << " cases)\n";
    return 0;
  }

  std::cerr << "usage: engine_logic_dump <path> | --check <path>\n";
  return 2;
}
