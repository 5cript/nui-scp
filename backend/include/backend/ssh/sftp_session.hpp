#pragma once

#include <libssh/sftp.h>

class SftpSession
{
  public:
    SftpSession();

  private:
    sftp_session session_;
};