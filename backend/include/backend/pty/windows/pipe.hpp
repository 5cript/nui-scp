#pragma once

#include <roar/detail/pimpl_special_functions.hpp>

#include <memory>
#include <string>
#include <vector>
#include <string_view>
#include <cstdint>

namespace ConPTY
{
    /**
     * @brief Do not reuse
     *
     */
    class Pipe
    {
      public:
        Pipe();
        ROAR_PIMPL_SPECIAL_FUNCTIONS_NO_MOVE(Pipe);

        template <typename HandleT>
        void applyReadWrite(std::invocable<HandleT, HandleT> auto&& func)
        {
            func(*reinterpret_cast<HandleT*>(readSide()), *reinterpret_cast<HandleT*>(writeSide()));
        }

        bool read(std::vector<char>& buffer, std::size_t& bytesRead);
        bool write(std::string_view data);

        void close();
        void closeReadSide();
        void closeWriteSide();

      private:
        void* readSide() const;
        void* writeSide() const;

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}