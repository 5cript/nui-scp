#include <frontend/components/progress_bar.hpp>

#include <nui/frontend/attributes.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/dom/basic_element.hpp>
#include <fmt/format.h>

#include <algorithm>

// clang-format off
#ifdef NUI_INLINE
// @inline(css, scp-progress-bar)
.progress-bar {
    height: 30px;
    background-color: #f1f1f1;
    border-radius: 15px;
    overflow: hidden;
    position: relative;
    display: flex;
    justify-content: center;
    align-items: center;
}

.progress-bar > div:nth-child(1) {
    height: 100%;
    width: 0%;
    transition: width 0.3s linear, background-color 0.3s linear;
    background-color: #4CAF50;
    position: absolute;
    left: 0;
}

.progress-bar > div:nth-child(2) {
    position: relative;
    z-index: 1;
    font-size: 16px;
    font-weight: bold;
    color: #303030;
    text-shadow: 0 0 5px rgba(0, 0, 0, 0.5);
}

.progress-bar > div:nth-child(3) {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: linear-gradient(to right, transparent, rgba(255, 255, 255, 0.5), transparent);
    animation: progress-shine 2s infinite;
}

.progress-bar > div:nth-child(3).hidden {
    display: none;
}

@media (prefers-reduced-motion: reduce) {
    .progress-bar > div:nth-child(3) {
        animation: none;
    }
}

@keyframes progress-shine {
    0% {
        transform: translateX(-100%);
    }

    100% {
        transform: translateX(100%);
    }
}
// @endinline
#endif
// clang-format on

namespace
{
    enum class OrderOfMagnitude
    {
        None = 0,
        Kilo,
        Mega,
        Giga,
        Tera
    };

    int percentageBetween(long long current, long long min, long long max)
    {
        // Ensure min is less than or equal to max
        if (min > max)
            std::swap(min, max);

        // Clamp current between min and max
        current = std::clamp(current, min, max);

        // Avoid division by zero if min == max
        if (min == max)
            return 100.0;

        // Calculate percentage
        return static_cast<int>(100. * static_cast<double>(current - min) / static_cast<double>(max - min));
    }

    OrderOfMagnitude determineOrderOfMagnitude(long long value)
    {
        if (value < 1000)
            return OrderOfMagnitude::None;
        else if (value < 1'000'000)
            return OrderOfMagnitude::Kilo;
        else if (value < 1'000'000'000)
            return OrderOfMagnitude::Mega;
        else if (value < 1'000'000'000'000)
            return OrderOfMagnitude::Giga;
        else
            return OrderOfMagnitude::Tera;
    }
}

struct ProgressBar::Implementation : public ProgressBar::Settings
{
    Nui::Observed<long long> progress{0};
    Nui::Observed<std::string> text{"0%"};
    Nui::Observed<std::string> backgroundColor{"#a0a0a0"};
    OrderOfMagnitude magnitude;

    Implementation(Settings settings)
        : Settings{std::move(settings)}
        , magnitude{determineOrderOfMagnitude(settings.max)}
    {}
};

ProgressBar::ProgressBar(Settings settings)
    : impl_{std::make_unique<Implementation>(std::move(settings))}
{}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(ProgressBar);

Nui::ElementRenderer ProgressBar::operator()() const
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;
    using fmt::format;

    // clang-format off
    return div{
        class_ = "progress-bar",
        style = Style {
            "height"_style = impl_->height,
        },
    }(
        // bar fill
        div{
            style = Style{
                "width"_style = observe(impl_->progress).generate([this]() {
                    return format("{}%", percentageBetween(impl_->progress.value(), impl_->min, impl_->max));
                }),
                "height"_style = impl_->height,
                "background-color"_style = impl_->backgroundColor
            },
        }(),
        // text
        div{}(impl_->text),
        // shine animation
        div{}()
    );
    // clang-format on
}

void ProgressBar::updateText()
{
    impl_->text = [this, percent = percentageBetween(impl_->progress.value(), impl_->min, impl_->max)]() {
        if (impl_->showMinMax)
        {
            if (impl_->byteMode)
            {
                const auto formatSize = [](long long value, OrderOfMagnitude magnitude) {
                    switch (magnitude)
                    {
                        case OrderOfMagnitude::None:
                            return fmt::format("{} B", value);
                        case OrderOfMagnitude::Kilo:
                            return fmt::format("{:.2f} KB", value / 1024.0);
                        case OrderOfMagnitude::Mega:
                            return fmt::format("{:.2f} MB", value / (1024.0 * 1024.0));
                        case OrderOfMagnitude::Giga:
                            return fmt::format("{:.2f} GB", value / (1024.0 * 1024.0 * 1024.0));
                        case OrderOfMagnitude::Tera:
                            return fmt::format("{:.2f} TB", value / (1024.0 * 1024.0 * 1024.0 * 1024.0));
                    }
                    return std::string{};
                };

                return fmt::format(
                    "{} - {} ({})",
                    formatSize(impl_->progress.value(), impl_->magnitude),
                    formatSize(impl_->max, impl_->magnitude),
                    percent);
            }
            else
                return fmt::format("{} / {} ({})", impl_->progress.value(), impl_->max, percent);
        }
        else
            return fmt::format("{}%", percent);
    }();
}

void ProgressBar::setProgress(long long current)
{
    const auto percent = percentageBetween(current, impl_->min, impl_->max);
    const auto hue = std::max(5., 120. * std::pow(static_cast<double>(percent) / 100., 1.8));

    impl_->progress = current;
    impl_->backgroundColor = fmt::format("hsl({}, 100%, 50%)", static_cast<int>(hue));
    updateText();

    Nui::globalEventContext.executeActiveEventsImmediately();
}

long long ProgressBar::max() const
{
    return impl_->max;
}