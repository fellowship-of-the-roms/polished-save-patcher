#include "Logging.h"

EM_JS(void, js_log_info, (const char* str), {
	var msg = UTF8ToString(str).trim();
	console.log(msg);
	var outputElement = document.getElementById('output');
	if (outputElement) {
		outputElement.innerHTML += msg + '\n';
	}
});

EM_JS(void, js_log_warning, (const char* str), {
	var msg = UTF8ToString(str).trim();
	console.warn(msg);
	var outputElement = document.getElementById('output');
	if (outputElement) {
		outputElement.innerHTML += '<span class="warning">' + msg + '</span>\n';
	}
});

EM_JS(void, js_log_error, (const char* str), {
	var msg = UTF8ToString(str).trim();
	console.error(msg);
	var outputElement = document.getElementById('output');
	if (outputElement) {
		outputElement.innerHTML += '<span class="error">' + msg + '</span>\n';
	}
});

JSStreambuf::JSStreambuf(LogLevel level) : level(level) {
	buffer.reserve(128);
}

int JSStreambuf::overflow(int c) {
	if (c != EOF) {
		buffer.push_back(c);
		if (c == '\n') {
			flushBuffer();
		}
	}
	return c;
}

std::streamsize JSStreambuf::xsputn(const char* s, std::streamsize n) {
	for (std::streamsize i = 0; i < n; ++i) {
		overflow(s[i]);
	}
	return n;
}

void JSStreambuf::flushBuffer() {
	if (!buffer.empty()) {
		buffer.push_back('\0'); // null-terminate the buffer
		logToJs(buffer.data());
		buffer.clear(); // clear the buffer after sending
	}
}

void JSStreambuf::logToJs(const std::string& message) {
	switch (level) {
		case LogLevel::INFO:
			js_log_info(message.c_str());
			break;
		case LogLevel::WARNING:
			js_log_warning(message.c_str());
			break;
		case LogLevel::ERROR:
			js_log_error(message.c_str());
			break;
	}
}

JSStreambuf js_info_buf(LogLevel::INFO);
JSStreambuf js_warning_buf(LogLevel::WARNING);
JSStreambuf js_error_buf(LogLevel::ERROR);

std::ostream js_info(&js_info_buf);
std::ostream js_warning(&js_warning_buf);
std::ostream js_error(&js_error_buf);
