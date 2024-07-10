#include <persistence/state/termios.hpp>

#ifdef _WIN32
#else
#    include <termios.h>
#endif

namespace Persistence
{
    unsigned int Termios::InputFlags::assemble() const
    {
#ifdef _WIN32
        return 0;
#else
        return 0 | (IGNBRK_ ? IGNBRK : 0) | (BRKINT_ ? BRKINT : 0) | (IGNPAR_ ? IGNPAR : 0) | (PARMRK_ ? PARMRK : 0) |
            (INPCK_ ? INPCK : 0) | (ISTRIP_ ? ISTRIP : 0) | (INLCR_ ? INLCR : 0) | (IGNCR_ ? IGNCR : 0) |
            (ICRNL_ ? ICRNL : 0) | (IUCLC_ ? IUCLC : 0) | (IXON_ ? IXON : 0) | (IXANY_ ? IXANY : 0) |
            (IXOFF_ ? IXOFF : 0) | (IMAXBEL_ ? IMAXBEL : 0) | (IUTF8_ ? IUTF8 : 0);
#endif
    }

    unsigned int Termios::OutputFlags::assemble() const
    {
#ifdef _WIN32
        return 0;
#else
        unsigned int flags = 0 | (OPOST_ ? OPOST : 0) | (OLCUC_ ? OLCUC : 0) | (ONLCR_ ? ONLCR : 0) |
            (OCRNL_ ? OCRNL : 0) | (ONOCR_ ? ONOCR : 0) | (ONLRET_ ? ONLRET : 0) | (OFILL_ ? OFILL : 0) |
            (OFDEL_ ? OFDEL : 0);

        if (NLDLY_ == "NL0")
            flags |= NL0;
        else if (NLDLY_ == "NL1")
            flags |= NL1;

        if (CRDLY_ == "CR0")
            flags |= CR0;
        else if (CRDLY_ == "CR1")
            flags |= CR1;
        else if (CRDLY_ == "CR2")
            flags |= CR2;
        else if (CRDLY_ == "CR3")
            flags |= CR3;

        if (TABDLY_ == "TAB0")
            flags |= TAB0;
        else if (TABDLY_ == "TAB1")
            flags |= TAB1;
        else if (TABDLY_ == "TAB2")
            flags |= TAB2;
        else if (TABDLY_ == "TAB3")
            flags |= TAB3;

        if (BSDLY_ == "BS0")
            flags |= BS0;
        else if (BSDLY_ == "BS1")
            flags |= BS1;

        if (VTDLY_ == "VT0")
            flags |= VT0;
        else if (VTDLY_ == "VT1")
            flags |= VT1;

        if (FFDLY_ == "FF0")
            flags |= FF0;
        else if (FFDLY_ == "FF1")
            flags |= FF1;

        return flags;
#endif
    }

    unsigned int Termios::ControlFlags::assemble() const
    {
#ifdef _WIN32
        return 0;
#else
        unsigned int flags = 0 | (CBAUD_) | (CBAUDEX_) | (CSTOPB_ ? CSTOPB : 0) | (CREAD_ ? CREAD : 0) |
            (PARENB_ ? PARENB : 0) | (PARODD_ ? PARODD : 0) | (HUPCL_ ? HUPCL : 0) | (CLOCAL_ ? CLOCAL : 0);

        if (CSIZE_ == "CS5")
            flags |= CS5;
        else if (CSIZE_ == "CS6")
            flags |= CS6;
        else if (CSIZE_ == "CS7")
            flags |= CS7;
        else if (CSIZE_ == "CS8")
            flags |= CS8;

        return flags;
#endif
    }

