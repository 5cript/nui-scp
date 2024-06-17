#include <backend/pty/windows/pipe.hpp>

#include <windows.h>
#include <namedpipeapi.h>

namespace ConPTY
{
    struct Pipe::Implementation
    {
        HANDLE readSide;
        HANDLE writeSide;

        bool readClosed{false};
        bool writeClosed{false};
    };

    Pipe::Pipe()
        : impl_(std::make_unique<Implementation>())
    {
        auto result = CreatePipe(&impl_->readSide, &impl_->writeSide, nullptr, 0);
        if (!result)
        {
            throw std::runtime_error{"Failed to create pipe"};
        }
    }
    Pipe::~Pipe()
    {
        close();
    }
    void* Pipe::readSide() const
    {
        return &impl_->readSide;
    }
    void* Pipe::writeSide() const
    {
        return &impl_->writeSide;
    }
    void Pipe::close()
    {
        closeReadSide();
        closeWriteSide();
    }
    void Pipe::closeReadSide()
    {
        if (!impl_->readClosed)
        {
            CloseHandle(impl_->readSide);
            impl_->readClosed = true;
        }
    }
    void Pipe::closeWriteSide()
    {
        if (!impl_->writeClosed)
        {
            CloseHandle(impl_->writeSide);
            impl_->writeClosed = true;
        }
    }
    bool Pipe::read(std::vector<char>& buffer, std::size_t& bytesRead)
    {
        ULONG bytesAvailable{0};
        PeekNamedPipe(impl_->readSide, nullptr, 0, nullptr, &bytesAvailable, nullptr);
        if (bytesAvailable == 0)
        {
            bytesRead = 0;
            return true;
        }

        DWORD bytesReadWord{0};
        const auto res = ReadFile(impl_->readSide, buffer.data(), buffer.size(), &bytesReadWord, nullptr);
        bytesRead = static_cast<std::size_t>(bytesReadWord);
        return res;
    }

    bool Pipe::write(std::string_view data)
    {
        for (std::size_t i = 0; i < data.size();)
        {
            DWORD bytesWritten{0};
            WriteFile(impl_->writeSide, data.data() + i, data.size() - i, &bytesWritten, nullptr);
            i += bytesWritten;
            if (bytesWritten == 0)
            {
                return false;
            }
        }
        return true;
    }
}