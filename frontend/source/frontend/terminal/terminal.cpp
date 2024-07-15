#include <frontend/terminal/terminal.hpp>
#include <log/log.hpp>
#include <frontend/nlohmann_compat.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/api/json.hpp>
#include <nui/frontend/utility/functions.hpp>

#include <cctype>

// clang-format off
#ifdef NUI_INLINE
// @inline(js, xterm-js)
js_import { Terminal } from "@xterm/xterm";
js_import { FitAddon } from "@xterm/addon-fit";
js_import { WebglAddon } from "@xterm/addon-webgl";
js_import { CanvasAddon } from "@xterm/addon-canvas";
js_import { nanoid } from "nanoid";

globalThis.terminalUtility = {};
globalThis.terminalUtility.stringToUint8Array = (str) => {
    return new TextEncoder().encode(str);
};
globalThis.terminalUtility.terminals = new Map();
globalThis.terminalUtility.createTerminal = (host, options) => {

    let renderer = undefined;
    if (options.hasOwnProperty("renderer")) {
        if (options.renderer === "canvas") {
            renderer = new CanvasAddon();
        } else if (options.renderer === "dom") {
            renderer = undefined;
        } else if (options.renderer === "webgl") {
            renderer = new WebglAddon();
        }
    }

    const defaultedOptions = {
        cursorBlink: true,
        fontSize: 14,
        fontFamily: "courier-new, courier, monospace",
        theme: {
            background: "#000000",
            foreground: "#FFFFFF",
        },
        ...options,
        cols: 80,
        rows: 30
    };

    const terminal = new Terminal(defaultedOptions);
    const addons = {
        fitAddon: new FitAddon(),
        rendererAddon: renderer
    };
    for (const [key, value] of Object.entries(addons))
    {
        if (value)
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

    terminal.focus();
    globalThis.terminalUtility.set(id, terminal, addons);
    return id;
};
globalThis.terminalUtility.getTerminal = (id) => {
    if (!globalThis.terminalUtility.terminals.has(id))
        return undefined;
    const obtained = globalThis.terminalUtility.terminals.get(id);
    if (!obtained.hasOwnProperty("terminal"))
        return undefined;
    return obtained.terminal;
};
globalThis.terminalUtility.refitTerminal = (id) => {
    if (!globalThis.terminalUtility.terminals.has(id))
        return;

    const terminalStuff = globalThis.terminalUtility.get(id);
    terminalStuff.addons.fitAddon.fit();
};
globalThis.terminalUtility.disposeTerminal = (id) => {
    if (!globalThis.terminalUtility.terminals.has(id))
        return;

    const terminalStuff = globalThis.terminalUtility.get(id);
    terminalStuff.addons.resizeObserver.disconnect();
    terminalStuff.terminal.dispose();
    globalThis.terminalUtility.terminals.delete(id);
};
globalThis.terminalUtility.set = (id, terminal, addons) => {
    globalThis.terminalUtility.terminals.set(id, {
        terminal: terminal,
        addons: addons
    });
};
globalThis.terminalUtility.get = (id) => {
    if (!globalThis.terminalUtility.terminals.has(id))
        return undefined;
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
            // Never! Could contain sensitive data
            // Log::debug("Terminal::sending", debugPrint);
        }
        else
        {
            Log::debug("Terminal::received", debugPrint);
        }
    }
}

struct Terminal::Implementation
{
    std::string termId;
    std::string command;
    std::unique_ptr<TerminalEngine> engine;
    std::vector<std::pair<std::string, bool>> writeCache;
    std::function<void(std::string const&, bool)> doWrite;

    Nui::val terminal() const
    {
        return terminalUtility().call<Nui::val>("getTerminal", termId);
    }

    void writeRespectingCache(std::string const& data, bool isUserInput);
    void writeAfterCache(std::string const& data, bool isUserInput);

    Implementation(std::unique_ptr<TerminalEngine> engine)
        : termId{}
        , command{}
        , engine{std::move(engine)}
        , writeCache{}
        , doWrite{}
    {}
};

