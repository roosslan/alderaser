#include <string>
#include <algorithm>
#include <filesystem>
#include <string>
#include <iostream>
#include <windows.h>
#include <codecvt>

#include <atlstr.h>
#include <atlbase.h>
#include <atlconv.h>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

#include "func.h"

int iniChecks()
{
    configFile.SetMultiKey(true);
    std::string iniFileName = "alderaser.ini";

    std::string iniPath = std::filesystem::current_path().generic_string() + "/" + iniFileName;
    configFile.SetUnicode();

    SI_Error rc = configFile.LoadFile(iniPath.c_str());
    if (rc < 0)
    {
        LOG_SAVE << "::InitializeINIconfig: Cannot open INI-file " << iniPath << std::endl;
        return 2;
    };

    HINSTANCE hInstance = 0;

    recycle_dir = configFile.GetValue("main", "recycle_directory", "");
    if (recycle_dir == "")
    {
        LOG_SAVE << "recycle_directory parameter is empty in INI-file " << recycle_dir << std::endl;
        return 4;
    }
}

int bootstrap()
{
    root_dir = configFile.GetValue("main", "root_directory", "");
    if (root_dir == "")
    {
        LOG_SAVE << "root_directory parameter is empty in INI-file " << root_dir << std::endl;
        return 6;
    }
    LOG_SAVE << "root_directory parameter is " << root_dir << std::endl;

    CSimpleIniA::TNamesDepend oValues;
    if (configFile.GetAllValues("excludeDirs", "directory", oValues)) {
        CSimpleIniA::TNamesDepend::const_iterator i = oValues.begin();
        for (; i != oValues.end(); ++i)
        {
            // Приводим слэши к единому виду, независимо от того, как указал пользователь
            std::string str_pItem = i->pItem;
            if (str_pItem.back() == '\\')
                str_pItem.pop_back();
            exclude_Folders.push_back(str_pItem);
        }
    }

    if (!std::filesystem::exists(root_dir))
    {
        LOG_SAVE << "The root directory not found. " << std::endl;
        return 7;
    }

    return 0;
}

void deleteDirectoryContents(const std::filesystem::path& dir)
{
	try
	{
        for (const auto& entry : std::filesystem::directory_iterator(dir))
            deleteFileOrFolder(entry.path().c_str());
			/* std::filesystem::remove_all(entry.path());
            /* remove_all calls RemoveDirectoryW which says:
                The path of the directory to be removed.
                This path must specify an empty directory, and the calling process must have delete access to the directory. */

	}
	catch (std::exception& e)
	{
		LOG_SAVE << "Catch e " << e.what() << std::endl;
		std::cout << e.what();
	}
}

void CopyRecursive(const std::filesystem::path& src, const std::filesystem::path& target) noexcept
{
	try
	{
        std::filesystem::path mkALLdirs = target;        
		LOG_SAVE << "Creating directories " << mkALLdirs.remove_filename() << std::endl;
        std::filesystem::create_directories(mkALLdirs.remove_filename());
		LOG_SAVE << "Copying data " << src.generic_string() << " to " << target.generic_string() << std::endl;
		std::filesystem::copy(src, target, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
	}
	catch (std::exception& e)
	{
		LOG_SAVE << "Catch e " << e.what() << std::endl;
		std::cout << e.what();
	}
}

BOOL deleteFileOrFolder(LPCWSTR fileOrFolderPath) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        //Couldn't initialize COM library - clean up and return
        LOG_SAVE << "Couldn't initialize COM library" << std::endl;
        CoUninitialize();
        return FALSE;
    }
    //Initialize the file operation
    IFileOperation* fileOperation;
    hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&fileOperation));
    if (FAILED(hr)) {
        //Couldn't CoCreateInstance - clean up and return
        LOG_SAVE <<  "Couldn't CoCreateInstance" << std::endl;
        CoUninitialize();
        return FALSE;
    }
    hr = fileOperation->SetOperationFlags(FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI);
    if (FAILED(hr)) {
        //Couldn't add flags - clean up and return
        LOG_SAVE << "Couldn't add flags" << std::endl;
        fileOperation->Release();
        CoUninitialize();
        return FALSE;
    }
    IShellItem* fileOrFolderItem = NULL;
    hr = SHCreateItemFromParsingName(fileOrFolderPath, NULL, IID_PPV_ARGS(&fileOrFolderItem));
    if (FAILED(hr)) {
        //Couldn't get file into an item - clean up and return (maybe the file doesn't exist?)
        LOG_SAVE <<  "Couldn't get file into an item" << std::endl;
        fileOrFolderItem->Release();
        fileOperation->Release();
        CoUninitialize();
        return FALSE;
    }
    hr = fileOperation->DeleteItem(fileOrFolderItem, NULL); //The second parameter is if you want to keep track of progress
    fileOrFolderItem->Release();
    if (FAILED(hr)) {
        //Failed to mark file/folder item for deletion - clean up and return
        LOG_SAVE << "Failed to mark file/folder item for deletion" << std::endl;
        fileOperation->Release();
        CoUninitialize();
        return FALSE;
    }
    hr = fileOperation->PerformOperations();
    fileOperation->Release();
    CoUninitialize();
    if (FAILED(hr)) {
        //failed to carry out delete - return
        LOG_SAVE << "failed to carry out delete" << std::endl;
        return FALSE;
    }
    return TRUE;
}

std::string rLogger::PathToFilename(std::string path) {
    return path.substr(path.find_last_of("/\\") + 1);
}

void rLogger::LogFormatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
    // strm << logging::extract< int >("Line", rec) << ":";
    strm << rec[expr::smessage];
}

void rLogger::InitLogging()
{
    boost::log::add_common_attributes();

    auto consoleSink = boost::log::add_console_log(std::clog);
    consoleSink->set_formatter(&LogFormatter);
    logging::core::get()->add_sink(consoleSink);

    auto fsSink = boost::log::add_file_log(
        boost::log::keywords::file_name = GetLogFolderPath() + "\\logs\\alderaser_%d.%m.%Y-%H_%M_%S.log",
        keywords::format = "%TimeStamp% % Message % ",
        boost::log::keywords::rotation_size = 10 * 1024 * 1024,
        boost::log::keywords::min_free_space = 30 * 1024 * 1024,
        boost::log::keywords::open_mode = std::ios_base::app);

    fsSink->locked_backend()->auto_flush(true);

    LOG_SAVE << "BIMALDEV X-change directory cleaner v" << alVersion << "'s rLogger init " << std::endl;
}

std::string rLogger::GetLogFolderPath()
{
    return std::filesystem::current_path().generic_string();
}
