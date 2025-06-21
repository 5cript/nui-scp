#include <backend/sftp/operation.hpp>
#include <backend/sftp/download_operation.hpp>

template <typename FunctionT>
auto Operation::visit(FunctionT&& func) const
{
    using enum OperationType;
    switch (type())
    {
        case Download:
        {
            return func(static_cast<DownloadOperation const&>(*this));
        }
        default:
        {
            Log::error("Operation: Unknown operation type: {}", static_cast<int>(type()));
            return func(std::nullopt);
        }
    }
}