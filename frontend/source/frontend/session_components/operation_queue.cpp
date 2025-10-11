#include <frontend/session_components/operation_queue.hpp>
#include <frontend/components/progress_bar.hpp>
#include <frontend/observed_random_access_map.hpp>
#include <shared_data/file_operations/download_progress.hpp>
#include <shared_data/file_operations/operation_added.hpp>
#include <shared_data/file_operations/operation_type.hpp>
#include <shared_data/file_operations/operation_error_type.hpp>
#include <shared_data/file_operations/operation_error.hpp>
#include <shared_data/file_operations/operation_state.hpp>
#include <shared_data/file_operations/operation_completed.hpp>
#include <utility/convert_naming_convention.hpp>
#include <utility/visit_overloaded.hpp>

#include <log/log.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/svg.hpp>
#include <nui/rpc.hpp>
#include <fmt/format.h>

#include <variant>
#include <map>
#include <cstdint>
#include <chrono>

namespace
{
    namespace Svgs
    {
        Nui::ElementRenderer play()
        {
            namespace svg = Nui::Elements::Svg;
            namespace svga = Nui::Attributes::Svg;

            // clang-format off
            return svg::svg {
                svga::viewBox = "0 0 24 24",
                svga::fill = "none",
            }(
                svg::path {
                    svga::d = "M8 5v14l11-7L8 5z",
                    svga::fill = "currentColor"
                }()
            );
            // clang-format on
        }

        Nui::ElementRenderer pause()
        {
            namespace svg = Nui::Elements::Svg;
            namespace svga = Nui::Attributes::Svg;

            // clang-format off
            return svg::svg {
                svga::viewBox = "0 0 24 24",
                svga::fill = "none",
            }(
                svg::path {
                    svga::d = "M6 5h4v14H6zM14 5h4v14h-4z",
                    svga::fill = "currentColor"
                }()
            );
            // clang-format on
        }

        Nui::ElementRenderer download()
        {
            namespace svg = Nui::Elements::Svg;
            namespace svga = Nui::Attributes::Svg;

            // clang-format off
            return svg::svg {
                svga::viewBox = "0 0 24 24",
                svga::fill = "none",
            }(
                svg::path {
                    svga::d = "M12 3v10m0 0 4-4m-4 4-4-4M5 21h14",
                    svga::stroke = "currentColor",
                    svga::strokeWidth = "1.6",
                    svga::strokeLinecap = "round",
                    svga::strokeLinejoin = "round"
                }()
            );
            // clang-format on
        }

        Nui::ElementRenderer upload()
        {
            namespace svg = Nui::Elements::Svg;
            namespace svga = Nui::Attributes::Svg;

            // clang-format off
            return svg::svg {
                svga::viewBox = "0 0 24 24",
                svga::fill = "none",
            }(
                svg::path {
                    svga::d = "M12 21V11m0 0 4 4m-4-4-4 4M19 3H5",
                    svga::stroke = "currentColor",
                    svga::strokeWidth = "1.6",
                    svga::strokeLinecap = "round",
                    svga::strokeLinejoin = "round"
                }()
            );
            // clang-format on
        }

        Nui::ElementRenderer scan()
        {
            namespace svg = Nui::Elements::Svg;
            namespace svga = Nui::Attributes::Svg;

            // clang-format off
            return svg::svg {
                svga::viewBox = "0 0 24 24",
                svga::fill = "none",
            }(
                svg::path {
                    svga::d = "M3 7v4a4 4 0 0 0 4 4h10",
                    svga::stroke = "currentColor",
                    svga::strokeWidth = "1.6",
                    svga::strokeLinecap = "round",
                    svga::strokeLinejoin = "round"
                }(),
                svg::path {
                    svga::d = "M21 17v-4a4 4 0 0 0-4-4H7",
                    svga::stroke = "currentColor",
                    svga::strokeWidth = "1.6",
                    svga::strokeLinecap = "round",
                    svga::strokeLinejoin = "round"
                }()
            );
            // clang-format on
        }
    }

