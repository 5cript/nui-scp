#include <persistence/state/termios.hpp>

#ifdef _WIN32
#else
#    include <termios.h>
#endif

namespace Persistence
{
    namespace
    {
        bool unpackBoolOptional(std::optional<bool> const& value)
        {
            return value.has_value() ? value.value() : false;
        }
    }

    unsigned int Termios::InputFlags::assemble() const
    {
#ifdef _WIN32
        return 0;
#else
        return 0 | (unpackBoolOptional(IGNBRK_) ? IGNBRK : 0) | (unpackBoolOptional(BRKINT_) ? BRKINT : 0) |
            (unpackBoolOptional(IGNPAR_) ? IGNPAR : 0) | (unpackBoolOptional(PARMRK_) ? PARMRK : 0) |
            (unpackBoolOptional(INPCK_) ? INPCK : 0) | (unpackBoolOptional(ISTRIP_) ? ISTRIP : 0) |
            (unpackBoolOptional(INLCR_) ? INLCR : 0) | (unpackBoolOptional(IGNCR_) ? IGNCR : 0) |
            (unpackBoolOptional(ICRNL_) ? ICRNL : 0) | (unpackBoolOptional(IUCLC_) ? IUCLC : 0) |
            (unpackBoolOptional(IXON_) ? IXON : 0) | (unpackBoolOptional(IXANY_) ? IXANY : 0) |
            (unpackBoolOptional(IXOFF_) ? IXOFF : 0) | (unpackBoolOptional(IMAXBEL_) ? IMAXBEL : 0) |
            (unpackBoolOptional(IUTF8_) ? IUTF8 : 0);
#endif
    }

    void Termios::InputFlags::useDefaultsFrom(InputFlags const& other)
    {

        if (!IGNBRK_)
            IGNBRK_ = other.IGNBRK_;
        if (!BRKINT_)
            BRKINT_ = other.BRKINT_;
        if (!IGNPAR_)
            IGNPAR_ = other.IGNPAR_;
        if (!PARMRK_)
            PARMRK_ = other.PARMRK_;
        if (!INPCK_)
            INPCK_ = other.INPCK_;
        if (!ISTRIP_)
            ISTRIP_ = other.ISTRIP_;
        if (!INLCR_)
            INLCR_ = other.INLCR_;
        if (!IGNCR_)
            IGNCR_ = other.IGNCR_;
        if (!ICRNL_)
            ICRNL_ = other.ICRNL_;
        if (!IUCLC_)
            IUCLC_ = other.IUCLC_;
        if (!IXON_)
            IXON_ = other.IXON_;
        if (!IXANY_)
            IXANY_ = other.IXANY_;
        if (!IXOFF_)
            IXOFF_ = other.IXOFF_;
        if (!IMAXBEL_)
            IMAXBEL_ = other.IMAXBEL_;
        if (!IUTF8_)
            IUTF8_ = other.IUTF8_;
    }

    unsigned int Termios::OutputFlags::assemble() const
    {
#ifdef _WIN32
        return 0;
#else
        unsigned int flags = 0 | (unpackBoolOptional(OPOST_) ? OPOST : 0) | (unpackBoolOptional(OLCUC_) ? OLCUC : 0) |
            (unpackBoolOptional(ONLCR_) ? ONLCR : 0) | (unpackBoolOptional(OCRNL_) ? OCRNL : 0) |
            (unpackBoolOptional(ONOCR_) ? ONOCR : 0) | (unpackBoolOptional(ONLRET_) ? ONLRET : 0) |
            (unpackBoolOptional(OFILL_) ? OFILL : 0) | (unpackBoolOptional(OFDEL_) ? OFDEL : 0);

        if (NLDLY_)
        {
            if (*NLDLY_ == "NL0")
                flags |= NL0;
            else if (*NLDLY_ == "NL1")
                flags |= NL1;
        }

        if (CRDLY_)
        {
            if (*CRDLY_ == "CR0")
                flags |= CR0;
            else if (*CRDLY_ == "CR1")
                flags |= CR1;
            else if (*CRDLY_ == "CR2")
                flags |= CR2;
            else if (*CRDLY_ == "CR3")
                flags |= CR3;
        }

        if (TABDLY_)
        {
            if (*TABDLY_ == "TAB0")
                flags |= TAB0;
            else if (*TABDLY_ == "TAB1")
                flags |= TAB1;
            else if (*TABDLY_ == "TAB2")
                flags |= TAB2;
            else if (*TABDLY_ == "TAB3")
                flags |= TAB3;
        }

        if (BSDLY_)
        {
            if (*BSDLY_ == "BS0")
                flags |= BS0;
            else if (*BSDLY_ == "BS1")
                flags |= BS1;
        }

        if (VTDLY_)
        {
            if (*VTDLY_ == "VT0")
                flags |= VT0;
            else if (*VTDLY_ == "VT1")
                flags |= VT1;
        }

        if (FFDLY_)
        {
            if (*FFDLY_ == "FF0")
                flags |= FF0;
            else if (*FFDLY_ == "FF1")
                flags |= FF1;
        }

        return flags;
#endif
    }

