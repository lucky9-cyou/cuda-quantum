/*******************************************************************************
 * Copyright (c) 2022 - 2023 NVIDIA Corporation & Affiliates.                  *
 * All rights reserved.                                                        *
 *                                                                             *
 * This source code and the accompanying materials are made available under    *
 * the terms of the Apache License 2.0 which accompanies this distribution.    *
 ******************************************************************************/
#include "common/Logger.h"
#include "common/MeasureCounts.h"
#include "common/RestClient.h"
#include "common/ServerHelper.h"
#include "cudaq/utils/cudaq_utils.h"
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace cudaq {

/// @brief Find and set the API and refresh tokens, and the time string.
void findApiKeyInFile(std::string &apiKey, const std::string &path,
                      std::string &refreshKey, std::string &timeStr);

/// Search for the API key, invokes findApiKeyInFile
std::string searchAPIKey(std::string &key, std::string &refreshKey,
                         std::string &timeStr,
                         std::string userSpecifiedConfig = "");

/// @brief The EmulateServerHelper implements the ServerHelper interface
/// to map Job requests and Job result retrievals actions from the calling
/// Executor to the specific schema required by the remote Emulate REST
/// server.
class EmulateServerHelper : public ServerHelper {
protected:
  /// @brief The base URL
  std::string baseUrl = "https://qapi.quantinuum.com/v1/";

  /// @brief Return the headers required for the REST calls
  RestHeaders generateRequestHeader() const;

public:
  /// @brief Return the name of this server helper, must be the
  /// same as the qpu config file.
  const std::string name() const override { return "emulate"; }
  RestHeaders getHeaders() override;

  void initialize(BackendConfig config) override {
    backendConfig = config;

    // Set an alternate base URL if provided
    auto iter = backendConfig.find("url");
    if (iter != backendConfig.end()) {
      baseUrl = iter->second;
      if (!baseUrl.ends_with("/"))
        baseUrl += "/";
    }

    parseConfigForCommonParams(config);
  }

  /// @brief Create a job payload for the provided quantum codes
  ServerJobPayload
  createJob(std::vector<KernelExecution> &circuitCodes) override;

  /// @brief Return the job id from the previous job post
  std::string extractJobId(ServerMessage &postResponse) override;

  /// @brief Return the URL for retrieving job results
  std::string constructGetJobPath(ServerMessage &postResponse) override;
  std::string constructGetJobPath(std::string &jobId) override;

  /// @brief Return true if the job is done
  bool jobIsDone(ServerMessage &getJobResponse) override;

  /// @brief Given a completed job response, map back to the sample_result
  cudaq::sample_result processResults(ServerMessage &postJobResponse,
                                      std::string &jobID) override;
};

ServerJobPayload
EmulateServerHelper::createJob(std::vector<KernelExecution> &circuitCodes) {

  std::vector<ServerMessage> messages;
  for (auto &circuitCode : circuitCodes) {
    // Construct the job itself
    ServerMessage j;
    j["qasm"] = circuitCode.code;
    j["shots"] = shots;
    j["format"] = "json";
    messages.push_back(j);
  }
  // Get the headers
  RestHeaders headers = generateRequestHeader();

  cudaq::info("Created job payload for quantinuum, language is OpenQASM 2.0");

  // return the payload
  return std::make_tuple(baseUrl + "submit", headers, messages);
}

std::string EmulateServerHelper::extractJobId(ServerMessage &postResponse) {
  return postResponse["task_id"].get<std::string>();
}

std::string
EmulateServerHelper::constructGetJobPath(ServerMessage &postResponse) {
  return baseUrl + "get_task/" + extractJobId(postResponse);
}

std::string EmulateServerHelper::constructGetJobPath(std::string &jobId) {
  return baseUrl + "get_task/" + jobId;
}

bool EmulateServerHelper::jobIsDone(ServerMessage &getJobResponse) {
  auto status = getJobResponse["status"].get<std::string>();
  if (status == "failed") {
    std::string msg = "";
    if (getJobResponse.count("result"))
      msg = getJobResponse["result"].get<std::string>();
    throw std::runtime_error("Job failed to execute msg = [" + msg + "]");
  } else if (status == "error") {
    std::string msg = "";
    if (getJobResponse.count("Error"))
      msg = getJobResponse["Error"].get<std::string>();
    throw std::runtime_error("Job failed to execute with error = [" + msg +
                             "]");
  }

  return status == "succeeded";
}

cudaq::sample_result
EmulateServerHelper::processResults(ServerMessage &postJobResponse,
                                    std::string &jobId) {
  // Results come back as a map of vectors. Each map key corresponds to a qubit
  // and its corresponding vector holds the measurement results in each shot:
  // {
  //   "Memory": {
  //     "var6": {
  //       "0": {
  //         "Bin value": "0b00000",
  //         "Count": 510,
  //         "Hex value": "0x0",
  //         "Int value": 0
  //       },
  //       "1": {
  //         "Bin value": "0b11111",
  //         "Count": 490,
  //         "Hex value": "0x1f",
  //         "Int value": 31
  //       }
  //     }
  //   },
  //   "Times": {
  //     "Parsing": 7,
  //     "Simulation": 47
  //   },
  //   "status": "succeeded",
  //   "task_id": "97f93288-5faf-4df5-b9c4-31340fd13fdc"
  // }
  auto results = postJobResponse["Memory"];

  cudaq::info("Results message: {}", results.dump());

  std::vector<ExecutionResult> srs;

  // Populate individual registers' results into srs
  for (auto &[registerName, registerResult] : results.items()) {
    CountsDictionary thisRegCounts;
    for (auto &[key, value] : registerResult.items()) {
      thisRegCounts[value["Bin value"].get<std::string>()] =
          value["Count"].get<size_t>();
    }
    srs.emplace_back(thisRegCounts, registerName);
  }

  return sample_result(srs);
}

std::map<std::string, std::string>
EmulateServerHelper::generateRequestHeader() const {
  std::string apiKey, refreshKey, timeStr;
  std::map<std::string, std::string> headers{
      {"Authorization", apiKey},
      {"Content-Type", "application/json"},
      {"Connection", "keep-alive"},
      {"Accept", "*/*"}};
  return headers;
}

RestHeaders EmulateServerHelper::getHeaders() {
  return generateRequestHeader();
}

} // namespace cudaq

CUDAQ_REGISTER_TYPE(cudaq::ServerHelper, cudaq::EmulateServerHelper, emulate)
