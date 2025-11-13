#include "outgoing_buffer.h"

#include "../lockfree.h"

namespace tfs::game_server {

std::shared_ptr<OutgoingBuffer> make_outgoing_buffer()
{
	return std::allocate_shared<OutgoingBuffer>(LockfreePoolingAllocator<void, 2048>());
}

} // namespace tfs::game_server
