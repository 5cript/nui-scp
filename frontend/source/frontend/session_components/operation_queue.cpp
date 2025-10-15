#include <frontend/session_components/operation_queue.hpp>
#include <frontend/components/progress_bar.hpp>
#include <frontend/observed_random_access_map.hpp>
#include <shared_data/file_operations/download_progress.hpp>
#include <shared_data/file_operations/scan_progress.hpp>
#include <shared_data/file_operations/operation_added.hpp>
#include <shared_data/file_operations/operation_type.hpp>
#include <shared_data/file_operations/operation_error_type.hpp>
#include <shared_data/file_operations/operation_error.hpp>
#include <shared_data/file_operations/operation_state.hpp>
#include <shared_data/file_operations/operation_completed.hpp>
#include <shared_data/is_paused.hpp>
#include <shared_data/error_or_success.hpp>
#include <utility/convert_naming_convention.hpp>
#include <utility/visit_overloaded.hpp>
#include <utility/format_bytes.hpp>

#include <log/log.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/api/timer.hpp>
#include <nui/frontend/svg.hpp>
#include <nui/rpc.hpp>
#include <fmt/format.h>

#include <ui5/components/switch.hpp>
#include <ui5/components/button.hpp>

#include <variant>
#include <map>
#include <chrono>
#include <string_view>

using namespace std::chrono_literals;

namespace
{
    constexpr std::string_view progressHeight{"15px"};

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
            using namespace Nui::Attributes;

