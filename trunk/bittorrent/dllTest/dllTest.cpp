// dllTest.cpp : �ܼ� ���� ���α׷��� ���� �������� �����մϴ�.
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
	/*
	std::vector<std::string> webseeds;
	std::vector<std::string> trackers;

	webseeds.push_back( "http://dl.dropbox.com/s/qod5i7lsqo2r9hb/20120723_145231.mp4" );

	libtorrent::make( "20120723_145231.mp4", "test3.torrent", [](int i,char const*msg){ ::MessageBoxA( 0, msg, 0, 0 ); }, [](int c, int m){}, webseeds, trackers );
	*/

	std::vector< std::string > setting_params;

	libtorrent::TorrentSession torrent_session( setting_params, [](int i, char const * msg){ ::MessageBoxA(0,msg, 0,0); } );

	torrent_session.AddTorrent( "test3.torrent" );

	while(true)
	{
		Sleep(1);

		//torrent_session.Update();
	}

	return 0;
}