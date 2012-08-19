#pragma once

#include "bittorrent.h"

#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/storage.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/file_pool.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/identify_client.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/ip_filter.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/bitfield.hpp"
#include "libtorrent/peer_info.hpp"
#include "libtorrent/socket_io.hpp" // print_address
#include "libtorrent/time.hpp"

#include "libtorrent/extensions/metadata_transfer.hpp"
#include "libtorrent/extensions/ut_metadata.hpp"
#include "libtorrent/extensions/ut_pex.hpp"
#include "libtorrent/extensions/smart_ban.hpp"

#include <boost/bind.hpp>
#include <boost/unordered_set.hpp>

namespace libtorrent
{
	//////////////////////////////////////////////////////////////////////////
	//

	typedef boost::unordered_set<torrent_status> TorrentHandles;
	typedef std::multimap<std::string, libtorrent::torrent_handle> handles_t;

	//////////////////////////////////////////////////////////////////////////
	//

	class TorrentSessionImpl : public TorrentSessionImplBase
	{
	public:
		TorrentSessionImpl(int listenPort, std::vector<std::string> const & sessionSettingParam, ErrorHandler error_handler);

		virtual ~TorrentSessionImpl() { SaveSetting(); }

		virtual void Update();

		virtual bool AddTorrent(std::string const & torrent);

		virtual bool SetSessionSetting( std::vector<std::string> const & params, bool isFirst = false );

	private:
		bool LoadTorrent( std::string const & torrent );
		void LoadSetting();
		void ScanDir();
		void SaveSetting();

		// returns true if the alert was handled (and should not be printed to the log)
		// returns false if the alert was not handled
		bool handle_alert( libtorrent::session& ses, libtorrent::alert* a
			, handles_t & files, std::set<libtorrent::torrent_handle>& non_files
			, TorrentHandles & all_handles );

		session session_;
		session_settings session_settings_;
		proxy_settings proxy_settings_;

		int listen_port_;
		int allocation_mode_;

		int torrent_upload_limit_;
		int torrent_download_limit_;
		int poll_interval_;
		int max_connections_per_torrent_;
		bool seed_mode_;
		bool share_mode_;
		bool disable_storage_;	

		bool start_dht_;
		bool start_upnp_;
		bool start_lsd_;

		std::string monitor_dir_; // �䷻Ʈ ������ �߰��Ǿ����� Ȯ���ؼ� �ڵ����� �ٿ�ε带 ������ ����
		std::string bind_to_interface_;
		std::string outgoing_interface_;
		std::string save_path_;

		// if non-empty, a peer that will be added to all torrents
		std::string peer_;

		ErrorHandler error_handler_;

		std::deque<std::string> events_;

		// the string is the filename of the .torrent file, but only if
		// it was added through the directory monitor. It is used to
		// be able to remove torrents that were added via the directory
		// monitor when they're not in the directory anymore.		
		TorrentHandles all_handles_;
		//FilteredHandles filtered_handles_;

		// maps filenames to torrent_handles		
		handles_t files_;
		// torrents that were not added via the monitor dir
		std::set<torrent_handle> non_files_;

		//int active_torrent_;

		ptime next_dir_scan_;

		int num_outstanding_resume_data_;
	};

	//////////////////////////////////////////////////////////////////////////
}
