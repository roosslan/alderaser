
#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define IDC_SETTINGS_BUTTON			150

#include <string>
#include <iostream>
#include <windows.h>
#include <time.h>
#include <format>
#include <chrono>
#include <thread>
#include "winuser.h"
#include <windowsx.h>
#include <SetupAPI.h>
#include <algorithm>

#include <atlbase.h>
#include <atlapp.h>
#include <atldlgs.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlgdi.h>
#include <atlctrls.h>
#include <atlmisc.h>

#include "resource.h"
#include "func.h"
#include "simpleini.h"

CSimpleIniA configFile;						/* Platform X86/Win32 */
std::string recycle_dir;
std::string root_dir;
std::vector<std::string> exclude_Folders;

//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
int main(int argc, char* argv[])
{
	rLogger::InitLogging();

	int hRes = iniChecks();

	LOG_SAVE << "recycle_directory parameter is " << recycle_dir << std::endl;

	if (argc > 1)
	{
		if (strcmp(argv[1], "clrtrash") == 0)
		{
			LOG_SAVE << "Erasing recycle bin's (" << recycle_dir << ") content" << std::endl;
			deleteDirectoryContents(recycle_dir);
		}

		if (argv[1] == "ShowWindow")
		{
/*			eraserWnd sampleWizard;
			sampleWizard.DoModal();
*/
		}
		return 10; // 0xA
	}

	hRes = bootstrap();	/* Вторая часть ~марлезонского~ работы с INI */

	try
	{
		// directory_iterator can be iterated using a range-for loop
		for (auto const& dir_entry : std::filesystem::directory_iterator{ root_dir })
		{
			if ( std::filesystem::is_empty(dir_entry.path()) || dir_entry.path() == recycle_dir || std::find(exclude_Folders.begin(), exclude_Folders.end(), dir_entry.path()) != exclude_Folders.end() )
			{
				// Папку не трогаем, ничего не делаем. Найдена в "Исключаемых", это Корзина, либо она не пуста
			}
			else
			{
				int rootDirLen = root_dir.length();
				std::string path_wo_drvColon = dir_entry.path().generic_string();
				path_wo_drvColon.erase(0, rootDirLen);
				CopyRecursive(dir_entry.path(), recycle_dir + "\\" + path_wo_drvColon);
				LOG_SAVE << "Removing " << dir_entry.path().generic_string() << " directory's content" << std::endl;
				deleteDirectoryContents(dir_entry.path().generic_string());
			}
		}

	}
	catch (std::exception& e)
	{
		LOG_SAVE << "Catch e " << e.what() << std::endl;
		std::cout << e.what();
	}
	return 0;

}

class eraserPage :
	public CPropertyPageImpl<eraserPage>
{
public:

	BEGIN_MSG_MAP(eraserPage)
		CHAIN_MSG_MAP(__super)
	END_MSG_MAP()

	enum { IDD = IDD_PROPPAGE_SMALL };

};

class eraserWnd :
	public CPropertySheetImpl<eraserWnd>
{
public:

	BEGIN_MSG_MAP(eraserWnd)
		CHAIN_MSG_MAP(__super)
	END_MSG_MAP()

	eraserWnd() :
		CPropertySheetImpl<eraserWnd>(IDS_TITLE)
	{
		AddPage(m_page);
	}

private:
	eraserPage m_page;
};