    template <typename Derived>
    class OperationCard
    {
      public:
        explicit OperationCard(SharedData::OperationType type)
            : type_{type}
        {}

        std::string formattedState() const
        {
            return Utility::splitByPascalCase(Utility::enumToString(state_.value())).joined(" ");
        }

        void state(SharedData::OperationState newState)
        {
            state_ = newState;
        }

        SharedData::OperationState state() const
        {
            return state_.value();
        }

        Nui::ElementRenderer operator()() const
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;
            using Nui::Elements::span;

            // clang-format off
            return section{
                class_ = "opq-card"
            }(
                div {
                    class_ = "opq-header",
                }(
                    div {
                        class_ = "type"
                    }(
                        [this]() -> Nui::ElementRenderer {
                            if (type_ == SharedData::OperationType::Download)
                                return Svgs::download();
                            else if (type_ == SharedData::OperationType::Upload)
                                return Svgs::upload();
                            else if (type_ == SharedData::OperationType::Rename)
                                return Svgs::scan();
                            else if (type_ == SharedData::OperationType::Delete)
                                return Svgs::scan();
                            else
                                return div{}("UnknownType");
                        }()
                    ),
                    div {
                        class_ = "opq-title"
                    }(
                        div{}(title_),
                        div {
                            class_ = "opq-muted"
                        // TODO: Build this string correctly
                        }(
                            Nui::observe(state_),
                            [this]() -> std::string {
                                return fmt::format("id: {} • status: {}", operationId_.value(), formattedState());
                            }
                        )
                    ),
                    button {
                        class_ = "opq-btn opq-cancel-btn",
                    }(
                        "Cancel"
                    )
                ),
                static_cast<Derived const*>(this)->body()
            );
            // clang-format on
        }

        Nui::ElementRenderer elapsedTime() const
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            return div{style = "margin-top: 8px, font-size: 13px; color: var(--muted);"}(
                // TODO:
                "Elapsed: 0s");
        }

      protected:
        std::chrono::steady_clock::time_point startTime_{std::chrono::steady_clock::now()};
        Nui::Observed<SharedData::OperationState> state_{SharedData::OperationState::NotStarted};
        SharedData::OperationType type_;
        Ids::OperationId operationId_;
        Nui::Observed<std::string> title_{"Not started"};
    };

    class DisplayedDownloadOperation : public OperationCard<DisplayedDownloadOperation>
    {
      public:
        DisplayedDownloadOperation(long long min, long long max)
            : OperationCard{SharedData::OperationType::Download}
            , progressBar_{{
                  .height = "10px",
                  .min = min,
                  .max = max,
                  .showMinMax = true,
                  .byteMode = true,
              }}
        {}

        Nui::ElementRenderer body() const
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;
            using Nui::Elements::span;

            // clang-format off
            return div{
                class_ = "opq-body",
            }(
                div{
                    class_ = "opq-muted"
                }("Dynamic content area — this would show progress, nested items, details, etc."),
                progressBar_()
            );
            // clang-format on
        }

        void setProgress(long long current)
        {
            progressBar_.setProgress(current);
        }

      private:
        ProgressBar progressBar_;
    };

    struct DisplayedOperation
    {
        Ids::OperationId operationId;
        SharedData::OperationType type;
        std::variant<std::monostate, DisplayedDownloadOperation> details;

        Ids::OperationId key() const
        {
            return operationId;
        }
    };
}

struct OperationQueue::Implementation
{
    Persistence::StateHolder* stateHolder;
    FrontendEvents* events;
    std::string persistenceSessionName;
    Ids::SessionId sessionId;
    ConfirmDialog* confirmDialog;
    FileEngine* fileEngine;

    std::vector<Nui::RpcClient::AutoUnregister> onUpdate;
    ObservedRandomAccessMap<Ids::OperationId, DisplayedOperation, std::map> operations;

