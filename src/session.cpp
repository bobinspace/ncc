#include "session.h"
#include "logging.h"

Session::Session(const int fd, const size_t read_threshold, const size_t write_threshold)
	: fd(fd)
	, socket_reader(fd, &io_benchmark)
	, read_threshold(read_threshold)
	, socket_writer(fd, &io_benchmark)
	, write_threshold(write_threshold)
	, ack_maker_and_serialiser(serialiser, io_benchmark)
{
	log::PrintLn(log::Debug, "NEW Session:%p", (void*)this);
}

Session::~Session() {
	log::PrintLn(log::Debug, "DEL Session:%p", (void*)this);
}

void Session::Reset() {
	fd = -1;

	serialiser.Reset();
	socket_writer.Reset();

	deserialiser.Reset();
	socket_reader.Reset();
}

bool Session::IsValid() const {
	return -1 != fd;
}
