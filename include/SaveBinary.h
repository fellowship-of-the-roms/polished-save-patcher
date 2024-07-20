#ifndef SAVEBINARY_H
#define SAVEBINARY_H

#include <string>
#include <vector>

// This class is used to store the save binary data. The save file is 2Mib in size.
// it can be initiated by inputing the path to the save file.

class SaveBinary {
public:
	// Constructor
	SaveBinary(const std::string& saveFilePath);
	// Destructor
	~SaveBinary();
	// Get the byte at the specified address
	uint8_t getByte(uint32_t address) const;
	// get the word at the specified address (little endian)
	uint16_t getWord(uint32_t address) const;
	// get the word at the specified address (big endian)
	uint16_t getWordBE(uint32_t address) const;
	// update the byte at the specified address
	void setByte(uint32_t address, uint8_t value);
	// update the word at the specified address (little endian)
	void setWord(uint32_t address, uint16_t value);
	// update the word at the specified address (big endian)
	void setWordBE(uint32_t address, uint16_t value);
	// lock the class to prevent further modification
	void lock();
	// unlock the class to allow modification
	void unlock();
	// save the data to the specified file
	void save(const std::string& saveFilePath) const;

	// Iterator for the save binary data
	class Iterator {
		public:
			// Iterator constructor
			Iterator(SaveBinary& saveBinary, uint32_t address);
			// Iterator destructor
			~Iterator();
			// Get the byte at the current address
			uint8_t getByte() const;
			// Get the word at the current address (little endian)
			uint16_t getWord() const;
			// Get the word at the current address (big endian)
			uint16_t getWordBE() const;
			// Update the byte at the current address
			void setByte(uint8_t value);
			// Update the word at the current address (little endian)
			void setWord(uint16_t value);
			// Update the word at the current address (big endian)
			void setWordBE(uint16_t value);
			// Move the iterator to the next byte
			void next();
			// Move the iterator to the previous byte
			void prev();
			// Move the iterator to the specified address
			void seek(uint32_t address);
			// Get the current address
			uint32_t getAddress() const;
			// Check if the iterator is at the end
			bool isEnd() const;
			// copy specified number of bytes from input iterator to this iterator starting at current address
			void copy(Iterator& it, uint32_t numBytes);
			// copy specified number of bytes from input iterator to this iterator starting at specified address
			void copy(Iterator& it, uint32_t address, uint32_t numBytes);
			// Return the current address
			uint32_t getAddress();
		
		private:
			SaveBinary& m_saveBinary;
			uint32_t m_address;
	};

private:
	std::vector<uint8_t> m_data;
	bool m_locked;
};

#endif // SAVEBINARY_H