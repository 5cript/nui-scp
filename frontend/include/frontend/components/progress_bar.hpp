#pragma once

#include <nui/frontend/element_renderer.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

class ProgressBar
{
  public:
    struct Settings
    {
        std::string height{"30px"};
        long long min{0};
        long long max{100};
        bool showMinMax{false};
        bool byteMode{false};
    };
    ProgressBar(Settings settings);

    ROAR_PIMPL_SPECIAL_FUNCTIONS(ProgressBar);

    Nui::ElementRenderer operator()() const;

    /**
     * Set the progress of the progress bar.
     * Cannot be set if the progress bar is not mounted.
     */
    void setProgress(long long current);

    /**
     * @brief Get the maximum value of the progress bar.
     *
     * @return long long
     */
    long long max() const;

  private:
    void updateText();

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};