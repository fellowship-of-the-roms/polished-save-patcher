#ifndef LOGGING_H
#define LOGGING_H

#include <iostream>
#include <streambuf>
#include <vector>

#ifndef CLI_VERSION
#include <emscripten/emscripten.h>
#endif

enum class LogLevel {
	INFO,
	WARNING,
	ERROR
};

class JSStreambuf : public std::streambuf {
public:
	JSStreambuf(LogLevel level); // constructor
protected:
	virtual int overflow(int c) override; // called when the buffer is full
	virtual std::streamsize xsputn(const char* s, std::streamsize n) override; // called when writing a string
private:
	LogLevel level;
	std::vector<char> buffer;
	void flushBuffer();
	void logToJs(const std::string& message);
};

extern std::ostream js_info; // output stream for info messages
extern std::ostream js_warning; // output stream for warning messages
extern std::ostream js_error; // output stream for error messages

#endif // LOGGING_H
