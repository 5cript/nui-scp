#pragma once

#include "common_fixture.hpp"
#include <ssh/session.hpp>
#include <nui/utility/scope_exit.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace std::string_literals;

namespace SecureShell::Test
{
    class SftpTests : public CommonFixture
    {
      public:
        static constexpr std::chrono::seconds connectTimeout{2};

      protected:
    };

    TEST_F(SftpTests, CanCreateSftpSession)
    {
        // SecureShell::Session client{};
        // SecureShell::SftpSession sftp{&client, nullptr};
    }
}
