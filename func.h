#pragma once

#ifndef FUNC_H
#define FUNC_H

#include <iostream>
#include <windows.h>
#include <Shobjidl.h>		/* Required for IFileOperation Interface			*/
#include <shellapi.h>		/* Required for Flags set in "SetOperationFlags"	*/
#include <filesystem>

#include <boost/current_function.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace src = boost::log::sources;
namespace logging = boost::log;

#define LOG_SAVE BOOST_LOG_SEV(boost::log::trivial::logger::get(), boost::log::trivial::severity_level::trace)	\
	<< "<" << rLogger::PathToFilename(__FILE__) << ":" << __LINE__ << "> " BOOST_CURRENT_FUNCTION << " | " 	\
	<< boost::log::add_value("Line", __LINE__)

#include "resource.h"
#include "simpleini.h"

const std::string alVersion = "2.11.4.12";

class alderaser
{
public:
	alderaser();
	std::string iniFileName = "alderaser.ini";
	std::string iniPath = std::filesystem::current_path().generic_string() + "/" + iniFileName;
	CSimpleIniA configFile;						/* Platform X86/Win32 */
	std::string recycle_dir;
	std::string root_dir;
	std::vector<std::string> exclude_Folders;

	int iniChecks();
	int bootstrap();

	void deleteDirectoryContents(const std::filesystem::path& dir);
	void CopyRecursive(const std::filesystem::path& src, const std::filesystem::path& target) noexcept;
	BOOL deleteFileOrFolder(LPCWSTR fileOrFolderPath);
};

namespace rLogger
{
	void InitLogging();
	void LogFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);
	std::string PathToFilename(std::string path);
	std::string GetLogFolderPath();
};
#endif