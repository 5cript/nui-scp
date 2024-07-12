#pragma once

#include <persistence/state_core.hpp>

#include <string>
#include <vector>

namespace Persistence
{
    struct Termios
    {
        struct InputFlags
        {
            std::optional<bool> IGNBRK_{false};
            std::optional<bool> BRKINT_{false};
            std::optional<bool> IGNPAR_{false};
            std::optional<bool> PARMRK_{false};
            std::optional<bool> INPCK_{false};
            std::optional<bool> ISTRIP_{false};
            std::optional<bool> INLCR_{false};
            std::optional<bool> IGNCR_{false};
            std::optional<bool> ICRNL_{true};
            std::optional<bool> IUCLC_{false};
            std::optional<bool> IXON_{true};
            std::optional<bool> IXANY_{false};
            std::optional<bool> IXOFF_{false};
            std::optional<bool> IMAXBEL_{false};
            std::optional<bool> IUTF8_{true};

            unsigned int assemble() const;
            void useDefaultsFrom(InputFlags const& other);
        };
        std::optional<InputFlags> inputFlags{InputFlags{}};

        struct OutputFlags
        {
            std::optional<bool> OPOST_{true};
            std::optional<bool> OLCUC_{false};
            std::optional<bool> ONLCR_{true};
            std::optional<bool> OCRNL_{false};
            std::optional<bool> ONOCR_{false};
            std::optional<bool> ONLRET_{false};
            std::optional<bool> OFILL_{false};
            std::optional<bool> OFDEL_{false};
            std::optional<std::string> NLDLY_{};
            std::optional<std::string> CRDLY_{};
            std::optional<std::string> TABDLY_{};
            std::optional<std::string> BSDLY_{};
            std::optional<std::string> VTDLY_{};
            std::optional<std::string> FFDLY_{};

            unsigned int assemble() const;
            void useDefaultsFrom(OutputFlags const& other);
        };
        std::optional<OutputFlags> outputFlags{OutputFlags{}};

        struct ControlFlags
        {
            std::optional<unsigned int> CBAUD_{0};
            std::optional<bool> CBAUDEX_{0};
            std::optional<std::string> CSIZE_{"CS8"};
            std::optional<bool> CSTOPB_{false};
            std::optional<bool> CREAD_{true};
            std::optional<bool> PARENB_{false};
            std::optional<bool> PARODD_{false};
            std::optional<bool> HUPCL_{false};
            std::optional<bool> CLOCAL_{false};
            std::optional<bool> LOBLK_{false};
            std::optional<bool> CIBAUD_{0};
            std::optional<bool> CMSPAR_{false};
            std::optional<bool> CRTSCTS_{false};

            unsigned int assemble() const;
            void useDefaultsFrom(ControlFlags const& other);
        };
        std::optional<ControlFlags> controlFlags{ControlFlags{}};

        struct LocalFlags
        {
            std::optional<bool> ISIG_{true};
            std::optional<bool> ICANON_{true};
            std::optional<bool> XCASE_{false};
            std::optional<bool> ECHO_{true};
            std::optional<bool> ECHOE_{true};
            std::optional<bool> ECHOK_{true};
            std::optional<bool> ECHONL_{false};
            std::optional<bool> ECHOCTL_{true};
            std::optional<bool> ECHOPRT_{false};
            std::optional<bool> ECHOKE_{true};
            std::optional<bool> FLUSHO_{false};
            std::optional<bool> NOFLSH_{false};
            std::optional<bool> TOSTOP_{false};
            std::optional<bool> PENDIN_{false};
            std::optional<bool> IEXTEN_{true};

            unsigned int assemble() const;
            void useDefaultsFrom(LocalFlags const& other);
        };
        std::optional<LocalFlags> localFlags{LocalFlags{}};

        struct CC
        {
            unsigned char VDISCARD_{3};
            unsigned char VDSUSP_{28};
            unsigned char VEOF_{127};
            unsigned char VEOL_{21};
            unsigned char VEOL2_{4};
            unsigned char VERASE_{0};
            unsigned char VINTR_{1};
            unsigned char VKILL_{0};
            unsigned char VLNEXT_{17};
            unsigned char VMIN_{19};
            unsigned char VQUIT_{26};
            unsigned char VREPRINT_{0};
            unsigned char VSTART_{18};
            unsigned char VSTATUS_{15};
            unsigned char VSTOP_{23};
            unsigned char VSUSP_{22};
            unsigned char VSWTCH_{0};
            unsigned char VTIME_{0};
            unsigned char VWERASE_{0};

            std::vector<unsigned char> assemble() const;
        };
        std::optional<CC> cc{std::nullopt};

        std::optional<unsigned int> iSpeed{0};
        std::optional<unsigned int> oSpeed{0};

        void useDefaultsFrom(Termios const& other);
    };
    void to_json(nlohmann::json& j, Termios::InputFlags const& inputFlags);
    void from_json(nlohmann::json const& j, Termios::InputFlags& inputFlags);
    void to_json(nlohmann::json& j, Termios::OutputFlags const& outputFlags);
    void from_json(nlohmann::json const& j, Termios::OutputFlags& outputFlags);
    void to_json(nlohmann::json& j, Termios::ControlFlags const& controlFlags);
    void from_json(nlohmann::json const& j, Termios::ControlFlags& controlFlags);
    void to_json(nlohmann::json& j, Termios::LocalFlags const& localFlags);
    void from_json(nlohmann::json const& j, Termios::LocalFlags& localFlags);
    void to_json(nlohmann::json& j, Termios::CC const& cc);
    void from_json(nlohmann::json const& j, Termios::CC& cc);

    void to_json(nlohmann::json& j, Termios const& termios);
    void from_json(nlohmann::json const& j, Termios& termios);
}