void Terminal::Implementation::writeRespectingCache(std::string const& data, bool isUserInput)
{
    if (termId.empty())
    {
        if (!data.empty())
            writeCache.emplace_back(data, isUserInput);
        return;
    }

    doWrite = [this](std::string const& cacheData, bool cacheIsUserInput) {
        writeAfterCache(cacheData, cacheIsUserInput);
    };

    if (!writeCache.empty())
    {
        for (auto const& [cacheData, cacheIsUserInput] : writeCache)
            doWrite(cacheData, cacheIsUserInput);
        writeCache.clear();
    }

    doWrite(data, isUserInput);
}
void Terminal::Implementation::writeAfterCache(std::string const& data, bool isUserInput)
{
    if (data.empty())
        return;

    if (isUserInput)
    {
        engine->write(data);
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
        auto term = terminal();
        if (term.isUndefined())
        {
            Log::error("Failed to get terminal with id to write to it: '{}", termId);
            return;
        }
        term.call<void>("write", nlFixedData);
    }
}

Terminal::Terminal(std::unique_ptr<TerminalEngine> engine)
    : impl_{std::make_unique<Implementation>(std::move(engine))}
{
    impl_->doWrite = [this](std::string const& data, bool isUserInput) {
        impl_->writeRespectingCache(data, isUserInput);
    };

    impl_->engine->setStdoutHandler([this](std::string const& data) {
        write(data, false);
    });
    impl_->engine->setStderrHandler([this](std::string const& data) {
        // TODO: Add stderr styling mode
        write(data, false);
    });
}

TerminalEngine& Terminal::engine()
{
    return *impl_->engine;
}

bool Terminal::isOpen() const
{
    return !impl_->termId.empty();
}
void Terminal::open(
    Nui::val element,
    Persistence::TerminalOptions const& options,
    std::function<void(bool, std::string const&)> onOpen)
{
    if (isOpen())
        return;
    Log::info("Opening terminal");
    impl_->engine->open([this, onOpen = std::move(onOpen), element, options](bool success, std::string const& info) {
        if (!success)
        {
            Log::error("Failed to open terminal: '{}'", info);
            dispose();
            onOpen(false, info);
            return;
        };

        impl_->termId = terminalUtility().call<std::string>("createTerminal", element, asVal(options));
        auto term = impl_->terminal();
        if (term.isUndefined())
        {
            Log::error("Failed to get terminal with id: '{}", impl_->termId);
            dispose();
            onOpen(false, "Failed to get terminal");
            return;
        }
        Log::info("Terminal opened with id: '{}", impl_->termId);

        term.call<void>(
            "onData",
            Nui::bind(
                [this](Nui::val data, Nui::val) {
                    write(data.as<std::string>(), true);
                },
                std::placeholders::_1,
                std::placeholders::_2));

        term.call<void>(
            "onResize",
            Nui::bind(
                [this](Nui::val obj, Nui::val) {
                    Log::debug("Terminal resized {}:{}. ", obj["cols"].as<int>(), obj["rows"].as<int>());
                    impl_->engine->resize(obj["cols"].as<int>(), obj["rows"].as<int>());
                },
                std::placeholders::_1,
                std::placeholders::_2));

        if (!impl_->writeCache.empty())
            write("", false);
        onOpen(true, info);
    });
}
void Terminal::focus()
{
    if (!isOpen())
        return;
    auto term = impl_->terminal();
    if (term.isUndefined())
    {
        Log::error("Failed to get terminal with id to focus it: '{}", impl_->termId);
        return;
    }
    term.call<void>("focus");
}
void Terminal::dispose()
{
    impl_->engine->dispose();
    if (!impl_->termId.empty())
    {
        auto term = impl_->terminal();
        if (term.isUndefined())
        {
            Log::error("Failed to get terminal with id to dispose it: '{}", impl_->termId);
            return;
        }
        terminalUtility().call<void>("disposeTerminal", impl_->termId);
    }
    impl_->termId.clear();
}
void Terminal::write(std::string const& data, bool isUserInput)
{
    impl_->doWrite(data, isUserInput);
}
Terminal::~Terminal()
{
    dispose();
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL_NO_DTOR(Terminal);