    void Termios::OutputFlags::useDefaultsFrom(OutputFlags const& other)
    {
        if (!OPOST_)
            OPOST_ = other.OPOST_;
        if (!OLCUC_)
            OLCUC_ = other.OLCUC_;
        if (!ONLCR_)
            ONLCR_ = other.ONLCR_;
        if (!OCRNL_)
            OCRNL_ = other.OCRNL_;
        if (!ONOCR_)
            ONOCR_ = other.ONOCR_;
        if (!ONLRET_)
            ONLRET_ = other.ONLRET_;
        if (!OFILL_)
            OFILL_ = other.OFILL_;
        if (!OFDEL_)
            OFDEL_ = other.OFDEL_;
        if (!NLDLY_)
            NLDLY_ = other.NLDLY_;
        if (!CRDLY_)
            CRDLY_ = other.CRDLY_;
        if (!TABDLY_)
            TABDLY_ = other.TABDLY_;
        if (!BSDLY_)
            BSDLY_ = other.BSDLY_;
        if (!VTDLY_)
            VTDLY_ = other.VTDLY_;
        if (!FFDLY_)
            FFDLY_ = other.FFDLY_;
    }

    unsigned int Termios::ControlFlags::assemble() const
    {
#ifdef _WIN32
        return 0;
#else
        unsigned int flags = 0 | (CBAUD_ ? *CBAUD_ : 0) | (unpackBoolOptional(CBAUDEX_) ? CBAUDEX : 0) |
            (unpackBoolOptional(CSTOPB_) ? CSTOPB : 0) | (unpackBoolOptional(CREAD_) ? CREAD : 0) |
            (unpackBoolOptional(PARENB_) ? PARENB : 0) | (unpackBoolOptional(PARODD_) ? PARODD : 0) |
            (unpackBoolOptional(HUPCL_) ? HUPCL : 0) | (unpackBoolOptional(CLOCAL_) ? CLOCAL : 0);

        if (CSIZE_)
        {
            if (*CSIZE_ == "CS5")
                flags |= CS5;
            else if (*CSIZE_ == "CS6")
                flags |= CS6;
            else if (*CSIZE_ == "CS7")
                flags |= CS7;
            else if (*CSIZE_ == "CS8")
                flags |= CS8;
        }

        return flags;
#endif
    }

    void Termios::ControlFlags::useDefaultsFrom(ControlFlags const& other)
    {
        if (!CBAUD_)
            CBAUD_ = other.CBAUD_;
        if (!CBAUDEX_)
            CBAUDEX_ = other.CBAUDEX_;
        if (!CSIZE_)
            CSIZE_ = other.CSIZE_;
        if (!CSTOPB_)
            CSTOPB_ = other.CSTOPB_;
        if (!CREAD_)
            CREAD_ = other.CREAD_;
        if (!PARENB_)
            PARENB_ = other.PARENB_;
        if (!PARODD_)
            PARODD_ = other.PARODD_;
        if (!HUPCL_)
            HUPCL_ = other.HUPCL_;
        if (!CLOCAL_)
            CLOCAL_ = other.CLOCAL_;
        if (!LOBLK_)
            LOBLK_ = other.LOBLK_;
    }