    unsigned int Termios::LocalFlags::assemble() const
    {
#ifdef _WIN32
        return 0;
#else
        return 0 | (ISIG_ ? ISIG : 0) | (ICANON_ ? ICANON : 0) | (XCASE_ ? XCASE : 0) | (ECHO_ ? ECHO : 0) |
            (ECHOE_ ? ECHOE : 0) | (ECHOK_ ? ECHOK : 0) | (ECHONL_ ? ECHONL : 0) | (ECHOCTL_ ? ECHOCTL : 0) |
            (ECHOPRT_ ? ECHOPRT : 0) | (ECHOKE_ ? ECHOKE : 0) | (FLUSHO_ ? FLUSHO : 0) | (NOFLSH_ ? NOFLSH : 0) |
            (TOSTOP_ ? TOSTOP : 0) | (PENDIN_ ? PENDIN : 0) | (IEXTEN_ ? IEXTEN : 0);
#endif
    }

    std::vector<unsigned char> Termios::CC::assemble() const
    {
#ifdef _WIN32
        return {};
#else
        return std::vector<unsigned char>{
            VDISCARD_, VDSUSP_,   VEOF_,   VEOL_,    VEOL2_, VERASE_, VINTR_,  VKILL_, VLNEXT_,  VMIN_,
            VQUIT_,    VREPRINT_, VSTART_, VSTATUS_, VSTOP_, VSUSP_,  VSWTCH_, VTIME_, VWERASE_,
        };
#endif
    }