    Implementation(
        Persistence::StateHolder* stateHolder,
        FrontendEvents* events,
        std::string persistenceSessionName,
        ConfirmDialog* confirmDialog)
        : stateHolder{stateHolder}
        , events{events}
        , persistenceSessionName{std::move(persistenceSessionName)}
        , sessionId{}
        , confirmDialog{confirmDialog}
        , fileEngine{nullptr}
        , onUpdate{}
        , operations{}
    {}
};

OperationQueue::OperationQueue(
    Persistence::StateHolder* stateHolder,
    FrontendEvents* events,
    std::string persistenceSessionName,
    ConfirmDialog* confirmDialog)
    : impl_{
          std::make_unique<Implementation>(stateHolder, events, std::move(persistenceSessionName), confirmDialog),
      }
{}
ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(OperationQueue);

void OperationQueue::activate(FileEngine* fileEngine, Ids::SessionId sessionId)
{
    impl_->fileEngine = fileEngine;
    impl_->sessionId = std::move(sessionId);

    impl_->onUpdate.push_back(
        Nui::RpcClient::autoRegisterFunction(
            fmt::format("OperationQueue::{}::onOperationAdded", impl_->sessionId.value()),
            [this](SharedData::OperationAdded const& added) {
                impl_->operations.insert(
                    added.operationId,
                    DisplayedOperation{
                        .operationId = added.operationId,
                        .type = added.type,
                        .details = [&added]() -> decltype(DisplayedOperation::details) {
                            if (added.type == SharedData::OperationType::Download)
                                return DisplayedDownloadOperation{
                                    0, added.totalBytes ? static_cast<long long>(*added.totalBytes) : 0};
                            return std::monostate{};
                        }()});
                Nui::globalEventContext.executeActiveEventsImmediately();
            }));

    impl_->onUpdate.push_back(
        Nui::RpcClient::autoRegisterFunction(
            fmt::format("OperationQueue::{}::onDownloadProgress", impl_->sessionId.value()),
            [this](SharedData::DownloadProgress const& progress) {
                Log::info(
                    "Received download progress for operation id: {} - {}/{}",
                    progress.operationId.value(),
                    progress.current - progress.min,
                    progress.max - progress.min);

                auto* operation = impl_->operations.at(progress.operationId);
                if (!operation)
                {
                    Log::error("Received download progress for unknown operation id: {}", progress.operationId.value());
                    return;
                }
                if (operation->type != SharedData::OperationType::Download)
                {
                    Log::error(
                        "Received download progress for operation id: {} which is not a download",
                        progress.operationId.value());
                    return;
                }
                auto* details = std::get_if<DisplayedDownloadOperation>(&operation->details);
                if (!details)
                {
                    Log::error(
                        "Received download progress for operation id: {} which has no download details",
                        progress.operationId.value());
                    return;
                }
                details->setProgress(progress.current - progress.min);
            }));

    impl_->onUpdate.push_back(
        Nui::RpcClient::autoRegisterFunction(
            fmt::format("OperationQueue::{}::onOperationCompleted", impl_->sessionId.value()),
            [this](SharedData::OperationCompleted const& completed) {
                Log::info("Received operation completed for operation id: {}", completed.operationId.value());
                auto* operation = impl_->operations.at(completed.operationId);
                if (!operation)
                {
                    Log::error(
                        "Received operation completed for unknown operation id: {}", completed.operationId.value());
                    return;
                }
                switch (completed.reason)
                {
                    case (SharedData::OperationCompletionReason::Completed):
                    {
                        Utility::visitOverloaded(
                            operation->details,
                            [](auto& op) {
                                op.state(SharedData::OperationState::Completed);
                            },
                            [](std::monostate) {
                                // Nothing to do
                            });
                        break;
                    }
                    case (SharedData::OperationCompletionReason::Canceled):
                    {
                        Utility::visitOverloaded(
                            operation->details,
                            [](auto& op) {
                                op.state(SharedData::OperationState::Canceled);
                            },
                            [](std::monostate) {
                                // Nothing to do
                            });
                        break;
                    }
                    case (SharedData::OperationCompletionReason::Failed):
                    {
                        Utility::visitOverloaded(
                            operation->details,
                            [](auto& op) {
                                op.state(SharedData::OperationState::Failed);
                            },
                            [](std::monostate) {
                                // Nothing to do
                            });
                        break;
                    }
                }
            }));
}