    unsigned int Termios::LocalFlags::assemble() const
    {
#ifdef _WIN32
        return 0;
#else
        return 0 | (unpackBoolOptional(ISIG_) ? ISIG : 0) | (unpackBoolOptional(ICANON_) ? ICANON : 0) |
            (unpackBoolOptional(XCASE_) ? XCASE : 0) | (unpackBoolOptional(ECHO_) ? ECHO : 0) |
            (unpackBoolOptional(ECHOE_) ? ECHOE : 0) | (unpackBoolOptional(ECHOK_) ? ECHOK : 0) |
            (unpackBoolOptional(ECHONL_) ? ECHONL : 0) | (unpackBoolOptional(ECHOCTL_) ? ECHOCTL : 0) |
            (unpackBoolOptional(ECHOPRT_) ? ECHOPRT : 0) | (unpackBoolOptional(ECHOKE_) ? ECHOKE : 0) |
            (unpackBoolOptional(FLUSHO_) ? FLUSHO : 0) | (unpackBoolOptional(NOFLSH_) ? NOFLSH : 0) |
            (unpackBoolOptional(TOSTOP_) ? TOSTOP : 0) | (unpackBoolOptional(PENDIN_) ? PENDIN : 0) |
            (unpackBoolOptional(IEXTEN_) ? IEXTEN : 0);
#endif
    }

