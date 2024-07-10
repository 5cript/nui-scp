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
            bool IGNBRK_{false};
            bool BRKINT_{false};
            bool IGNPAR_{false};
            bool PARMRK_{false};
            bool INPCK_{false};
            bool ISTRIP_{false};
            bool INLCR_{false};
            bool IGNCR_{false};
            bool ICRNL_{true};
            bool IUCLC_{false};
            bool IXON_{true};
            bool IXANY_{false};
            bool IXOFF_{false};
            bool IMAXBEL_{false};
            bool IUTF8_{true};

            unsigned int assemble() const;
        } inputFlags;

        struct OutputFlags
        {
            bool OPOST_{true};
            bool OLCUC_{false};
            bool ONLCR_{true};
            bool OCRNL_{false};
            bool ONOCR_{false};
            bool ONLRET_{false};
            bool OFILL_{false};
            bool OFDEL_{false};
            std::string NLDLY_{};
            std::string CRDLY_{};
            std::string TABDLY_{};
            std::string BSDLY_{};
            std::string VTDLY_{};
            std::string FFDLY_{};

            unsigned int assemble() const;
        } outputFlags;

        struct ControlFlags
        {
            unsigned int CBAUD_{0};
            unsigned int CBAUDEX_{0};
            std::string CSIZE_{"CS8"};
            bool CSTOPB_{false};
            bool CREAD_{true};
            bool PARENB_{false};
            bool PARODD_{false};
            bool HUPCL_{false};
            bool CLOCAL_{false};
            bool LOBLK_{false};
            bool CIBAUD_{0};
            bool CMSPAR_{false};
            bool CRTSCTS_{false};

            unsigned int assemble() const;
        } controlFlags;

        struct LocalFlags
        {
            bool ISIG_{true};
            bool ICANON_{true};
            bool XCASE_{false};
            bool ECHO_{true};
            bool ECHOE_{true};
            bool ECHOK_{true};
            bool ECHONL_{false};
            bool ECHOCTL_{true};
            bool ECHOPRT_{false};
            bool ECHOKE_{true};
            bool FLUSHO_{false};
            bool NOFLSH_{false};
            bool TOSTOP_{false};
            bool PENDIN_{false};
            bool IEXTEN_{true};

            unsigned int assemble() const;
        } localFlags;

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
        } cc;

        unsigned int iSpeed{0};
        unsigned int oSpeed{0};
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