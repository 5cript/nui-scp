#pragma once

#include <backend/process/boost_process.hpp>

#include <nui/utility/move_detector.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

#include <memory>

namespace ConPTY
{
    class PseudoConsole
    {
      public:
        ROAR_PIMPL_SPECIAL_FUNCTIONS(PseudoConsole);

        /// Factory method to create a new pseudo console.
        friend PseudoConsole createPseudoConsole();

        bool prepareProcessLauncher(boost::process::v2::windows::default_launcher& launcher);
        void closeOtherPipeEnd();

        void resize(short width, short height);

        void
        startReading(std::function<void(std::string_view)> onStdout, std::function<void(std::string_view)> onStderr);

        void stopReading();

        bool write(std::string_view data);

      private:
        PseudoConsole();
        bool initialize();

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;

        Nui::MoveDetector moveDetector_;
    };

    PseudoConsole createPseudoConsole();
}