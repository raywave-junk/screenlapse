#define _CRT_SECURE_NO_WARNINGS
#include "resource.h"
#include <Windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <filesystem>
#include <fstream>
#include <rapidjson/document.h>
#include <gdiplus.h>
#include <libzippp/libzippp.h>

using namespace rapidjson;

HINSTANCE g_hInst = NULL;

// @credits: https://gist.github.com/prashanthrajagopal/05f8ad157ece964d8c4d
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
	using namespace Gdiplus;
	UINT  num = 0;
	UINT  size = 0;

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;

	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}
	free(pImageCodecInfo);
	return 0;
}

// @credits: https://gist.github.com/prashanthrajagopal/05f8ad157ece964d8c4d
void gdiscreen(std::wstring fileName, std::wstring format) {
	using namespace Gdiplus;
	IStream* istream;
	HRESULT res = CreateStreamOnHGlobal(NULL, true, &istream);
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	{
		HDC scrdc, memdc;
		HBITMAP membit;
		scrdc = ::GetDC(0);
		int Height = GetSystemMetrics(SM_CYSCREEN);
		int Width = GetSystemMetrics(SM_CXSCREEN);
		memdc = CreateCompatibleDC(scrdc);
		membit = CreateCompatibleBitmap(scrdc, Width, Height);
		HBITMAP hOldBitmap = (HBITMAP)SelectObject(memdc, membit);
		BitBlt(memdc, 0, 0, Width, Height, scrdc, 0, 0, SRCCOPY);

		Gdiplus::Bitmap bitmap(membit, NULL);
		CLSID clsid;
		std::wstring encoderformat = L"image/" + format;
		GetEncoderClsid(encoderformat.c_str(), &clsid);
		std::wstring fileNameWithFormat = fileName + L"." + format;
		bitmap.Save(fileNameWithFormat.c_str(), &clsid, NULL); // To save the jpeg to a file

		DeleteObject(memdc);
		DeleteObject(membit);
		::ReleaseDC(0, scrdc);
	}
	GdiplusShutdown(gdiplusToken);
}

auto padString(const int pad, std::string current, char toInsert) -> std::string
{
	size_t length = current.length();
	if (length < pad)
		current.insert(0, pad - length, toInsert);
	return current;
}

