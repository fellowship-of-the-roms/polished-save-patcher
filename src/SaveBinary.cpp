// Include necessary headers
#include <iostream>
#include <fstream>
#include "PatcherConstants.h"
#include "SaveBinary.h"
#include "Logging.h"

// Constructor
SaveBinary::SaveBinary(const std::string& saveFilePath) : m_locked(false) {
	// Open the file
	std::ifstream file(saveFilePath, std::ios::binary);
	if (!file.is_open()) {
		js_error <<  "Failed to open save file: " << saveFilePath << std::endl;
		return;
	}

	// Read the file size
	file.seekg(0, std::ios::end);
	auto fileSize = file.tellg();
	if (fileSize < MIN_SAVE_SIZE) {
		js_error <<  "Save file size is too small: " << saveFilePath << ". Expected minimum size: " << MIN_SAVE_SIZE << ", Actual size: " << fileSize << std::endl;
		file.close();
		return;
	}

	// Resize buffer and read the file
	m_data.resize(fileSize);
	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char*>(m_data.data()), m_data.size());
	file.close();
}

// Destructor
SaveBinary::~SaveBinary() {
}

// Get the byte at the specified address
uint8_t SaveBinary::getByte(uint32_t address) const {
	if (address >= m_data.size()) {
		js_error <<  "Address out of bounds: " << std::hex << address << std::endl;
		return 0;
	}
	return m_data[address];
}

// get the word at the specified address (little endian)
uint16_t SaveBinary::getWord(uint32_t address) const {
	if (address + 1 >= m_data.size()) {
		js_error <<  "Address out of bounds: " << std::hex << address << std::endl;
		return 0;
	}
	return m_data[address] | (m_data[address + 1] << 8);
}

// get the word at the specified address (big endian)
uint16_t SaveBinary::getWordBE(uint32_t address) const {
	if (address + 1 >= m_data.size()) {
		js_error <<  "Address out of bounds: " << std::hex << address << std::endl;
		return 0;
	}
	return (m_data[address] << 8) | m_data[address + 1];
}

// update the byte at the specified address
void SaveBinary::setByte(uint32_t address, uint8_t value) {
	// error if locked
	if (m_locked) {
		js_error <<  "Save file is locked" << std::endl;
		return;
	}
	// error if address out of bounds
	if (address >= m_data.size()) {
		js_error <<  "Address out of bounds: " << std::hex << address << std::endl;
		return;
	}
	m_data[address] = value;
}

// update the word at the specified address (little endian)
void SaveBinary::setWord(uint32_t address, uint16_t value) {
	// error if locked
	if (m_locked) {
		js_error <<  "Save file is locked" << std::endl;
		return;
	}
	// error if address out of bounds
	if (address + 1 >= m_data.size()) {
		js_error <<  "Address out of bounds: " << std::hex << address << std::endl;
		return;
	}
	m_data[address] = value & 0xFF;
	m_data[address + 1] = (value >> 8) & 0xFF;
}

// update the word at the specified address (big endian)
void SaveBinary::setWordBE(uint32_t address, uint16_t value) {
	// error if locked
	if (m_locked) {
		js_error <<  "Save file is locked" << std::endl;
		return;
	}
	// error if address out of bounds
	if (address + 1 >= m_data.size()) {
		js_error <<  "Address out of bounds: " << std::hex << address << std::endl;
		return;
	}
	m_data[address] = (value >> 8) & 0xFF;
	m_data[address + 1] = value & 0xFF;
}

// lock the class to prevent further modification
void SaveBinary::lock() {
	m_locked = true;
}

// unlock the class to allow modification
void SaveBinary::unlock() {
	m_locked = false;
}

// save the data to the specified file
void SaveBinary::save(const std::string& saveFilePath) const {
	// Open the file
	std::ofstream file(saveFilePath, std::ios::binary);
	if (!file.is_open()) {
		js_error <<  "Failed to open save file: " << saveFilePath << std::endl;
		return;
	}
	// Write the data
	file.write(reinterpret_cast<const char*>(m_data.data()), m_data.size());
	file.close();
}

// Iterator constructor
SaveBinary::Iterator::Iterator(SaveBinary& saveBinary, uint32_t address) : m_saveBinary(saveBinary), m_address(address) {
}

// Iterator destructor
SaveBinary::Iterator::~Iterator() {
}

// Get the byte at the current address
uint8_t SaveBinary::Iterator::getByte() const {
	return m_saveBinary.getByte(m_address);
}

// Get the word at the current address (little endian)
uint16_t SaveBinary::Iterator::getWord() const {
	return m_saveBinary.getWord(m_address);
}

// Get the word at the current address (big endian)
uint16_t SaveBinary::Iterator::getWordBE() const {
	return m_saveBinary.getWordBE(m_address);
}

// Update the byte at the current address
void SaveBinary::Iterator::setByte(uint8_t value) {
	m_saveBinary.setByte(m_address, value);
}

// Update the word at the current address (little endian)
void SaveBinary::Iterator::setWord(uint16_t value) {
	m_saveBinary.setWord(m_address, value);
}

// Update the word at the current address (big endian)
void SaveBinary::Iterator::setWordBE(uint16_t value) {
	m_saveBinary.setWordBE(m_address, value);
}

// Move the iterator to the next byte
void SaveBinary::Iterator::next() {
	++m_address;
}

// Move the iterator to the previous byte
void SaveBinary::Iterator::prev() {
	--m_address;
}

// Move the iterator to the specified address
void SaveBinary::Iterator::seek(uint32_t address) {
	if (m_address == address) {
		//js_error <<  "Iterator is already at the requested address: " << std::hex << address << std::endl;
		return;
	}
	m_address = address;
}

// Get the current address
uint32_t SaveBinary::Iterator::getAddress() const {
	return m_address;
}

// Check if the iterator is at the end
bool SaveBinary::Iterator::isEnd() const {
	return m_address >= m_saveBinary.m_data.size();
}

// copy specified number of bytes from input iterator to this iterator starting at current address
void SaveBinary::Iterator::copy(Iterator& it, uint32_t numBytes) {
	// copy bytes
	for (uint32_t i = 0; i < numBytes; ++i) {
		if (it.isEnd()) {
			break;
		}
		setByte(it.getByte());
		it.next();
		next();
	}
}

// copy specified number of bytes from input iterator to this iterator starting at specified address
void SaveBinary::Iterator::copy(Iterator& it, uint32_t address, uint32_t numBytes) {
	// move to specified address
	seek(address);
	// copy bytes
	copy(it, numBytes);
}

// Return the current address
uint32_t SaveBinary::Iterator::getAddress() {
	return m_address;
}