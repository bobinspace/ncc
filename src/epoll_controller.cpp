#include "epoll_controller.h"
#include "socket_utils.h"
#include <sys/epoll.h>
#include "logging.h"

EpollController::EpollController()
    : fd_epoll_instance_(epoll_create1(0))
{}

EpollController::~EpollController() {
    ClearInterestList();
    if (fd_epoll_instance_ >= 0) {
        close(fd_epoll_instance_);
    }
}

// 0 on success, or 1 on error
int EpollController::AddToInterestList(const int fd_of_interest, const uint32_t events_of_interest) {
    std::lock_guard<std::mutex> lock(mutex_);
    epoll_event event = { 0 };
    event.events = events_of_interest;
    event.data.fd = fd_of_interest;
    const int e = epoll_ctl(fd_epoll_instance_, EPOLL_CTL_ADD, fd_of_interest, &event);
    if (0 == e) {
        char events_string[512] = { 0 };
        EpollEventsToString(events_of_interest, events_string, sizeof(events_string));
        log::PrintLn(log::Debug, "%d|+|%s", fd_of_interest, events_string);
        watched_fds_to_events_[fd_of_interest] = events_of_interest;
    }
    return e;
}

bool EpollController::GetEventsOfInterests(const int fd, uint32_t& events_of_interest) const {
    auto it = watched_fds_to_events_.find(fd);
    if (watched_fds_to_events_.end() == it) return false;
    events_of_interest = it->second;
    return true;
}

bool EpollController::UpdateEventsOfInterests(const int fd, const uint32_t events_of_interest) {
    auto it = watched_fds_to_events_.find(fd);
    if (watched_fds_to_events_.end() == it) return false;
    it->second = events_of_interest;
    return true;
}

bool EpollController::ModifyInterestList(const int fd_of_interest, const uint32_t events_of_interest_to_add, const uint32_t events_of_interest_to_remove, int * const epoll_ctl_status_ptr) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t existing_registered_events = 0;
    if (!GetEventsOfInterests(fd_of_interest, existing_registered_events)) {
        return false;
    }
    const bool is_already_in_desired_state = 
        ((0 == events_of_interest_to_add) || (existing_registered_events & events_of_interest_to_add))
        && ((0 == events_of_interest_to_remove) || (!(existing_registered_events & events_of_interest_to_remove)));

    log::PrintLn(log::Debug, "%d|?|existing=%08x add=%08x del=%08x nochg=%d", fd_of_interest, existing_registered_events, events_of_interest_to_add, events_of_interest_to_remove, is_already_in_desired_state);
    if (is_already_in_desired_state) return true;

    epoll_event event = { 0 };
    event.events = (existing_registered_events | events_of_interest_to_add) & (~events_of_interest_to_remove);
    event.data.fd = fd_of_interest;
    const int e = epoll_ctl(fd_epoll_instance_, EPOLL_CTL_MOD, fd_of_interest, &event);
    if (epoll_ctl_status_ptr) {
        *epoll_ctl_status_ptr = e;
    }
    if (0 == e) {
        char events_string[512] = { 0 };
        EpollEventsToString(event.events, events_string, sizeof(events_string));
        log::PrintLn(log::Debug, "%d|+|%s", fd_of_interest, events_string);
        UpdateEventsOfInterests(fd_of_interest, event.events);
    }
    return e;
}

int EpollController::RemoveFromInterestList(const int fd_to_remove) {
    std::lock_guard<std::mutex> lock(mutex_);
    // ATTENTION: EPOLL_CTL_DEL has no effect on closed fds.
    // Therefore, ensure that EPOLL_CTL_DEL is done on the fd before closing it.
    const int e = epoll_ctl(fd_epoll_instance_, EPOLL_CTL_DEL, fd_to_remove, NULL);
    if (0 == e) {
        watched_fds_to_events_.erase(fd_to_remove);
        log::PrintLn(log::Debug, "%d|-", fd_to_remove);
    }
    return e;
}

// Returns last bad errno instead of last bad epoll_ctl return value
int EpollController::ClearInterestList() {
    int last_bad_errno = 0;
    for (const auto& [watched_fd, events]: watched_fds_to_events_) {
        const int e = RemoveFromInterestList(watched_fd);
        if (e < 0) {
            last_bad_errno = e;
        }
    }
    return last_bad_errno;
}

int EpollController::WaitForEvents(epoll_event* const ready_events, const int max_events, const int timeout) {
    return epoll_wait(fd_epoll_instance_, ready_events, max_events, timeout);
}

size_t EpollController::NumWatchedFds() const {
    return watched_fds_to_events_.size();
}

