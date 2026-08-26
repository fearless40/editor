#include "commandbuffer.hpp"
#include <sys/poll.h>

namespace term {
bool CommandBuffer::read_response(const CommandBuffer &command,
                                  CommandBuffer &response) {

  command.submit();

  bool success{false};
  response.clear();
  char value;

  pollfd pdata;

  pdata.fd = STDIN_FILENO;
  pdata.events = POLLIN;

  // Wait for 100 ms to get data back.
  if (auto ret = poll(&pdata, 1, 100); ret == -1) {
    // Some error occurded terminate!
    return false;
  }

  if (pdata.revents & POLLIN) {
    // Read the data from stdin

    auto amount = read(STDIN_FILENO, response.data(), response.capacity());
    if (amount <= 0)
      return false;
  }

  return true;
}

} // namespace term
