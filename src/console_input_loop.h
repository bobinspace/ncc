#pragma once
#include <iostream>
#include "session.h"

template<typename FrameHandler>
void RunConsoleInputLoop(FrameHandler& frame_handler) {
	Header header = { 0, MsgType_VariableLength };
	char const* const pHeader = (char const*)(&header);
	std::string line(pHeader, pHeader + sizeof(header));
	std::vector<char> v;
	while (1) {
		std::getline(std::cin, line);
		const size_t num_char_in_line = line.size();
		if (num_char_in_line < 1) continue;

		header.length = sizeof(header) + num_char_in_line;
		if (header.length > v.size()) {
			v.resize(header.length);
		}
		memcpy(&v[0], (char const*)(&header), sizeof(header));
		memcpy(&v[sizeof(header)], line.c_str(), num_char_in_line);
		if (!frame_handler.HandleFrame(&v[0], header.length)) {
			return;
		}
	}
}
