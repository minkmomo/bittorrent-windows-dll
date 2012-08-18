// dllTest.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include <stdio.h>
#include <tchar.h>

#include "bittorrent.h"

#include <windows.h>

#ifdef _DEBUG
#pragma comment (lib, "bittorrent_d.lib")
#else
#pragma comment (lib, "bittorrent.lib")
#endif

int _tmain(int argc, _TCHAR* argv[])
{
	std::vector<std::string> webseeds;
	std::vector<std::string> trackers;

	webseeds.push_back( "http://dl.dropbox.com/s/qod5i7lsqo2r9hb/20120723_145231.mp4" );

	libtorrent::make( "20120723_145231.mp4", "test3.torrent", [](int i,char const*msg){ ::MessageBoxA( 0, msg, 0, 0 ); }, [](int c, int m){}, webseeds, trackers );

	return 0;
}