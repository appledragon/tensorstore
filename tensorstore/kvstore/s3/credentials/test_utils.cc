// Copyright 2023 The TensorStore Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tensorstore/kvstore/s3/credentials/test_utils.h"

#include <string>

#include "absl/strings/cord.h"
#include "absl/time/time.h"
#include "tensorstore/internal/http/http_request.h"
#include "tensorstore/internal/http/http_response.h"
#include "tensorstore/util/str_cat.h"

namespace tensorstore {
namespace internal_kvstore_s3 {

Future<internal_http::HttpResponse> EC2MetadataMockTransport::IssueRequest(
    const internal_http::HttpRequest& request, absl::Cord payload,
    absl::Duration request_timeout, absl::Duration connect_timeout) {
  ABSL_LOG(INFO) << request;
  if (auto it = url_to_response_.find(StrCat(request.method, " ", request.url));
      it != url_to_response_.end()) {
    return it->second;
  }

  return internal_http::HttpResponse{404, absl::Cord(), {}};
}

absl::flat_hash_map<std::string, internal_http::HttpResponse>
DefaultEC2MetadataFlow(const std::string& api_token,
                       const std::string& access_key,
                       const std::string& secret_key,
                       const std::string& session_token,
                       const absl::Time& expires_at) {
  return absl::flat_hash_map<std::string, internal_http::HttpResponse>{
      {"POST http://169.254.169.254/latest/api/token",
       internal_http::HttpResponse{200, absl::Cord{api_token}}},
      {"GET http://169.254.169.254/latest/meta-data/iam/",
       internal_http::HttpResponse{
           200, absl::Cord{"info"}, {{"x-aws-ec2-metadata-token", api_token}}}},
      {"GET http://169.254.169.254/latest/meta-data/iam/security-credentials/",
       internal_http::HttpResponse{200,
                                   absl::Cord{"mock-iam-role"},
                                   {{"x-aws-ec2-metadata-token", api_token}}}},
      {"GET "
       "http://169.254.169.254/latest/meta-data/iam/security-credentials/"
       "mock-iam-role",
       internal_http::HttpResponse{
           200,
           absl::Cord(absl::StrFormat(
               R"({
                      "Code": "Success",
                      "AccessKeyId": "%s",
                      "SecretAccessKey": "%s",
                      "Token": "%s",
                      "Expiration": "%s"
                  })",
               access_key, secret_key, session_token,
               absl::FormatTime("%Y-%m-%d%ET%H:%M:%E*S%Ez", expires_at,
                                absl::UTCTimeZone()))),
           {{"x-aws-ec2-metadata-token", api_token}}}}};
}

}  // namespace internal_kvstore_s3
}  // namespace tensorstore