#pragma once
#include <unistd.h>
#include <map>
#include <mutex>

struct epoll_event;
class EpollController {
	mutable std::mutex mutex_;
	int fd_epoll_instance_;
	std::map<int, uint32_t> watched_fds_to_events_;
	
	bool GetEventsOfInterests(const int fd, uint32_t& events_of_interest) const;
	bool UpdateEventsOfInterests(const int fd, const uint32_t events_of_interest);
	int ClearInterestList();
public:
	EpollController();
	~EpollController();

	// 0 on success, or –1 on error
	int AddToInterestList(const int fd_of_interest, const uint32_t events_of_interest);
	bool ModifyInterestList(const int fd_of_interest, const uint32_t events_of_interest_to_add, const uint32_t events_of_interest_to_remove, int* const epoll_ctl_status_ptr = 0);
	int RemoveFromInterestList(const int fd_to_remove);

	// Returns last bad errno instead of last bad epoll_ctl return value
	int WaitForEvents(epoll_event* const ready_events, const int max_events, const int timeout);
	size_t NumWatchedFds() const;
};
