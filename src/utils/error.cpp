#include "mcpp/utils/error.h"
#include "mcpp/core/json_rpc_message.h"

namespace mcpp::utils {

std::unique_ptr<core::JsonRpcResponse> create_error_response(
    const core::JsonRpcResponse::IdType& id,
    core::JsonRpcErrorCode code,
    const std::string& message,
    const std::optional<nlohmann::json>& data) {

    core::JsonRpcError error{code, message, data};
    return std::make_unique<core::JsonRpcResponse>(id, error);
}

} // namespace mcpp::utils