    void to_json(nlohmann::json& j, Termios::InputFlags const& inputFlags)
    {
        j = {
            {"IGNBRK", inputFlags.IGNBRK_},
            {"BRKINT", inputFlags.BRKINT_},
            {"IGNPAR", inputFlags.IGNPAR_},
            {"PARMRK", inputFlags.PARMRK_},
            {"INPCK", inputFlags.INPCK_},
            {"ISTRIP", inputFlags.ISTRIP_},
            {"INLCR", inputFlags.INLCR_},
            {"IGNCR", inputFlags.IGNCR_},
            {"ICRNL", inputFlags.ICRNL_},
            {"IUCLC", inputFlags.IUCLC_},
            {"IXON", inputFlags.IXON_},
            {"IXANY", inputFlags.IXANY_},
            {"IXOFF", inputFlags.IXOFF_},
            {"IMAXBEL", inputFlags.IMAXBEL_},
            {"IUTF8", inputFlags.IUTF8_},
        };
    }
    void from_json(nlohmann::json const& j, Termios::InputFlags& inputFlags)
    {
        if (j.contains("IGNBRK"))
            j.at("IGNBRK").get_to(inputFlags.IGNBRK_);
        if (j.contains("BRKINT"))
            j.at("BRKINT").get_to(inputFlags.BRKINT_);
        if (j.contains("IGNPAR"))
            j.at("IGNPAR").get_to(inputFlags.IGNPAR_);
        if (j.contains("PARMRK"))
            j.at("PARMRK").get_to(inputFlags.PARMRK_);
        if (j.contains("INPCK"))
            j.at("INPCK").get_to(inputFlags.INPCK_);
        if (j.contains("ISTRIP"))
            j.at("ISTRIP").get_to(inputFlags.ISTRIP_);
        if (j.contains("INLCR"))
            j.at("INLCR").get_to(inputFlags.INLCR_);
        if (j.contains("IGNCR"))
            j.at("IGNCR").get_to(inputFlags.IGNCR_);
        if (j.contains("ICRNL"))
            j.at("ICRNL").get_to(inputFlags.ICRNL_);
        if (j.contains("IUCLC"))
            j.at("IUCLC").get_to(inputFlags.IUCLC_);
        if (j.contains("IXON"))
            j.at("IXON").get_to(inputFlags.IXON_);
        if (j.contains("IXANY"))
            j.at("IXANY").get_to(inputFlags.IXANY_);
        if (j.contains("IXOFF"))
            j.at("IXOFF").get_to(inputFlags.IXOFF_);
        if (j.contains("IMAXBEL"))
            j.at("IMAXBEL").get_to(inputFlags.IMAXBEL_);
        if (j.contains("IUTF8"))
            j.at("IUTF8").get_to(inputFlags.IUTF8_);
    }
    void to_json(nlohmann::json& j, Termios::OutputFlags const& outputFlags)
    {
        j = {
            {"OPOST", outputFlags.OPOST_},
            {"OLCUC", outputFlags.OLCUC_},
            {"ONLCR", outputFlags.ONLCR_},
            {"OCRNL", outputFlags.OCRNL_},
            {"ONOCR", outputFlags.ONOCR_},
            {"ONLRET", outputFlags.ONLRET_},
            {"OFILL", outputFlags.OFILL_},
            {"OFDEL", outputFlags.OFDEL_},
            {"NLDLY", outputFlags.NLDLY_},
            {"CRDLY", outputFlags.CRDLY_},
            {"TABDLY", outputFlags.TABDLY_},
            {"BSDLY", outputFlags.BSDLY_},
            {"VTDLY", outputFlags.VTDLY_},
            {"FFDLY", outputFlags.FFDLY_},
        };
    }
    void from_json(nlohmann::json const& j, Termios::OutputFlags& outputFlags)
    {
        if (j.contains("OPOST"))
            j.at("OPOST").get_to(outputFlags.OPOST_);
        if (j.contains("OLCUC"))
            j.at("OLCUC").get_to(outputFlags.OLCUC_);
        if (j.contains("ONLCR"))
            j.at("ONLCR").get_to(outputFlags.ONLCR_);
        if (j.contains("OCRNL"))
            j.at("OCRNL").get_to(outputFlags.OCRNL_);
        if (j.contains("ONOCR"))
            j.at("ONOCR").get_to(outputFlags.ONOCR_);
        if (j.contains("ONLRET"))
            j.at("ONLRET").get_to(outputFlags.ONLRET_);
        if (j.contains("OFILL"))
            j.at("OFILL").get_to(outputFlags.OFILL_);
        if (j.contains("OFDEL"))
            j.at("OFDEL").get_to(outputFlags.OFDEL_);
        if (j.contains("NLDLY"))
            j.at("NLDLY").get_to(outputFlags.NLDLY_);
        if (j.contains("CRDLY"))
            j.at("CRDLY").get_to(outputFlags.CRDLY_);
        if (j.contains("TABDLY"))
            j.at("TABDLY").get_to(outputFlags.TABDLY_);
        if (j.contains("BSDLY"))
            j.at("BSDLY").get_to(outputFlags.BSDLY_);
        if (j.contains("VTDLY"))
            j.at("VTDLY").get_to(outputFlags.VTDLY_);
        if (j.contains("FFDLY"))
            j.at("FFDLY").get_to(outputFlags.FFDLY_);
    }
    void to_json(nlohmann::json& j, Termios::ControlFlags const& controlFlags)
    {
        j = {
            {"CBAUD", controlFlags.CBAUD_},
            {"CBAUDEX", controlFlags.CBAUDEX_},
            {"CSIZE", controlFlags.CSIZE_},
            {"CSTOPB", controlFlags.CSTOPB_},
            {"CREAD", controlFlags.CREAD_},
            {"PARENB", controlFlags.PARENB_},
            {"PARODD", controlFlags.PARODD_},
            {"HUPCL", controlFlags.HUPCL_},
            {"CLOCAL", controlFlags.CLOCAL_},
            {"LOBLK", controlFlags.LOBLK_},
            {"CIBAUD", controlFlags.CIBAUD_},
            {"CMSPAR", controlFlags.CMSPAR_},
            {"CRTSCTS", controlFlags.CRTSCTS_},
        };
    }
    void from_json(nlohmann::json const& j, Termios::ControlFlags& controlFlags)
    {
        if (j.contains("CBAUD"))
            j.at("CBAUD").get_to(controlFlags.CBAUD_);
        if (j.contains("CBAUDEX"))
            j.at("CBAUDEX").get_to(controlFlags.CBAUDEX_);
        if (j.contains("CSIZE"))
            j.at("CSIZE").get_to(controlFlags.CSIZE_);
        if (j.contains("CSTOPB"))
            j.at("CSTOPB").get_to(controlFlags.CSTOPB_);
        if (j.contains("CREAD"))
            j.at("CREAD").get_to(controlFlags.CREAD_);
        if (j.contains("PARENB"))
            j.at("PARENB").get_to(controlFlags.PARENB_);
        if (j.contains("PARODD"))
            j.at("PARODD").get_to(controlFlags.PARODD_);
        if (j.contains("HUPCL"))
            j.at("HUPCL").get_to(controlFlags.HUPCL_);
        if (j.contains("CLOCAL"))
            j.at("CLOCAL").get_to(controlFlags.CLOCAL_);
        if (j.contains("LOBLK"))
            j.at("LOBLK").get_to(controlFlags.LOBLK_);
        if (j.contains("CIBAUD"))
            j.at("CIBAUD").get_to(controlFlags.CIBAUD_);
        if (j.contains("CMSPAR"))
            j.at("CMSPAR").get_to(controlFlags.CMSPAR_);
        if (j.contains("CRTSCTS"))
            j.at("CRTSCTS").get_to(controlFlags.CRTSCTS_);
    }
    void to_json(nlohmann::json& j, Termios::LocalFlags const& localFlags)
    {
        j = {
            {"ISIG", localFlags.ISIG_},
            {"ICANON", localFlags.ICANON_},
            {"XCASE", localFlags.XCASE_},
            {"ECHO", localFlags.ECHO_},
            {"ECHOE", localFlags.ECHOE_},
            {"ECHOK", localFlags.ECHOK_},
            {"ECHONL", localFlags.ECHONL_},
            {"ECHOCTL", localFlags.ECHOCTL_},
            {"ECHOPRT", localFlags.ECHOPRT_},
            {"ECHOKE", localFlags.ECHOKE_},
            {"FLUSHO", localFlags.FLUSHO_},
            {"NOFLSH", localFlags.NOFLSH_},
            {"TOSTOP", localFlags.TOSTOP_},
            {"PENDIN", localFlags.PENDIN_},
            {"IEXTEN", localFlags.IEXTEN_},
        };
    }
    void from_json(nlohmann::json const& j, Termios::LocalFlags& localFlags)
    {
        if (j.contains("ISIG"))
            j.at("ISIG").get_to(localFlags.ISIG_);
        if (j.contains("ICANON"))
            j.at("ICANON").get_to(localFlags.ICANON_);
        if (j.contains("XCASE"))
            j.at("XCASE").get_to(localFlags.XCASE_);
        if (j.contains("ECHO"))
            j.at("ECHO").get_to(localFlags.ECHO_);
        if (j.contains("ECHOE"))
            j.at("ECHOE").get_to(localFlags.ECHOE_);
        if (j.contains("ECHOK"))
            j.at("ECHOK").get_to(localFlags.ECHOK_);
        if (j.contains("ECHONL"))
            j.at("ECHONL").get_to(localFlags.ECHONL_);
        if (j.contains("ECHOCTL"))
            j.at("ECHOCTL").get_to(localFlags.ECHOCTL_);
        if (j.contains("ECHOPRT"))
            j.at("ECHOPRT").get_to(localFlags.ECHOPRT_);
        if (j.contains("ECHOKE"))
            j.at("ECHOKE").get_to(localFlags.ECHOKE_);
        if (j.contains("FLUSHO"))
            j.at("FLUSHO").get_to(localFlags.FLUSHO_);
        if (j.contains("NOFLSH"))
            j.at("NOFLSH").get_to(localFlags.NOFLSH_);
        if (j.contains("TOSTOP"))
            j.at("TOSTOP").get_to(localFlags.TOSTOP_);
        if (j.contains("PENDIN"))
            j.at("PENDIN").get_to(localFlags.PENDIN_);
        if (j.contains("IEXTEN"))
            j.at("IEXTEN").get_to(localFlags.IEXTEN_);
    }
    void to_json(nlohmann::json& j, Termios::CC const& cc)
    {
        j = {
            {"VDISCARD", cc.VDISCARD_}, {"VDSUSP", cc.VDSUSP_},   {"VEOF", cc.VEOF_},       {"VEOL", cc.VEOL_},
            {"VEOL2", cc.VEOL2_},       {"VERASE", cc.VERASE_},   {"VINTR", cc.VINTR_},     {"VKILL", cc.VKILL_},
            {"VLNEXT", cc.VLNEXT_},     {"VMIN", cc.VMIN_},       {"VQUIT", cc.VQUIT_},     {"VREPRINT", cc.VREPRINT_},
            {"VSTART", cc.VSTART_},     {"VSTATUS", cc.VSTATUS_}, {"VSTOP", cc.VSTOP_},     {"VSUSP", cc.VSUSP_},
            {"VSWTCH", cc.VSWTCH_},     {"VTIME", cc.VTIME_},     {"VWERASE", cc.VWERASE_},
        };
    }
    void from_json(nlohmann::json const& j, Termios::CC& cc)
    {
        if (j.contains("VDISCARD"))
            j.at("VDISCARD").get_to(cc.VDISCARD_);
        if (j.contains("VDSUSP"))
            j.at("VDSUSP").get_to(cc.VDSUSP_);
        if (j.contains("VEOF"))
            j.at("VEOF").get_to(cc.VEOF_);
        if (j.contains("VEOL"))
            j.at("VEOL").get_to(cc.VEOL_);
        if (j.contains("VEOL2"))
            j.at("VEOL2").get_to(cc.VEOL2_);
        if (j.contains("VERASE"))
            j.at("VERASE").get_to(cc.VERASE_);
        if (j.contains("VINTR"))
            j.at("VINTR").get_to(cc.VINTR_);
        if (j.contains("VKILL"))
            j.at("VKILL").get_to(cc.VKILL_);
        if (j.contains("VLNEXT"))
            j.at("VLNEXT").get_to(cc.VLNEXT_);
        if (j.contains("VMIN"))
            j.at("VMIN").get_to(cc.VMIN_);
        if (j.contains("VQUIT"))
            j.at("VQUIT").get_to(cc.VQUIT_);
        if (j.contains("VREPRINT"))
            j.at("VREPRINT").get_to(cc.VREPRINT_);
        if (j.contains("VSTART"))
            j.at("VSTART").get_to(cc.VSTART_);
        if (j.contains("VSTATUS"))
            j.at("VSTATUS").get_to(cc.VSTATUS_);
        if (j.contains("VSTOP"))
            j.at("VSTOP").get_to(cc.VSTOP_);
        if (j.contains("VSUSP"))
            j.at("VSUSP").get_to(cc.VSUSP_);
        if (j.contains("VSWTCH"))
            j.at("VSWTCH").get_to(cc.VSWTCH_);
        if (j.contains("VTIME"))
            j.at("VTIME").get_to(cc.VTIME_);
        if (j.contains("VWERASE"))
            j.at("VWERASE").get_to(cc.VWERASE_);
    }

    void to_json(nlohmann::json& j, Termios const& termios)
    {
        j = {
            {"inputFlags", termios.inputFlags},
            {"outputFlags", termios.outputFlags},
            {"controlFlags", termios.controlFlags},
            {"localFlags", termios.localFlags},
            {"cc", termios.cc},
            {"iSpeed", termios.iSpeed},
            {"oSpeed", termios.oSpeed},
        };
    }
    void from_json(nlohmann::json const& j, Termios& termios)
    {
        termios = Termios{};

        if (j.contains("inputFlags"))
            j.at("inputFlags").get_to(termios.inputFlags);

        if (j.contains("outputFlags"))
            j.at("outputFlags").get_to(termios.outputFlags);

        if (j.contains("controlFlags"))
            j.at("controlFlags").get_to(termios.controlFlags);

        if (j.contains("localFlags"))
            j.at("localFlags").get_to(termios.localFlags);

        if (j.contains("cc"))
            j.at("cc").get_to(termios.cc);

        if (j.contains("iSpeed"))
            j.at("iSpeed").get_to(termios.iSpeed);

        if (j.contains("oSpeed"))
            j.at("oSpeed").get_to(termios.oSpeed);
    }
}