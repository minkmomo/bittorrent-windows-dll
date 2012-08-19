#include "bittorrent.h"
#include "session_impl.h"

namespace libtorrent
{
	//////////////////////////////////////////////////////////////////////////
	//

	TorrentSession::TorrentSession( std::vector< std::string > const & sessionSettingParam, ErrorHandler error_handler, EventHandler event_handler, int listenPort )
		: impl_( new TorrentSessionImpl( listenPort, sessionSettingParam, error_handler, event_handler ) )
	{}

	//////////////////////////////////////////////////////////////////////////
	//

	TorrentSession::~TorrentSession() { if( impl_ ) delete impl_; };

	//////////////////////////////////////////////////////////////////////////
	//

	void TorrentSession::Update() { impl_->Update(); }

	//////////////////////////////////////////////////////////////////////////
	//

	bool TorrentSession::AddTorrent(std::string torrent) { return impl_->AddTorrent( torrent ); }

	//////////////////////////////////////////////////////////////////////////
	//


	bool TorrentSession::SetSessionSetting( std::vector< std::string > const & params ) { return impl_->SetSessionSetting( params ); }

	//////////////////////////////////////////////////////////////////////////
}