            // clang-format off
            return svg::svg {
                svga::viewBox = "0 0 24 24",
                svga::fill = "none",
            }(
                // Circle (lens)
                svg::circle {
                    svga::cx = "11",
                    svga::cy = "11",
                    svga::r = "7",
                    svga::stroke = "currentColor",
                    svga::strokeWidth = "1.6",
                }(),
                // Handle
                svg::line {
                    "x1"_attr = "16.65",
                    "y1"_attr = "16.65",
                    "x2"_attr = "21",
                    "y2"_attr = "21",
                    svga::stroke = "currentColor",
                    svga::strokeWidth = "1.6",
                    svga::strokeLinecap = "round",
                }()
            );
            // clang-format on
        }

        Nui::ElementRenderer scanAnimated()
        {
            namespace svg = Nui::Elements::Svg;
            namespace svga = Nui::Attributes::Svg;
            using namespace Nui::Attributes;

            // clang-format off
            return svg::svg {
                svga::viewBox = "0 0 24 24",
                svga::fill = "none",
                "xmlns"_attr = "http://www.w3.org/2000/svg"
            }(
                // Lens (circle)
                svg::circle {
                    svga::cx = "11",
                    svga::cy = "11",
                    svga::r = "7",
                    svga::stroke = "currentColor",
                    svga::strokeWidth = "1.6",
                    svga::strokeLinecap = "round",
                    svga::strokeLinejoin = "round"
                }(),

                // Moving scan line inside the lens (thin rounded rect)
                svg::rect {
                    // place rectangle inside circle bounds (left aligned at x=5 to right at x=17)
                    svga::x = "5",
                    svga::y = "6",           // starting y — animate will update this
                    svga::width = "12",
                    svga::height = "0.9",
                    svga::rx = "0.45",      // rounded corners to make it look like a soft line
                    svga::fill = "currentColor",
                    svga::opacity = "0.18",
                    svga::clipPath = "url(#lensClip)" // clip to the lens so it doesn't draw outside
                }(
                    // animate y position: 6 -> 16 -> 6 (loop)
                    svg::animate {
                        svga::attributeName = "y",
                        svga::values = "6;16;6",
                        svga::dur = "1.6s",
                        svga::repeatCount = "indefinite",
                        svga::keyTimes = "0;0.5;1",
                        svga::calcMode = "linear"
                    }()
                ),

                // optional faint pulse on the leading edge to suggest scanning sweep (subtle)
                svg::rect {
                    svga::x = "5",
                    svga::y = "6",
                    svga::width = "12",
                    svga::height = "0.9",
                    svga::rx = "0.45",
                    svga::fill = "currentColor",
                    svga::opacity = "0.0",
                    svga::clipPath = "url(#lensClip)"
                }(
                    svg::animate {
                        svga::attributeName = "opacity",
                        svga::values = "0;0.28;0",
                        svga::dur = "1.6s",
                        svga::repeatCount = "indefinite",
                        svga::keyTimes = "0;0.5;1"
                    }(),
                    // animate the opacity slightly offset (so the pulse follows the scan line)
                    svg::animateTransform {
                        svga::attributeName = "transform",
                        svga::type = "translate",
                        svga::values = "0 0; 0 10; 0 0",
                        svga::dur = "1.6s",
                        svga::repeatCount = "indefinite"
                    }()
                ),

                // Clip path matching the lens so scan line stays inside the circle
                svg::defs {}(
                    svg::clipPath {
                        svga::id = "lensClip"
                    }(
                        svg::circle {
                            svga::cx = "11",
                            svga::cy = "11",
                            svga::r = "7"
                        }()
                    )
                ),

                // Handle
                svg::line {
                    "x1"_attr = "16.65",
                    "y1"_attr = "16.65",
                    "x2"_attr = "21",
                    "y2"_attr = "21",
                    "stroke"_attr = "currentColor",
                    "strokeWidth"_attr = "1.6",
                    "strokeLinecap"_attr = "round",
                    "strokeLinejoin"_attr = "round"
                }()
            );
            // clang-format on
        }
    }

    template <typename Derived>
    class OperationCard
    {
      public:
        explicit OperationCard(
            SharedData::OperationType type,
            Ids::OperationId operationId,
            std::function<void(OperationCard const& operation)> doRemoveSelf,
            std::shared_ptr<Nui::Observed<bool>> doDeletionCountdown)
            : type_{type}
            , operationId_{std::move(operationId)}
            , doRemoveSelf_{std::move(doRemoveSelf)}
            , doDeletionCountdown_{std::move(doDeletionCountdown)}
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

        virtual std::string statusText() const
        {
            return fmt::format("status: {}", formattedState());
        }

        Nui::ElementRenderer operator()() const
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            namespace svg = Nui::Elements::Svg;
            namespace svga = Nui::Attributes::Svg;
            using Nui::Elements::div;
            using Nui::Elements::span;

            // clang-format off
            return section{
                class_ = observe(state_).generate([this](){
                    const auto state = state_.value();
                    if (state == SharedData::OperationState::Completed)
                        return "opq-card opq-completed opq-folded";
                    else if (state == SharedData::OperationState::Failed)
                        return "opq-card opq-failed opq-folded";
                    else if (state == SharedData::OperationState::Canceled)
                        return "opq-card opq-canceled opq-folded";
                    else
                        return "opq-card";
                }),
            }(
                div {
                    class_ = "opq-header",
                }(
                    div {
                        class_ = "opq-type"
                    }(
                        observe(state_),
                        [this]() -> Nui::ElementRenderer {
                            if (type_ == SharedData::OperationType::Download)
                                return Svgs::download();
                            else if (type_ == SharedData::OperationType::Upload)
                                return Svgs::upload();
                            else if (type_ == SharedData::OperationType::Scan)
                            {
                                if (isCompletedState())
                                    return Svgs::scan();
                                else
                                    return Svgs::scanAnimated();
                            }
                            else
                                return div{}("UnknownType");
                        }
                    ),
                    div {
                        class_ = "opq-title"
                    }(
                        div{}(static_cast<Derived const*>(this)->title()),
                        div {
                            class_ = "opq-muted",
                            Nui::Attributes::title = fmt::format("id: {}", operationId_.value())
                        }(
                            Nui::observe(state_),
                            [this]() -> std::string {
                                return static_cast<Derived const*>(this)->statusText();
                            }
                        )
                    ),
                    div{
                        class_ = "opq-clock"
                    }(
                        observe(completionTime_, doDeletionCountdown_).generate([this]() -> Nui::ElementRenderer {
                            if (isCompletedState() && *doDeletionCountdown_)
                            {
                                constexpr auto radius = 10;
                                constexpr auto circumference = 2 * 3.14159 * radius;
                                return svg::svg{
                                    svga::viewBox = "0 0 24 24",
                                    svga::fill = "none",
                                    svga::width = "24",
                                    svga::height = "24",
                                }(
                                    svg::circle{
                                        svga::r = "10",
                                        svga::cx = "12",
                                        svga::cy = "12",
                                        svga::fill = "none",
                                        svga::stroke = "white",
                                        svga::strokeWidth = "2",
                                        svga::strokeDasharray = std::to_string(circumference),
                                        svga::strokeLinecap = "round",
                                        svga::transform = "rotate(-90 12 12)",
                                    }(
                                        svg::animate{
                                            svga::attributeName = "stroke-dashoffset",
                                            svga::values = fmt::format("{};0", circumference),
                                            svga::dur = fmt::format("{}s", OperationQueue::autoRemoveTime.count()),
                                            svga::repeatCount = "1",
                                        }()
                                    )
                                );
                            }
                            return Nui::nil();
                        })
                    ),
                    button {
                        class_ = "opq-btn opq-cancel-btn",
                        onClick = [this](){
                            cancel();
                        }
                    }(
                        observe(state_).generate([this]() -> std::string {
                            if (isCompletedState())
                                return "Remove";
                            else
                                return "Cancel";
                        })
                    )
                ),
                static_cast<Derived const*>(this)->body()
            );
            // clang-format on
        }

        bool isCompletedState() const
        {
            const auto state = state_.value();
            return state == SharedData::OperationState::Completed || state == SharedData::OperationState::Failed ||
                state == SharedData::OperationState::Canceled;
        }

        void cancel() const
        {
            doRemoveSelf_(*this);
        }

        Ids::OperationId operationId() const
        {
            return operationId_;
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

        std::chrono::steady_clock::time_point completionTime() const
        {
            return completionTime_.value();
        }

        void completionTime(std::chrono::steady_clock::time_point time)
        {
            completionTime_ = time;
            Nui::globalEventContext.executeActiveEventsImmediately();
        }

        std::chrono::steady_clock::time_point startTime() const
        {
            return startTime_;
        }

        virtual bool warrantsCancelConfirm() const = 0;

      protected:
        std::chrono::steady_clock::time_point startTime_{std::chrono::steady_clock::now()};
        Nui::Observed<std::chrono::steady_clock::time_point> completionTime_{};
        Nui::Observed<SharedData::OperationState> state_{SharedData::OperationState::NotStarted};
        SharedData::OperationType type_;
        Ids::OperationId operationId_;
        std::function<void(OperationCard const& operation)> doRemoveSelf_;
        std::shared_ptr<Nui::Observed<bool>> doDeletionCountdown_;
    };

    class DisplayedDownloadOperation : public OperationCard<DisplayedDownloadOperation>
    {
      public:
        DisplayedDownloadOperation(
            long long max,
            Ids::OperationId operationId,
            std::filesystem::path localPath,
            std::filesystem::path remotePath,
            std::function<void(OperationCard const& operation)> doRemoveSelf,
            std::shared_ptr<Nui::Observed<bool>> doDeletionCountdown)
            : OperationCard{SharedData::OperationType::Download, std::move(operationId), std::move(doRemoveSelf), std::move(doDeletionCountdown)}
            , progressBar_{{
                  .height = std::string{progressHeight},
                  .min = 0,
                  .max = max,
                  .showMinMax = true,
                  .byteMode = true,
              }}
            , localPath_{std::move(localPath)}
            , remotePath_{std::move(remotePath)}
        {}

        Nui::ElementRenderer body() const
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;
            using Nui::Elements::span;

            // clang-format off
            return div{
                class_ = observe(state_).generate([this](){
                    const auto state = state_.value();
                    if (state == SharedData::OperationState::Completed || state == SharedData::OperationState::Failed || state == SharedData::OperationState::Canceled)
                        return "opq-body opq-collapsed";
                    else
                        return "opq-body";
                }),
            }(
                progressBar_()
            );
            // clang-format on
        }

        void setProgress(long long current)
        {
            progressBar_.setProgress(current);
        }

        std::string title() const
        {
            return fmt::format("Download '{}' to '{}'", remotePath_.generic_string(), localPath_.generic_string());
        }

        bool warrantsCancelConfirm() const override
        {
            return !isCompletedState() && progressBar_.max() > 10'000'000; // 10 MB
        }

      private:
        ProgressBar progressBar_;
        std::filesystem::path localPath_;
        std::filesystem::path remotePath_;
    };

    class DisplayedScanOperation : public OperationCard<DisplayedScanOperation>
    {
      public:
        DisplayedScanOperation(
            Ids::OperationId operationId,
            std::function<void(OperationCard const& operation)> doRemoveSelf,
            std::shared_ptr<Nui::Observed<bool>> doDeletionCountdown)
            : OperationCard{
                  SharedData::OperationType::Scan,
                  std::move(operationId),
                  std::move(doRemoveSelf),
                  std::move(doDeletionCountdown)}
        {}

        Nui::ElementRenderer body() const
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;
            using Nui::Elements::span;

            // clang-format off
            return div{
                class_ = observe(state_).generate([this](){
                    if (isCompletedState())
                        return "opq-body opq-collapsed";
                    else
                        return "opq-body";
                }),
            }(
                div {
                    style = "margin-top: 8px, font-size: 13px; color: var(--muted);"
                }(
                    observe(totalBytes_, currentIndex_, totalScanned_).generate([this]() -> std::string {
                        return fmt::format(
                            "Scanned a total of {} items, currently at item {} ({} total).",
                            totalScanned_.value(),
                            currentIndex_.value(),
                            Utility::formatBytes(totalBytes_.value(), Utility::determineOrderOfMagnitude(totalBytes_.value())));
                    })
                )
            );
            // clang-format on
        }

        void setProgress(std::uint64_t totalBytes, std::uint64_t currentIndex, std::uint64_t totalScanned)
        {
            totalBytes_ = totalBytes;
            currentIndex_ = currentIndex;
            totalScanned_ = totalScanned;
            Nui::globalEventContext.executeActiveEventsImmediately();
        }

        std::string title() const
        {
            return "Scanning remote directory";
        }

        bool warrantsCancelConfirm() const override
        {
            return true;
        }

        std::string statusText() const override
        {
            return fmt::format(
                "status: {}, scanned {} items of size {}",
                formattedState(),
                totalScanned_.value(),
                Utility::formatBytes(totalBytes_.value(), Utility::determineOrderOfMagnitude(totalBytes_.value())));
        }

      private:
        Nui::Observed<std::uint64_t> totalBytes_{0ull};
        Nui::Observed<std::uint64_t> currentIndex_{0ull};
        Nui::Observed<std::uint64_t> totalScanned_{0ull};
    };

    struct DisplayedOperation
    {
        Ids::OperationId operationId;
        SharedData::OperationType type;
        std::variant<std::monostate, DisplayedDownloadOperation, DisplayedScanOperation> details;

        Ids::OperationId key() const
        {
            return operationId;
        }

        SharedData::OperationState state() const
        {
            return Utility::visitOverloaded(
                details,
                [](auto const& op) {
                    return op.state();
                },
                [](std::monostate) {
                    return SharedData::OperationState::NotStarted;
                });
        }

        void state(SharedData::OperationState newState)
        {
            return Utility::visitOverloaded(
                details,
                [newState](auto& op) {
                    op.state(newState);
                    if (op.isCompletedState())
                        op.completionTime(std::chrono::steady_clock::now());
                },
                [](std::monostate) {});
        }

        bool isCompletedState() const
        {
            return Utility::visitOverloaded(
                details,
                [](auto const& op) {
                    return op.isCompletedState();
                },
                [](std::monostate) {
                    return false;
                });
        }

        std::chrono::steady_clock::time_point completionTime() const
        {
            return Utility::visitOverloaded(
                details,
                [](auto const& op) {
                    return op.completionTime();
                },
                [](std::monostate) {
                    return std::chrono::steady_clock::now();
                });
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
    Nui::Observed<bool> paused{true};
    std::shared_ptr<Nui::Observed<bool>> autoClean{std::make_shared<Nui::Observed<bool>>(false)};
    ObservedRandomAccessMap<Ids::OperationId, DisplayedOperation, std::map> operations;
    Nui::TimerHandle autoCleanTimer;

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
        , autoCleanTimer{}
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
{
    impl_->stateHolder->load(
        [this, name = impl_->persistenceSessionName](bool success, Persistence::StateHolder& holder) {
            if (!success)
                return;

            auto const& state = holder.stateCache().fullyResolve();

            auto iter = state.sessions.find(name);
            if (iter == end(state.sessions))
            {
                Log::error("No options found for name: {}", name);
                return;
            }

            auto [engineKey, engine] = *iter;
            impl_->paused = engine.queueOptions->startInPausedState.value_or(true);
            *impl_->autoClean = engine.queueOptions->autoRemoveCompletedOperations.value_or(false);
            Nui::globalEventContext.executeActiveEventsImmediately();
        });

    Nui::setInterval(
        1000,
        [this]() {
            if (impl_->autoClean->value() && !impl_->operations.empty())
            {
                auto now = std::chrono::steady_clock::now();
                bool anyRemoved = false;
                for (auto* front = impl_->operations.front(); front != nullptr; front = impl_->operations.front())
                {
                    if (front->isCompletedState())
                    {
                        auto duration = now - front->completionTime();
                        if (duration >= autoRemoveTime)
                        {
                            impl_->operations.pop_front();
                            anyRemoved = true;
                            continue;
                        }
                    }
                    break;
                }
                if (anyRemoved)
                    Nui::globalEventContext.executeActiveEventsImmediately();
            }
        },
        [this](Nui::TimerHandle&& handle) {
            impl_->autoCleanTimer = std::move(handle);
        });
}
ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(OperationQueue);

template <typename OperationCard>
void OperationQueue::cancelOperation(OperationCard const& operation)
{
    if (operation.isCompletedState())
    {
        impl_->operations.erase(operation.operationId());
        Nui::globalEventContext.executeActiveEventsImmediately();
    }
    else
    {
        auto doCancel = [this, operationId = operation.operationId()]() {
            Nui::RpcClient::callWithBackChannel(
                fmt::format("OperationQueue::{}::cancel", impl_->sessionId.value()),
                [this, operationId](SharedData::ErrorOrSuccess<> const& result) {
                    if (result)
                    {
                        auto* operation = impl_->operations.at(operationId);
                        if (operation)
                            operation->state(SharedData::OperationState::Canceled);
                        Nui::globalEventContext.executeActiveEventsImmediately();
                    }
                    else
                    {
                        Log::error("Failed to cancel operation id {}: {}", operationId.value(), result.error.value());
                    }
                },
                operationId);
        };

        if (operation.warrantsCancelConfirm())
        {
            impl_->confirmDialog->open(
                {.headerText = "Cancel Operation",
                 .text = fmt::format("Are you sure you want to cancel the operation?"),
                 .buttons = ConfirmDialog::Button::Yes | ConfirmDialog::Button::No,
                 .onClose = [doCancel](ConfirmDialog::Button buttonPressed) {
                     if (buttonPressed == ConfirmDialog::Button::Yes)
                         doCancel();
                 }});
        }
        else
        {
            doCancel();
        }
    }
}

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
                        .details = [&added, this]() -> decltype(DisplayedOperation::details) {
                            if (added.type == SharedData::OperationType::Download)
                            {
                                if (!added.localPath || !added.remotePath)
                                {
                                    Log::error(
                                        "Received OperationAdded for operation id: {} without localPath or remotePath",
                                        added.operationId.value());
                                    return std::monostate{};
                                }
                                return DisplayedDownloadOperation{
                                    added.totalBytes ? static_cast<long long>(*added.totalBytes) : 0,
                                    added.operationId,
                                    *added.localPath,
                                    *added.remotePath,
                                    [this](OperationCard<DisplayedDownloadOperation> const& operation) {
                                        cancelOperation(operation);
                                    },
                                    impl_->autoClean,
                                };
                            }
                            else if (added.type == SharedData::OperationType::Scan)
                            {
                                if (!added.remotePath)
                                {
                                    Log::error(
                                        "Received OperationAdded for operation id: {} without remotePath",
                                        added.operationId.value());
                                    return std::monostate{};
                                }
                                // TODO:
                                return DisplayedScanOperation{
                                    added.operationId,
                                    [this](OperationCard<DisplayedScanOperation> const& operation) {
                                        cancelOperation(operation);
                                    },
                                    impl_->autoClean,
                                };
                            }
                            return std::monostate{};
                        }()});
                Nui::globalEventContext.executeActiveEventsImmediately();
            }));

    impl_->onUpdate.push_back(
        Nui::RpcClient::autoRegisterFunction(
            fmt::format("OperationQueue::{}::onDownloadProgress", impl_->sessionId.value()),
            [this](SharedData::DownloadProgress const& progress) {
                Log::debug(
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
            fmt::format("OperationQueue::{}::onScanProgress", impl_->sessionId.value()),
            [this](SharedData::ScanProgress const& progress) {
                Log::debug(
                    "Received scan progress for operation id: {} - totalBytes: {}, currentIndex: {}, totalItems: {}",
                    progress.operationId.value(),
                    progress.totalBytes,
                    progress.currentIndex,
                    progress.totalScanned);

                auto* operation = impl_->operations.at(progress.operationId);
                if (!operation)
                {
                    Log::error("Received scan progress for unknown operation id: {}", progress.operationId.value());
                    return;
                }
                if (operation->type != SharedData::OperationType::Scan)
                {
                    Log::error(
                        "Received scan progress for operation id: {} which is not a scan",
                        progress.operationId.value());
                    return;
                }
                auto* details = std::get_if<DisplayedScanOperation>(&operation->details);
                if (!details)
                {
                    Log::error(
                        "Received scan progress for operation id: {} which has no scan details",
                        progress.operationId.value());
                    return;
                }
                details->setProgress(progress.totalBytes, progress.currentIndex, progress.totalScanned);
            }));

    impl_->onUpdate.push_back(
        Nui::RpcClient::autoRegisterFunction(
            fmt::format("OperationQueue::{}::onOperationCompleted", impl_->sessionId.value()), [this](Nui::val val) {
                SharedData::OperationCompleted completed{};
                try
                {
                    Nui::convertFromVal(val, completed);
                }
                catch (std::exception const& e)
                {
                    Log::error("Failed to convert OperationCompleted: {}", e.what());
                    return;
                }

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
                        operation->state(SharedData::OperationState::Completed);
                        break;
                    }
                    case (SharedData::OperationCompletionReason::Canceled):
                    {
                        operation->state(SharedData::OperationState::Canceled);
                        break;
                    }
                    case (SharedData::OperationCompletionReason::Failed):
                    {
                        operation->state(SharedData::OperationState::Failed);
                        break;
                    }
                    default:
                        Log::warn(
                            "Received operation completed for operation id: {} with unknown reason: {}",
                            completed.operationId.value(),
                            static_cast<int>(completed.reason));
                }
                Nui::globalEventContext.executeActiveEventsImmediately();
            }));

    Nui::RpcClient::callWithBackChannel(
        fmt::format("OperationQueue::{}::isPaused", impl_->sessionId.value()),
        [this](SharedData::ErrorOrSuccess<SharedData::IsPaused> const& result) {
            if (result)
            {
                Log::info("Initial pause state of operation queue is: {}", result.paused ? "paused" : "running");
                impl_->paused = result.paused;
                Nui::globalEventContext.executeActiveEventsImmediately();
            }
            else
            {
                Log::error("Failed to get initial paused state: {}", result.error.value());
            }
        });
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
            element->details));
    };

    auto makeSummaryText = [this]() -> std::string {
        return fmt::format("{} total operations", impl_->operations.observedValues().value().size());
    };

    // clang-format off
    return div{
        class_ = "operation-queue",
    }(
        header{
            class_ = "opq-controls"
        }(
            div{
                class_ = "opq-play-toggle",
                role = "button",
                tabIndex = "0",
                onClick = [this](){
                    Nui::RpcClient::callWithBackChannel(fmt::format("OperationQueue::{}::pauseUnpause", impl_->sessionId.value()), [this, requestToPauseUnpauseWas = !impl_->paused.value()](SharedData::ErrorOrSuccess<> const& result){
                        if (result) {
                            Log::info("{} operation queue successfully", requestToPauseUnpauseWas ? "Paused" : "Unpaused");
                            impl_->paused = !impl_->paused.value();
                            Nui::globalEventContext.executeActiveEventsImmediately();
                        }
                        else
                            Log::error("Failed to {} operation queue: {}", requestToPauseUnpauseWas ? "pause" : "unpause", result.error.value());
                    }, !impl_->paused.value());
                }
            }(
                div{
                    class_ = "opq-icon",
                }(
                    observe(impl_->paused).generate([this](){
                        if (impl_->paused.value())
                            return Svgs::play();
                        else
                            return Svgs::pause();
                    })
                ),
                div{
                    class_ = "opq-label",
                }(
                    observe(impl_->paused).generate([this]() -> std::string {
                        if (!impl_->paused.value())
                            return "Pause";
                        else
                            return "Continue";
                    })
                )
            ),
            div{
                class_ = "opq-summary"
            }(
                observe(impl_->operations.observedValues()).generate(makeSummaryText)
            )
        ),
        // Main content
        div{
            class_ = "opq-list"
        }(
            impl_->operations.observedValues().map(operationsMapper)
        ),
        // Footer
        div{
            class_ = "opq-footer"
        }(
            ui5::switch_{
                "checked"_prop = impl_->autoClean,
                "design"_prop = "Graphical",
                "change"_event = [this](Nui::val event) {
                    *impl_->autoClean = event["target"]["checked"].as<bool>();
                    impl_->stateHolder->load([this, name = impl_->persistenceSessionName](bool success, Persistence::StateHolder& holder) {
                        if (!success)
                            return;

                        auto iter = holder.stateCache().sessions.find(name);
                        iter->second.queueOptions->autoRemoveCompletedOperations = impl_->autoClean->value();
                        holder.save([]() {});
                    });
                }
            }(),
            div{
                style = "font-size: 14px; color: var(--muted);"
            }("Auto Remove Completed Operations")
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
    std::filesystem::path const&,
    std::filesystem::path const&,
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
    std::filesystem::path const&,
    std::filesystem::path const&,
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
    std::filesystem::path const&,
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