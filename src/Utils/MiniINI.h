/*
 * The MIT License (MIT)
 * Copyright (c) 2018 Danijel Durakovic
 *
 * Permission is hereby granted, free of TCHARge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

 ///////////////////////////////////////////////////////////////////////////////
 //
 //  /mINI/ v0.9.12
 //  An INI file reader and writer for the modern age.
 //
 ///////////////////////////////////////////////////////////////////////////////
 //
 //  A tiny utility library for manipulating INI files with a straightforward
 //  API and a minimal footprint. It conforms to the (somewhat) standard INI
 //  format - sections and keys are case insensitive and all leading and
 //  trailing whitespace is ignored. Comments are lines that begin with a
 //  semicolon. Trailing comments are allowed on section lines.
 //
 //  Files are read on demand, upon which data is kept in memory and the file
 //  is closed. This utility supports lazy writing, which only writes changes
 //  and updates to a file and preserves custom formatting and comments. A lazy
 //  write invoked by a write() call will read the output file, find what
 //  changes have been made and update the file accordingly. If you only need to
 //  generate files, use generate() instead. Section and key order is preserved
 //  on read, write and insert.
 //
 ///////////////////////////////////////////////////////////////////////////////
 //
 //  /* BASIC USAGE EXAMPLE: */
 //
 //  /* read from file */
 //  mINI::INIFile file("myfile.ini");
 //  mINI::INIStructure ini;
 //  file.read(ini);
 //
 //  /* read value; gets a reference to actual value in the structure.
 //     if key or section don't exist, a new empty value will be created */
 //  generic_string& value = ini["section"]["key"];
 //
 //  /* read value safely; gets a copy of value in the structure.
 //     does not alter the structure */
 //  generic_string value = ini.get("section").get("key");
 //
 //  /* set or update values */
 //  ini["section"]["key"] = "value";
 //
 //  /* set multiple values */
 //  ini["section2"].set({
 //      {"key1", "value1"},
 //      {"key2", "value2"}
 //  });
 //
 //  /* write updates back to file, preserving comments and formatting */
 //  file.write(ini);
 //
 //  /* or generate a file (overwrites the original) */
 //  file.generate(ini);
 //
 ///////////////////////////////////////////////////////////////////////////////
 //
 //  Long live the INI file!!!
 //
 ///////////////////////////////////////////////////////////////////////////////
 //  Patched to support Unicode files if compiled in UNICODE mode
 //  BY: Leonardo Silva (Feb-2022)
 ///////////////////////////////////////////////////////////////////////////////


#pragma once

#include <string>
#include <sstream>
#include <algorithm>
#include <utility>
#include <unordered_map>
#include <vector>
#include <memory>
#include <fstream>
#include <sys/stat.h>
#include <cctype>

#ifndef GENERIC_STRING
#define GENERIC_STRING
typedef std::basic_string<TCHAR> generic_string;
#endif

#ifdef  UNICODE
#ifndef UNICODE_TSTAT
#define UNICODE_TSTAT
#define _tstat _wstat
struct tstat : _stat64i32 {};
#endif
#else
#ifndef UNICODE_TSTAT
#define _tstat stat
struct tstat : stat {};
#endif
#endif

#ifndef GENERIC_FILESTREAM
#define GENERIC_FILESTREAM
typedef std::basic_ofstream<TCHAR> tofstream;
typedef std::basic_ifstream<TCHAR> tifstream;
#endif



namespace mINI
{
	namespace INIStringUtil
	{
		const TCHAR* const whitespaceDelimiters = TEXT(" \t\n\r\f\v");
		inline void trim(generic_string& str)
		{
			str.erase(str.find_last_not_of(whitespaceDelimiters) + 1);
			str.erase(0, str.find_first_not_of(whitespaceDelimiters));
		}
#ifndef MINI_CASE_SENSITIVE
		inline void toLower(generic_string& str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](const TCHAR c) {
				return static_cast<TCHAR>(std::tolower(c));
			});
		}
