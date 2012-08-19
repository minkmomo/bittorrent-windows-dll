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
	/*
	std::vector<std::string> webseeds;
	std::vector<std::string> trackers;

	webseeds.push_back( "http://dl.dropbox.com/s/qod5i7lsqo2r9hb/20120723_145231.mp4" );

	libtorrent::make( "20120723_145231.mp4", "test3.torrent", [](int i,char const*msg){ ::MessageBoxA( 0, msg, 0, 0 ); }, [](int c, int m){}, webseeds, trackers );
	*/

	bool finish = false;

	char const * test_torrent_name = "test3.torrent";

	std::vector< std::string > setting_params;

	auto error_handler = [](int i, char const * msg){ 
		::MessageBoxA(0,msg, 0,0);
	};

	auto event_handler = [&finish,test_torrent_name]( libtorrent::eTorrentEvent e, std::string const & torrent ) {
		if( torrent == test_torrent_name )
		{
			::MessageBoxA( 0, test_torrent_name, "complete", 0 );
			finish = true;
		}
	};

	libtorrent::TorrentSession torrent_session( setting_params, error_handler, event_handler );

	torrent_session.AddTorrent( test_torrent_name );

	while(!finish)
	{
		Sleep(1);

		torrent_session.Update();
	}

	return 0;
}