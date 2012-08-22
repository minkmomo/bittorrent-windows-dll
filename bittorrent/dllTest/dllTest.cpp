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

	trackers.push_back( "http://192.168.123.133:46936/announce" );

	libtorrent::make( "20120723_145231.mp4", "test3.torrent", [](int i,char const*msg){ ::MessageBoxA( 0, msg, 0, 0 ); }, [](int c, int m){}, webseeds, trackers );
	*/

	
	bool finish = false;

	//char const * test_torrent_name = "test3.torrent";
	char const * test_torrent_name
		= "magnet:?xt=urn:btih:E578B9873C12C393C2B2B07E21668C1498D88CA4&dn=%eb%82%98%eb%8a%94%20%ea%bc%bc%ec%88%98%eb%8b%a4%20-%20%eb%b4%89%ec%a3%bc17%ed%9a%8c.mp3&tr=udp%3a//tracker.openbittorrent.com%3a80/announce";

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

	torrent_session.add( test_torrent_name );

	while(!finish)
	{
		Sleep(1);

		torrent_session.update();
	}
	

	return 0;
}