#endif
		inline void replace(generic_string& str, generic_string const& a, generic_string const& b)
		{
			if (!a.empty())
			{
				std::size_t pos = 0;
				while ((pos = str.find(a, pos)) != generic_string::npos)
				{
					str.replace(pos, a.size(), b);
					pos += b.size();
				}
			}
		}
#ifdef _WIN32
		const TCHAR* const endl = TEXT("\r\n");
#else
		const TCHAR* const endl = TEXT("\n");
#endif
	}

	template<typename T>
	class INIMap
	{
	private:
		using T_DataIndexMap = std::unordered_map<generic_string, std::size_t>;
		using T_DataItem = std::pair<generic_string, T>;
		using T_DataContainer = std::vector<T_DataItem>;
		using T_MultiArgs = typename std::vector<std::pair<generic_string, T>>;

		T_DataIndexMap dataIndexMap;
		T_DataContainer data;

		inline std::size_t setEmpty(generic_string& key)
		{
			std::size_t index = data.size();
			dataIndexMap[key] = index;
			data.emplace_back(key, T());
			return index;
		}

	public:
		using const_iterator = typename T_DataContainer::const_iterator;

		INIMap() { }

		INIMap(INIMap const& other)
		{
			std::size_t data_size = other.data.size();
			for (std::size_t i = 0; i < data_size; ++i)
			{
				auto const& key = other.data[i].first;
				auto const& obj = other.data[i].second;
				data.emplace_back(key, obj);
			}
			dataIndexMap = T_DataIndexMap(other.dataIndexMap);
		}

		T& operator[](generic_string key)
		{
			INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
			INIStringUtil::toLower(key);
#endif
			auto it = dataIndexMap.find(key);
			bool hasIt = (it != dataIndexMap.end());
			std::size_t index = (hasIt) ? it->second : setEmpty(key);
			return data[index].second;
		}
		T get(generic_string key) const
		{
			INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
			INIStringUtil::toLower(key);
#endif
			auto it = dataIndexMap.find(key);
			if (it == dataIndexMap.end())
			{
				return T();
			}
			return T(data[it->second].second);
		}
		bool has(generic_string key) const
		{
			INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
			INIStringUtil::toLower(key);
#endif
			return (dataIndexMap.count(key) == 1);
		}
		void set(generic_string key, T obj)
		{
			INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
			INIStringUtil::toLower(key);
#endif
			auto it = dataIndexMap.find(key);
			if (it != dataIndexMap.end())
			{
				data[it->second].second = obj;
			}
			else
			{
				dataIndexMap[key] = data.size();
				data.emplace_back(key, obj);
			}
		}
		void set(T_MultiArgs const& multiArgs)
		{
			for (auto const& it : multiArgs)
			{
				auto const& key = it.first;
				auto const& obj = it.second;
				set(key, obj);
			}
		}
		bool remove(generic_string key)
		{
			INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
			INIStringUtil::toLower(key);
#endif
			auto it = dataIndexMap.find(key);
			if (it != dataIndexMap.end())
			{
				std::size_t index = it->second;
				data.erase(data.begin() + index);
				dataIndexMap.erase(it);
				for (auto& it2 : dataIndexMap)
				{
					auto& vi = it2.second;
					if (vi > index)
					{
						vi--;
					}
				}
				return true;
			}
			return false;
		}
		void clear()
		{
			data.clear();
			dataIndexMap.clear();
		}
		std::size_t size() const
		{
			return data.size();
		}
		const_iterator begin() const { return data.begin(); }
		const_iterator end() const { return data.end(); }
	};

	using INIStructure = INIMap<INIMap<generic_string>>;

	namespace INIParser
	{
		using T_ParseValues = std::pair<generic_string, generic_string>;

		enum class PDataType : TCHAR
		{
			PDATA_NONE,
			PDATA_COMMENT,
			PDATA_SECTION,
			PDATA_KEYVALUE,
			PDATA_UNKNOWN
		};

		inline PDataType parseLine(generic_string line, T_ParseValues& parseData)
		{
			parseData.first.clear();
			parseData.second.clear();
			INIStringUtil::trim(line);
			if (line.empty())
			{
				return PDataType::PDATA_NONE;
			}
			TCHAR firstCharacter = line[0];
			if (firstCharacter == TEXT(';'))
			{
				return PDataType::PDATA_COMMENT;
			}
			if (firstCharacter == TEXT('['))
			{
				auto commentAt = line.find_first_of(TEXT(';'));
				if (commentAt != generic_string::npos)
				{
					line = line.substr(0, commentAt);
				}
				auto closingBracketAt = line.find_last_of(TEXT(']'));
				if (closingBracketAt != generic_string::npos)
				{
					auto section = line.substr(1, closingBracketAt - 1);
					INIStringUtil::trim(section);
					parseData.first = section;
					return PDataType::PDATA_SECTION;
				}
			}
			auto lineNorm = line;
			INIStringUtil::replace(lineNorm, TEXT("\\="), TEXT("  "));
			auto equalsAt = lineNorm.find_first_of(TEXT('='));
			if (equalsAt != generic_string::npos)
			{
				auto key = line.substr(0, equalsAt);
				INIStringUtil::trim(key);
				INIStringUtil::replace(key, TEXT("\\="), TEXT("="));
				auto value = line.substr(equalsAt + 1);
				INIStringUtil::trim(value);
				parseData.first = key;
				parseData.second = value;
				return PDataType::PDATA_KEYVALUE;
			}
			return PDataType::PDATA_UNKNOWN;
		}
	}

	class INIReader
	{
	public:
		using T_LineData = std::vector<generic_string>;
		using T_LineDataPtr = std::shared_ptr<T_LineData>;

	private:
		tifstream fileReadStream;
		T_LineDataPtr lineData;

		T_LineData readFile()
		{
			generic_string fileContents;
			fileReadStream.seekg(0, std::ios::end);
			fileContents.resize(static_cast<std::size_t>(fileReadStream.tellg()));
			fileReadStream.seekg(0, std::ios::beg);
			std::size_t fileSize = fileContents.size();
			fileReadStream.read(&fileContents[0], fileSize);
			fileReadStream.close();
			T_LineData output;
			if (fileSize == 0)
			{
				return output;
			}
			generic_string buffer;
			buffer.reserve(50);
			for (std::size_t i = 0; i < fileSize; ++i)
			{
				TCHAR& c = fileContents[i];
				if (c == TEXT('\n'))
				{
					output.emplace_back(buffer);
					buffer.clear();
					continue;
				}
				if (c != TEXT('\0') && c != TEXT('\r'))
				{
					buffer += c;
				}
			}
			output.emplace_back(buffer);
			return output;
		}

	public:
		INIReader(generic_string const& filename, bool keepLineData = false)
		{
			fileReadStream.open(filename.c_str(), std::ios::in | std::ios::binary);
			if (keepLineData)
			{
				lineData = std::make_shared<T_LineData>();
			}
		}
		~INIReader() { }

		bool operator>>(INIStructure& data)
		{
			if (!fileReadStream.is_open())
			{
				return false;
			}
			T_LineData fileLines = readFile();
			generic_string section;
			bool inSection = false;
			INIParser::T_ParseValues parseData;
			for (auto const& line : fileLines)
			{
				auto parseResult = INIParser::parseLine(line, parseData);
				if (parseResult == INIParser::PDataType::PDATA_SECTION)
				{
					inSection = true;
					data[section = parseData.first];
				}
				else if (inSection && parseResult == INIParser::PDataType::PDATA_KEYVALUE)
				{
					auto const& key = parseData.first;
					auto const& value = parseData.second;
					data[section][key] = value;
				}
				if (lineData && parseResult != INIParser::PDataType::PDATA_UNKNOWN)
				{
					if (parseResult == INIParser::PDataType::PDATA_KEYVALUE && !inSection)
					{
						continue;
					}
					lineData->emplace_back(line);
				}
			}
			return true;
		}
		T_LineDataPtr getLines()
		{
			return lineData;
		}
	};

	class INIGenerator
	{
	private:
		tofstream fileWriteStream;

	public:
		bool prettyPrint = false;

		INIGenerator(generic_string const& filename)
		{
			fileWriteStream.open(filename.c_str(), std::ios::out | std::ios::binary);
		}
		~INIGenerator() { }

		bool operator<<(INIStructure const& data)
		{
			if (!fileWriteStream.is_open())
			{
				return false;
			}
			if (!data.size())
			{
				return true;
			}
			auto it = data.begin();
			for (;;)
			{
				auto const& section = it->first;
				auto const& collection = it->second;
				fileWriteStream
					<< TEXT("[")
					<< section
					<< TEXT("]");
				if (collection.size())
				{
					fileWriteStream << INIStringUtil::endl;
					auto it2 = collection.begin();
					for (;;)
					{
						auto key = it2->first;
						INIStringUtil::replace(key, TEXT("="), TEXT("\\="));
						auto value = it2->second;
						INIStringUtil::trim(value);
						fileWriteStream
							<< key
							<< ((prettyPrint) ? TEXT(" = ") : TEXT("="))
							<< value;
						if (++it2 == collection.end())
						{
							break;
						}
						fileWriteStream << INIStringUtil::endl;
					}
				}
				if (++it == data.end())
				{
					break;
				}
				fileWriteStream << INIStringUtil::endl;
				if (prettyPrint)
				{
					fileWriteStream << INIStringUtil::endl;
				}
			}
			return true;
		}
	};

	class INIWriter
	{
	private:
		using T_LineData = std::vector<generic_string>;
		using T_LineDataPtr = std::shared_ptr<T_LineData>;

		generic_string filename;

		T_LineData getLazyOutput(T_LineDataPtr const& lineData, INIStructure& data, INIStructure& original)
		{
			T_LineData output;
			INIParser::T_ParseValues parseData;
			generic_string sectionCurrent;
			bool parsingSection = false;
			bool continueToNextSection = false;
			bool discardNextEmpty = false;
			bool writeNewKeys = false;
			std::size_t lastKeyLine = 0;
			for (auto line = lineData->begin(); line != lineData->end(); ++line)
			{
				if (!writeNewKeys)
				{
					auto parseResult = INIParser::parseLine(*line, parseData);
					if (parseResult == INIParser::PDataType::PDATA_SECTION)
					{
						if (parsingSection)
						{
							writeNewKeys = true;
							parsingSection = false;
							--line;
							continue;
						}
						sectionCurrent = parseData.first;
						if (data.has(sectionCurrent))
						{
							parsingSection = true;
							continueToNextSection = false;
							discardNextEmpty = false;
							output.emplace_back(*line);
							lastKeyLine = output.size();
						}
						else
						{
							continueToNextSection = true;
							discardNextEmpty = true;
							continue;
						}
					}
					else if (parseResult == INIParser::PDataType::PDATA_KEYVALUE)
					{
						if (continueToNextSection)
						{
							continue;
						}
						if (data.has(sectionCurrent))
						{
							auto& collection = data[sectionCurrent];
							auto const& key = parseData.first;
							auto const& value = parseData.second;
							if (collection.has(key))
							{
								auto outputValue = collection[key];
								if (value == outputValue)
								{
									output.emplace_back(*line);
								}
								else
								{
									INIStringUtil::trim(outputValue);
									auto lineNorm = *line;
									INIStringUtil::replace(lineNorm, TEXT("\\="), TEXT("  "));
									auto equalsAt = lineNorm.find_first_of('=');
									auto valueAt = lineNorm.find_first_not_of(
										INIStringUtil::whitespaceDelimiters,
										equalsAt + 1
									);
									generic_string outputLine = line->substr(0, valueAt);
									if (prettyPrint && equalsAt + 1 == valueAt)
									{
										outputLine += TEXT(" ");
									}
									outputLine += outputValue;
									output.emplace_back(outputLine);
								}
								lastKeyLine = output.size();
							}
						}
					}
					else
					{
						if (discardNextEmpty && line->empty())
						{
							discardNextEmpty = false;
						}
						else if (parseResult != INIParser::PDataType::PDATA_UNKNOWN)
						{
							output.emplace_back(*line);
						}
					}
				}
				if (writeNewKeys || std::next(line) == lineData->end())
				{
					T_LineData linesToAdd;
					if (data.has(sectionCurrent) && original.has(sectionCurrent))
					{
						auto const& collection = data[sectionCurrent];
						auto const& collectionOriginal = original[sectionCurrent];
						for (auto const& it : collection)
						{
							auto key = it.first;
							if (collectionOriginal.has(key))
							{
								continue;
							}
							auto value = it.second;
							INIStringUtil::replace(key, TEXT("="), TEXT("\\="));
							INIStringUtil::trim(value);
							linesToAdd.emplace_back(
								key + ((prettyPrint) ? TEXT(" = ") : TEXT("=")) + value
							);
						}
					}
					if (!linesToAdd.empty())
					{
						output.insert(
							output.begin() + lastKeyLine,
							linesToAdd.begin(),
							linesToAdd.end()
						);
					}
					if (writeNewKeys)
					{
						writeNewKeys = false;
						--line;
					}
				}
			}
			for (auto const& it : data)
			{
				auto const& section = it.first;
				if (original.has(section))
				{
					continue;
				}
				if (prettyPrint && output.size() > 0 && !output.back().empty())
				{
					output.emplace_back();
				}
				output.emplace_back(TEXT("[") + section + TEXT("]"));
				auto const& collection = it.second;
				for (auto const& it2 : collection)
				{
					auto key = it2.first;
					auto value = it2.second;
					INIStringUtil::replace(key, TEXT("="), TEXT("\\="));
					INIStringUtil::trim(value);
					output.emplace_back(
						key + ((prettyPrint) ? TEXT(" = ") : TEXT("=")) + value
					);
				}
			}
			return output;
		}

	public:
		bool prettyPrint = false;

		INIWriter(generic_string const& filename)
			: filename(filename)
		{
		}
		~INIWriter() { }

		bool operator<<(INIStructure& data)
		{
			struct tstat buf = {};
			bool fileExists = (_tstat(filename.c_str(), &buf) == 0);
			if (!fileExists)
			{
				INIGenerator generator(filename);
				generator.prettyPrint = prettyPrint;
				return generator << data;
			}
			INIStructure originalData;
			T_LineDataPtr lineData;
			bool readSuccess = false;
			{
				INIReader reader(filename, true);
				if ((readSuccess = reader >> originalData))
				{
					lineData = reader.getLines();
				}
			}
			if (!readSuccess)
			{
				return false;
			}
			T_LineData output = getLazyOutput(lineData, data, originalData);
			tofstream fileWriteStream(filename, std::ios::out | std::ios::binary);
			if (fileWriteStream.is_open())
			{
				if (output.size())
				{
					auto line = output.begin();
					for (;;)
					{
						fileWriteStream << *line;
						if (++line == output.end())
						{
							break;
						}
						fileWriteStream << INIStringUtil::endl;
					}
				}
				return true;
			}
			return false;
		}
	};

	class INIFile
	{
	private:
		generic_string filename;

	public:
		INIFile(generic_string const& filename)
			: filename(filename)
		{ }

		~INIFile() { }

		bool read(INIStructure& data) const
		{
			if (data.size())
			{
				data.clear();
			}
			if (filename.empty())
			{
				return false;
			}
			INIReader reader(filename);
			return reader >> data;
		}
		bool generate(INIStructure const& data, bool pretty = false) const
		{
			if (filename.empty())
			{
				return false;
			}
			INIGenerator generator(filename);
			generator.prettyPrint = pretty;
			return generator << data;
		}
		bool write(INIStructure& data, bool pretty = false) const
		{
			if (filename.empty())
			{
				return false;
			}
			INIWriter writer(filename);
			writer.prettyPrint = pretty;
			return writer << data;
		}
	};
}

