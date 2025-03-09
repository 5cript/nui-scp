#pragma once

#include <ids/ids.hpp>
#include <ssh/file_stream.hpp>

class FileOperationQueue
{
  public:
    FileOperationQueue();

    void pause();
    void resume();
    void cancelAll(bool clean);

  private:
};