Nui::ElementRenderer OperationQueue::operator()()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;
    using Nui::Elements::span;

    auto operationsMapper = [](auto, auto const& element) {
        return div{}(std::visit(
            [](auto const& details) -> Nui::ElementRenderer {
                if constexpr (std::is_same_v<std::decay_t<decltype(details)>, std::monostate>)
                {
                    return div{}("Unknown operation type");
                }
                else
                {
                    return details();
                }
            },
            element.details));
    };

    // clang-format off
    return div{
        class_ = "operation-queue",
        style = "width: 100%; height: auto; display: block",
    }(
        header{
            class_ = "opq-controls"
        }(
            div{
                class_ = "opq-play-toggle",
                // TODO: remove id
                id = "playToggle",
                role = "button",
                tabIndex = "0",
                "aria-pressed"_prop = "false"
            }(
                div{
                    class_ = "icon",
                    // TODO: remove id
                    id = "playIcon",
                    "aria-hidden"_prop = "true"
                }(
                    // svg replaced by js
                ),
                div{
                    class_ = "label",
                    id = "playLabel"
                }("Processing")
            ),
            div{
                class_ = "opq-pill",
                id = "queueState",
            }(
                span{}("Queue: "),
                strong{id = "queueCount"}("0")
            ),
            div{
                class_ = "summary",
                id = "summaryText"
            }("No operations yet")
        ),
        // Main content
        div{}(
            div{
                class_ = "opq-list",
                id = "operationList"
            }(
                impl_->operations.observedValues().map(operationsMapper)
            )
        )
    );
    // clang-format on
}

void OperationQueue::enqueueDownload(
    std::filesystem::path const& remotePath,
    std::filesystem::path const& localPath,
    std::function<void(std::optional<Ids::OperationId> const&)> onComplete)
{
    if (!impl_->fileEngine)
    {
        Log::error("No file engine set for operation queue, cannot enqueue download");
        onComplete(std::nullopt);
        return;
    }

    Log::info("Frontend Operation Queue download: {} -> {}", remotePath.generic_string(), localPath.generic_string());
    impl_->fileEngine->addDownload(remotePath, localPath, std::move(onComplete));
}
void OperationQueue::enqueueUpload(
    std::filesystem::path const& localPath,
    std::filesystem::path const& remotePath,
    std::function<void(std::optional<Ids::OperationId> const&)> onComplete)
{
    if (!impl_->fileEngine)
    {
        Log::error("No file engine set for operation queue, cannot enqueue download");
        onComplete(std::nullopt);
        return;
    }
    // TODO: Implement
}
void OperationQueue::enqueueRename(
    std::filesystem::path const& oldPath,
    std::filesystem::path const& newPath,
    std::function<void(std::optional<Ids::OperationId> const&)> onComplete)
{
    if (!impl_->fileEngine)
    {
        Log::error("No file engine set for operation queue, cannot enqueue download");
        onComplete(std::nullopt);
        return;
    }
    // TODO: Implement
}
void OperationQueue::enqueueDelete(
    std::filesystem::path const& path,
    std::function<void(std::optional<Ids::OperationId> const&)> onComplete)
{
    if (!impl_->fileEngine)
    {
        Log::error("No file engine set for operation queue, cannot enqueue download");
        onComplete(std::nullopt);
        return;
    }
    // TODO: Implement
}