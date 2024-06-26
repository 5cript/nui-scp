#include <frontend/terminal/terminal.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/utility/functions.hpp>

#include <cctype>

// clang-format off
#ifdef NUI_INLINE
// @inline(js, xterm-js)
js_import { Terminal } from "@xterm/xterm";
js_import { FitAddon } from "@xterm/addon-fit";
js_import { WebglAddon } from "@xterm/addon-webgl";
js_import { nanoid } from "nanoid";

globalThis.terminalUtility = {};
globalThis.terminalUtility.stringToUint8Array = (str) => {
    return new TextEncoder().encode(str);
};
globalThis.terminalUtility.terminals = new Map();
globalThis.terminalUtility.createTerminal = (host) => {
    console.log(host);

    const terminal = new Terminal({
        cursorBlink: true,
        fontSize: 14,
        fontFamily: "Consolas",
        theme: {
            background: "#000000",
            foreground: "#FFFFFF",
        },
        cols: 80,
        rows: 30
    });
    const addons = {
        fitAddon: new FitAddon(),
        webglAddon: new WebglAddon()
    };
    for (const [key, value] of Object.entries(addons))
    {
        terminal.loadAddon(value);
    }

    terminal.open(host);
    const id = nanoid();

    // Resizing:
    const resizeObserver = new ResizeObserver((entries) => {
        globalThis.requestAnimationFrame(() => {
            if (!Array.isArray(entries) || !entries.length)
                return;

            globalThis.terminalUtility.refitTerminal(id);
        });
    });
    addons.resizeObserver = resizeObserver;
    addons.resizeObserver.observe(host);

    globalThis.terminalUtility.set(id, terminal, addons);
    return id;
};
globalThis.terminalUtility.getTerminal = (id) => {
    return globalThis.terminalUtility.get(id).terminal;
};
globalThis.terminalUtility.refitTerminal = (id) => {
    const terminalStuff = globalThis.terminalUtility.get(id);
    terminalStuff.addons.fitAddon.fit();
};
globalThis.terminalUtility.disposeTerminal = (id) => {
    const terminalStuff = globalThis.terminalUtility.get(id);
    terminalStuff.addons.resizeObserver.disconnect();
    terminalStuff.terminal.dispose();
    globalThis.terminalUtility.delete(id);
};
globalThis.terminalUtility.set = (id, terminal, addons) => {
    globalThis.terminalUtility.terminals.set(id, {
        terminal: terminal,
        addons: addons
    });
};
globalThis.terminalUtility.get = (id) => {
    return globalThis.terminalUtility.terminals.get(id);
};
// @endinline

// @inline(css, xterm-js-css)
@import "../../node_modules/@xterm/xterm/css/xterm.css";
// @endinline
#endif
// clang-format on

namespace
{
    Nui::val terminalUtility()
    {
        return Nui::val::global("terminalUtility");
    }
    void debugPrintTerminalWrite(std::string const& data, bool isUserInput)
    {
        std::string debugPrint;
        for (auto c : data)
        {
            if (c == '\r')
            {
                debugPrint += "\\r";
            }
            else if (c == '\n')
            {
                debugPrint += "\\n";
            }
            else if (std::isprint(c))
            {
                debugPrint += c;
            }
            else
            {
                debugPrint += "\\x" + std::to_string(static_cast<int>(c));
            }
        }
        if (isUserInput)
        {
            Nui::Console::log("Terminal::sending", debugPrint);
        }
        else
        {
            Nui::Console::log("Terminal::received", debugPrint);
        }
    }
}

struct Terminal::Implementation
{
    std::string termId;
    std::string command;
    std::unique_ptr<TerminalEngine> engine;
    Nui::val terminal() const
    {
        return terminalUtility().call<Nui::val>("getTerminal", termId);
    }

    Implementation(std::unique_ptr<TerminalEngine> engine)
        : termId{}
        , command{}
        , engine{std::move(engine)}
    {}
};

Terminal::Terminal(std::unique_ptr<TerminalEngine> engine)
    : impl_{std::make_unique<Implementation>(std::move(engine))}
{
    impl_->engine->setStdoutHandler([this](std::string const& data) {
        write(data, false);
    });
    impl_->engine->setStderrHandler([this](std::string const& data) {
        // TODO: Add stderr styling mode
        write(data, false);
    });
}
void Terminal::open(Nui::val element)
{
    impl_->termId = terminalUtility().call<std::string>("createTerminal", element);
    impl_->engine->open([this](bool success) {
        if (!success)
        {
            Nui::Console::error("Failed to open terminal");
        }
        Nui::Console::log("Terminal opened");

        impl_->terminal().call<void>(
            "onData",
            Nui::bind(
                [this](Nui::val data, Nui::val) {
                    write(data.as<std::string>(), true);
                },
                std::placeholders::_1,
                std::placeholders::_2));

        impl_->terminal().call<void>(
            "onResize",
            Nui::bind(
                [this](Nui::val obj, Nui::val) {
                    impl_->engine->resize(obj["cols"].as<int>(), obj["rows"].as<int>());
                },
                std::placeholders::_1,
                std::placeholders::_2));
    });
}
void Terminal::dispose()
{
    if (!impl_->termId.empty())
    {
        impl_->terminal().call<void>("disposeTerminal", impl_->termId);
    }
    impl_->termId.clear();
    impl_->engine->dispose();
}
void Terminal::write(std::string const& data, bool isUserInput)
{
    if (data.empty())
        return;

    if (isUserInput)
    {
        impl_->engine->write(data);
        // if (data.front() == '\r' || data.front() == '\n')
        // {
        //     // TODO: trim whitespace
        //     for (std::size_t i = 0; i != impl_->command.size(); ++i)
        //     {
        //         impl_->terminal().call<void>("write", std::string{"\x08 \x08"});
        //     }
        //     impl_->engine->write(impl_->command + data);
        //     debugPrintTerminalWrite(impl_->command, isUserInput);
        //     impl_->command = "";
        //     return;
        // }
        // else if (data.front() < 0x20)
        // {
        //     impl_->engine->write(data);
        //     debugPrintTerminalWrite(data, isUserInput);
        //     return;
        // }
        // else if (data.front() == '\x7f')
        // {
        //     if (!impl_->command.empty())
        //     {
        //         impl_->terminal().call<void>("write", std::string{"\x08 \x08"});
        //         impl_->command.pop_back();
        //     }
        //     else
        //     {
        //         impl_->terminal().call<void>("write", "\a");
        //     }
        //     return;
        // }
        // else
        // {
        //     Nui::Console::log("X: ", data);
        //     impl_->command += data;
        // }
        // impl_->terminal().call<void>("write", data);
    }
    else
    {
        std::string nlFixedData;
        bool previousWasCR = false;
        for (auto c : data)
        {
            if (c == '\n')
            {
                if (!previousWasCR)
                {
                    nlFixedData += '\r';
                }
                previousWasCR = false;
            }
            else if (c == '\r')
            {
                previousWasCR = true;
            }
            nlFixedData += c;
        }
        debugPrintTerminalWrite(nlFixedData, isUserInput);
        impl_->terminal().call<void>("write", nlFixedData);
    }
}
Terminal::~Terminal()
{
    dispose();
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL_NO_DTOR(Terminal);
