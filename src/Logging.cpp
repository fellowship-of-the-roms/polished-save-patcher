#include "Logging.h"

// Declare an external JavaScript function to log messages
EM_JS(void, js_log_message, (const char* str, const char* level), {
	// Convert the C string to a JavaScript string and trim any whitespace
	var msg = UTF8ToString(str).trim();
	var lvl = UTF8ToString(level);
	// Call the existing logMessage function defined in index.html
	if (typeof logMessage === 'function') {
		logMessage(msg, lvl);
	} else {
		console.warn("logMessage function not found.");
	}
});

// Constructor for JSStreambuf, initializes the log level and reserves space in the buffer
JSStreambuf::JSStreambuf(LogLevel level) : level(level) {
	buffer.reserve(128);
}

// Handles overflowing characters in the stream buffer
int JSStreambuf::overflow(int c) {
	if (c != EOF) {
		// Add the character to the buffer
		buffer.push_back(c);
		// If the character is a newline, flush the buffer
		if (c == '\n') {
			flushBuffer();
		}
	}
	return c;
}

// Writes a sequence of characters to the stream buffer
std::streamsize JSStreambuf::xsputn(const char* s, std::streamsize n) {
	for (std::streamsize i = 0; i < n; ++i) {
		// Add each character to the buffer using overflow
		overflow(s[i]);
	}
	return n;
}

// Flushes the buffer by sending its content to the JavaScript logging functions
void JSStreambuf::flushBuffer() {
	if (!buffer.empty()) {
		// Null-terminate the buffer
		buffer.push_back('\0');
		// Log the message to JavaScript
		logToJs(buffer.data());
		// Clear the buffer after sending
		buffer.clear();
	}
}

// Logs the message to the appropriate JavaScript function based on the log level
void JSStreambuf::logToJs(const std::string& message) {
	switch (level) {
		case LogLevel::INFO:
			js_log_message(message.c_str(), "info");
			break;
		case LogLevel::WARNING:
			js_log_message(message.c_str(), "warning");
			break;
		case LogLevel::ERROR:
			js_log_message(message.c_str(), "error");
			break;
	}
}

// Create JSStreambuf objects for different log levels
JSStreambuf js_info_buf(LogLevel::INFO);
JSStreambuf js_warning_buf(LogLevel::WARNING);
JSStreambuf js_error_buf(LogLevel::ERROR);

// Create std::ostream objects that use the JSStreambuf objects for logging
std::ostream js_info(&js_info_buf);
std::ostream js_warning(&js_warning_buf);
std::ostream js_error(&js_error_buf);
