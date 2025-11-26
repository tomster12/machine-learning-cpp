#include <atomic>

static std::atomic<int> tbmlOmpThreads{ 1 };

namespace tbml
{
	void setOmpThreads(int n) { tbmlOmpThreads = n; }
	int getOmpThreads() { return tbmlOmpThreads.load(); }
}