    void Termios::LocalFlags::useDefaultsFrom(LocalFlags const& other)
    {
        if (!ISIG_)
            ISIG_ = other.ISIG_;
        if (!ICANON_)
            ICANON_ = other.ICANON_;
        if (!XCASE_)
            XCASE_ = other.XCASE_;
        if (!ECHO_)
            ECHO_ = other.ECHO_;
        if (!ECHOE_)
            ECHOE_ = other.ECHOE_;
        if (!ECHOK_)
            ECHOK_ = other.ECHOK_;
        if (!ECHONL_)
            ECHONL_ = other.ECHONL_;
        if (!ECHOCTL_)
            ECHOCTL_ = other.ECHOCTL_;
        if (!ECHOPRT_)
            ECHOPRT_ = other.ECHOPRT_;
        if (!ECHOKE_)
            ECHOKE_ = other.ECHOKE_;
        if (!FLUSHO_)
            FLUSHO_ = other.FLUSHO_;
        if (!NOFLSH_)
            NOFLSH_ = other.NOFLSH_;
        if (!TOSTOP_)
            TOSTOP_ = other.TOSTOP_;
        if (!PENDIN_)
            PENDIN_ = other.PENDIN_;
        if (!IEXTEN_)
            IEXTEN_ = other.IEXTEN_;
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

#define TO_JSON_OPTIONAL_NO_UNDERSCORE(j, termios, field) TO_JSON_OPTIONAL_RENAME(j, termios, field##_, #field)

#define FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, termios, field) FROM_JSON_OPTIONAL_RENAME(j, termios, field##_, #field)

    void to_json(nlohmann::json& j, Termios::InputFlags const& inputFlags)
    {
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IGNBRK);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, BRKINT);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IGNPAR);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, PARMRK);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, INPCK);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, ISTRIP);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, INLCR);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IGNCR);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, ICRNL);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IUCLC);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IXON);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IXANY);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IXOFF);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IMAXBEL);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IUTF8);
    }
    void from_json(nlohmann::json const& j, Termios::InputFlags& inputFlags)
    {
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IGNBRK);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, BRKINT);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IGNPAR);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, PARMRK);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, INPCK);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, ISTRIP);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, INLCR);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IGNCR);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, ICRNL);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IUCLC);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IXON);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IXANY);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IXOFF);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IMAXBEL);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, inputFlags, IUTF8);
    }
    void to_json(nlohmann::json& j, Termios::OutputFlags const& outputFlags)
    {
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, OPOST);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, OLCUC);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, ONLCR);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, OCRNL);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, ONOCR);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, ONLRET);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, OFILL);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, OFDEL);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, NLDLY);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, CRDLY);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, TABDLY);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, BSDLY);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, VTDLY);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, FFDLY);
    }
    void from_json(nlohmann::json const& j, Termios::OutputFlags& outputFlags)
    {
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, OPOST);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, OLCUC);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, ONLCR);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, OCRNL);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, ONOCR);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, ONLRET);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, OFILL);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, OFDEL);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, NLDLY);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, CRDLY);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, TABDLY);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, BSDLY);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, VTDLY);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, outputFlags, FFDLY);
    }
    void to_json(nlohmann::json& j, Termios::ControlFlags const& controlFlags)
    {
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CBAUD);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CBAUDEX);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CSIZE);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CSTOPB);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CREAD);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, PARENB);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, PARODD);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, HUPCL);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CLOCAL);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, LOBLK);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CIBAUD);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CMSPAR);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CRTSCTS);
    }
    void from_json(nlohmann::json const& j, Termios::ControlFlags& controlFlags)
    {
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CBAUD);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CBAUDEX);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CSIZE);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CSTOPB);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CREAD);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, PARENB);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, PARODD);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, HUPCL);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CLOCAL);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, LOBLK);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CIBAUD);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CMSPAR);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, controlFlags, CRTSCTS);
    }
    void to_json(nlohmann::json& j, Termios::LocalFlags const& localFlags)
    {
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ISIG);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ICANON);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, XCASE);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHO);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHOE);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHOK);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHONL);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHOCTL);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHOPRT);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHOKE);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, FLUSHO);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, NOFLSH);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, TOSTOP);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, PENDIN);
        TO_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, IEXTEN);
    }
    void from_json(nlohmann::json const& j, Termios::LocalFlags& localFlags)
    {
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ISIG);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ICANON);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, XCASE);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHO);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHOE);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHOK);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHONL);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHOCTL);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHOPRT);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, ECHOKE);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, FLUSHO);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, NOFLSH);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, TOSTOP);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, PENDIN);
        FROM_JSON_OPTIONAL_NO_UNDERSCORE(j, localFlags, IEXTEN);
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
        TO_JSON_OPTIONAL(j, termios, inputFlags);
        TO_JSON_OPTIONAL(j, termios, outputFlags);
        TO_JSON_OPTIONAL(j, termios, controlFlags);
        TO_JSON_OPTIONAL(j, termios, localFlags);
        TO_JSON_OPTIONAL(j, termios, cc);
        TO_JSON_OPTIONAL(j, termios, iSpeed);
        TO_JSON_OPTIONAL(j, termios, oSpeed);
    }
    void from_json(nlohmann::json const& j, Termios& termios)
    {
        termios = Termios{};

        FROM_JSON_OPTIONAL(j, termios, inputFlags);
        FROM_JSON_OPTIONAL(j, termios, outputFlags);
        FROM_JSON_OPTIONAL(j, termios, controlFlags);
        FROM_JSON_OPTIONAL(j, termios, localFlags);
        FROM_JSON_OPTIONAL(j, termios, cc);
        FROM_JSON_OPTIONAL(j, termios, iSpeed);
        FROM_JSON_OPTIONAL(j, termios, oSpeed);
    }

    void Termios::useDefaultsFrom(Termios const& other)
    {
        if (inputFlags)
            inputFlags->useDefaultsFrom(other.inputFlags.value());
        else
            inputFlags = other.inputFlags;

        if (outputFlags)
            outputFlags->useDefaultsFrom(other.outputFlags.value());
        else
            outputFlags = other.outputFlags;

        if (controlFlags)
            controlFlags->useDefaultsFrom(other.controlFlags.value());
        else
            controlFlags = other.controlFlags;

        if (localFlags)
            localFlags->useDefaultsFrom(other.localFlags.value());
        else
            localFlags = other.localFlags;

        if (!cc)
            cc = other.cc;

        if (!iSpeed)
            iSpeed = other.iSpeed;
        if (!oSpeed)
            oSpeed = other.oSpeed;
    }

    Termios Termios::saneDefaults()
    {
        Termios termios{};
        termios.inputFlags = InputFlags::saneDefaults();
        termios.outputFlags = OutputFlags::saneDefaults();
        termios.controlFlags = ControlFlags::saneDefaults();
        termios.localFlags = LocalFlags::saneDefaults();
        termios.cc = CC{};
        termios.iSpeed = 0;
        termios.oSpeed = 0;
        return termios;
    }
}