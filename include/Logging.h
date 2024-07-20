#ifndef LOGGING_H
#define LOGGING_H

#include <iostream>
#include <streambuf>
#include <vector>
#include <emscripten/emscripten.h>

enum class LogLevel {
	INFO,
	WARNING,
	ERROR
};

class JSStreambuf : public std::streambuf {
public:
	JSStreambuf(LogLevel level);
protected:
	virtual int overflow(int c) override;
	virtual std::streamsize xsputn(const char* s, std::streamsize n) override;
private:
	LogLevel level;
	std::vector<char> buffer;
	void flushBuffer();
	void logToJs(const std::string& message);
};

extern std::ostream js_info;
extern std::ostream js_warning;
extern std::ostream js_error;

#endif // LOGGING_H
