#include "Logging.h"

// Declare an external JavaScript function to log info messages
EM_JS(void, js_log_info, (const char* str), {
	// convert the C string to a JavaScript string and trim any whitespace
	var msg = UTF8ToString(str).trim();
	// log the message to the JavaScript console
	console.log(msg);
	// Find the HTML element with the ID 'output'
	var outputElement = document.getElementById('output');
	if (outputElement) {
		// Append the message to the inner HTML of the output element
		outputElement.innerHTML += msg + '\n';
	}
});

EM_JS(void, js_log_warning, (const char* str), {
	var msg = UTF8ToString(str).trim();
	console.warn(msg);
	var outputElement = document.getElementById('output');
	if (outputElement) {
		// Append the message to the inner HTML of the output element, wrapped in a span with the 'warning' class
		outputElement.innerHTML += '<span class="warning">' + msg + '</span>\n';
	}
});

EM_JS(void, js_log_error, (const char* str), {
	var msg = UTF8ToString(str).trim();
	console.error(msg);
	var outputElement = document.getElementById('output');
	if (outputElement) {
		// Append the message to the inner HTML of the output element, wrapped in a span with the 'error' class
		outputElement.innerHTML += '<span class="error">' + msg + '</span>\n';
	}
});

// Constructor for JSStreambuf, initializes the log level and reserves space in the buffer.
JSStreambuf::JSStreambuf(LogLevel level) : level(level) {
	buffer.reserve(128);
}

// Handles overflowing characters in the stream buffer.
int JSStreambuf::overflow(int c) {
	if (c != EOF) {
		// Add the character to the buffer.
		buffer.push_back(c);
		// If the character is a newline, flush the buffer.
		if (c == '\n') {
			flushBuffer();
		}
	}
	return c;
}

// Writes a sequence of characters to the stream buffer.
std::streamsize JSStreambuf::xsputn(const char* s, std::streamsize n) {
	for (std::streamsize i = 0; i < n; ++i) {
		// Add each character to the buffer using overflow.
		overflow(s[i]);
	}
	return n;
}

// Flushes the buffer by sending its content to the JavaScript logging functions.
void JSStreambuf::flushBuffer() {
	if (!buffer.empty()) {
		// Null-terminate the buffer.
		buffer.push_back('\0');
		// Log the message to JavaScript.
		logToJs(buffer.data());
		// Clear the buffer after sending.
		buffer.clear();
	}
}

// Logs the message to the appropriate JavaScript function based on the log level.
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

// Create JSStreambuf objects for different log levels.
JSStreambuf js_info_buf(LogLevel::INFO);
JSStreambuf js_warning_buf(LogLevel::WARNING);
JSStreambuf js_error_buf(LogLevel::ERROR);

// Create std::ostream objects that use the JSStreambuf objects for logging.
std::ostream js_info(&js_info_buf);
std::ostream js_warning(&js_warning_buf);
std::ostream js_error(&js_error_buf);