// @note: need recoding, pasted from stackoverflow
std::wstring s2ws(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

auto zipArchiveFolder(std::string folderName, std::string zipName) -> void
{
	using namespace libzippp;

	ZipArchive zf(zipName.c_str());
	zf.open(ZipArchive::Write);
	for (auto& p : std::filesystem::directory_iterator(folderName))
	{
		zf.addEntry(p.path().filename().string().c_str());
		zf.addFile(p.path().filename().string().c_str(), p.path().string().c_str());
	}
	std::vector<ZipEntry> entries = zf.getEntries();
	std::vector<ZipEntry>::iterator it;
	for (it = entries.begin(); it != entries.end(); ++it) {
		ZipEntry entry = *it;
		entry.setCompressionEnabled(true);
	}

	zf.close();
}

auto WinMain(HINSTANCE, HINSTANCE, char*, int) -> int
{
	HANDLE mut = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "Screenlapse");
	if (mut == NULL)
	{
		mut = CreateMutex(NULL, false, "Screenlapse");
	}
	else
	{
		MessageBoxA(NULL, "Another instance of Screenlapse is running.", "Screenlapse", MB_OK);
		return 1;
	}

	std::string baseDir = "C:\\screenlapse";
	std::string format = "png";
	int timeToSleep = 120000;
	bool archive = true;
	if (std::filesystem::exists("config.json"))
	{
		std::ifstream jsonFile("config.json");
		if (jsonFile.is_open())
		{
			std::string line;
			std::string curline;
			while (getline(jsonFile, curline))
			{
				line += curline;
			}
			Document document;

			if (!document.Parse(line.c_str()).HasParseError() && document.IsObject())
			{
				if (document.HasMember("directory"))
				{
					if (document["directory"].IsString())
					{
						baseDir = document["directory"].GetString();
					}
					else
					{
						MessageBoxA(NULL, "Failed to parse config file. (directory isn't a string)", "Screenlapse", MB_OK);
						return 1;
					}
				}
				else
				{
					MessageBoxA(NULL, "Failed to parse config file. (directory is missing)", "Screenlapse", MB_OK);
					return 1;
				}
				if (document.HasMember("timeout"))
				{
					if (document["timeout"].IsNumber())
					{
						timeToSleep = document["timeout"].GetInt();
					}
					else
					{
						MessageBoxA(NULL, "Failed to parse config file. (timeout isn't a number)", "Screenlapse", MB_OK);
						return 1;
					}
				}
				else
				{
					MessageBoxA(NULL, "Failed to parse config file. (timeout is missing)", "Screenlapse", MB_OK);
					return 1;
				}
				if (document.HasMember("format"))
				{
					if (document["format"].IsString())
					{
						format = document["format"].GetString();
						if (format == "jpg") format = "jpeg";
					}
					else
					{
						MessageBoxA(NULL, "Failed to parse config file. (format isn't a string)", "Screenlapse", MB_OK);
						return 1;
					}
				}
				else
				{
					MessageBoxA(NULL, "Failed to parse config file. (format is missing)", "Screenlapse", MB_OK);
					return 1;
				}
				if (document.HasMember("archive"))
				{
					if (document["archive"].IsBool())
					{
						archive = document["archive"].GetBool();
					}
					else
					{
						MessageBoxA(NULL, "Failed to parse config file. (archive isn't a bool)", "Screenlapse", MB_OK);
						return 1;
					}
				}
				else
				{
					MessageBoxA(NULL, "Failed to parse config file. (archive is missing)", "Screenlapse", MB_OK);
					return 1;
				}
			}
			else
			{
				MessageBoxA(NULL, "Failed to parse config file. (failed to parse)", "Screenlapse", MB_OK);
				return 1;
			}
		}
		else
		{
			MessageBoxA(NULL, "Failed to open config file. (failed to open)", "Screenlapse", MB_OK);
			return 1;
		}
	}
	else
	{
		const char json[] = "{ \"directory\": \"C:\\\\screenlapse\", \"timeout\": 120000, \"format\": \"png\", \"archive\": true }";
		std::ofstream jsonFile;
		jsonFile.open("config.json");
		jsonFile << json;
		jsonFile.close();
		MessageBoxA(NULL, "Generated config file, please, customize it.", "Screenlapse", MB_OK);
		return 1;
	}

	if (!std::filesystem::exists(baseDir))
	{
		auto isSuccess = std::filesystem::create_directory(baseDir);
		if (!isSuccess)
		{
			MessageBoxA(NULL, "Failed to create screenlapse directory, maybe you dont have enough rights.", "Screenlapse", MB_OK);
			return 1;
		}
	}

	std::string previousDate = "";

	{
		time_t t = time(0);
		std::tm* now = localtime(&t);

		previousDate = std::to_string(now->tm_year + 1900) + "." + padString(2, std::to_string(now->tm_mon + 1), '0') + "." + padString(2, std::to_string(now->tm_mday), '0');
	}

	if (archive)
	{
		for (auto& p : std::filesystem::directory_iterator(baseDir))
		{
			if (p.is_directory() && p.path().filename().string() != previousDate)
			{
				std::string fileName = p.path().filename().string().c_str();
				std::string zipName = baseDir + "\\" + fileName + ".zip";

				if (std::filesystem::exists(zipName))
				{
					MessageBoxA(NULL, "ZIP archive with previous date already exists.", "Screenlapse", MB_OK);
					return 1;
				}

				zipArchiveFolder(baseDir + "\\" + fileName, zipName);
				std::filesystem::remove_all(baseDir + "\\" + fileName);
			}
		}
	}

	while (true)
	{
		time_t t = time(0);
		std::tm* now = localtime(&t);

		std::string currentDate = std::to_string(now->tm_year + 1900) + "." + padString(2, std::to_string(now->tm_mon + 1), '0') + "." + padString(2, std::to_string(now->tm_mday), '0');

		std::string dateDir = baseDir + "\\" + currentDate;

		if (archive && !previousDate.empty() && currentDate != previousDate)
		{
			std::string zipName = baseDir + "\\" + previousDate + ".zip";
			if (std::filesystem::exists(zipName))
			{
				MessageBoxA(NULL, "ZIP archive with previous date already exists.", "Screenlapse", MB_OK);
				return 1;
			}

			{
				if (std::filesystem::exists(baseDir + "\\" + previousDate))
				{
					zipArchiveFolder(baseDir + "\\" + previousDate, zipName);

					std::filesystem::remove_all(baseDir + "\\" + previousDate);
				}
			}
		}

		previousDate = currentDate;

		if (!std::filesystem::exists(dateDir))
		{
			auto isSuccess = std::filesystem::create_directory(dateDir);
			if (!isSuccess)
			{
				MessageBoxA(NULL, "Failed to create screenlapse current date directory, maybe you dont have enough rights.", "Screenlapse", MB_OK);
				return 1;
			}
		}

		HWND hwnd = GetDesktopWindow();
		gdiscreen(s2ws(dateDir + "\\" + padString(2, std::to_string(now->tm_hour), '0') + "_" + padString(2, std::to_string(now->tm_min), '0') + "_" + padString(2, std::to_string(now->tm_sec), '0')), s2ws(format));

		Sleep(timeToSleep);
	}

	return 0;
}