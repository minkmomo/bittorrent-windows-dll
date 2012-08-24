// dllTest.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include <stdio.h>
#include <tchar.h>

#include "bittorrent.h"

#include <windows.h>
#include <iostream>

#ifdef _DEBUG
#pragma comment (lib, "bittorrent_d.lib")
#else
#pragma comment (lib, "bittorrent.lib")
#endif

int _tmain(int argc, _TCHAR* argv[])
{	
	bool finish = false;

	char const * test_torrent_name = "http://dl.dropbox.com/s/tidb3j9pdwmhvr2/test3.torrent";

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

	torrent_session.set_print_debug( true );

	torrent_session.add( test_torrent_name );

	while(!finish)
	{
		Sleep(1);

		torrent_session.update();
	}	

	return 0;
}