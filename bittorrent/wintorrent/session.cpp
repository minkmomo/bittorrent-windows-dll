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
#include <unordered_set>

namespace libtorrent
{
	//////////////////////////////////////////////////////////////////////////
	//

	bool compare_torrent(torrent_status const* lhs, torrent_status const* rhs)
	{
		if (lhs->queue_position != -1 && rhs->queue_position != -1)
		{
			// both are downloading, sort by queue pos
			return lhs->queue_position < rhs->queue_position;
		}
		else if (lhs->queue_position == -1 && rhs->queue_position == -1)
		{
			// both are seeding, sort by seed-rank
			if (lhs->seed_rank != rhs->seed_rank)
				return lhs->seed_rank > rhs->seed_rank;

			return lhs->info_hash < rhs->info_hash;
		}

		return (lhs->queue_position == -1) < (rhs->queue_position == -1);
	}

	//////////////////////////////////////////////////////////////////////////
	//

	class TorrentSessionImpl : public TorrentSessionImplBase
	{
		enum {
			torrents_all,
			torrents_downloading,
			torrents_not_paused,
			torrents_seeding,
			torrents_queued,
			torrents_stopped,
			torrents_checking,
			torrents_feeds,

			torrents_max
		};
	public:
		TorrentSessionImpl(int listenPort, std::vector<std::string> sessionSettingParam, ErrorHandler & error_handler) 
			: listen_port_(listenPort)
			, allocation_mode_( libtorrent::storage_mode_sparse )
			, save_path_(".")
			, torrent_upload_limit_(0)
			, torrent_download_limit_(0)
			, bind_to_interface_("")
			, outgoing_interface_("")
			, poll_interval_(5)
			, max_connections_per_torrent_(50)
			, seed_mode_(false)
			, share_mode_(false)
			, disable_storage_(false)
			, start_dht_(true)
			, start_upnp_(true)
			, start_lsd_(true)
			, error_handler_(error_handler)
			, active_torrent_(0)
			, tick_(0)
			, session_(fingerprint("LT", LIBTORRENT_VERSION_MAJOR, LIBTORRENT_VERSION_MINOR, 0, 0)
				, session::add_default_plugins
				, alert::all_categories
				& ~(alert::dht_notification
				+ alert::progress_notification
				+ alert::debug_notification
				+ alert::stats_notification))
		{
			memset(counters_, 0, sizeof(counters_));

			LoadSetting();

			SetSessionSetting( sessionSettingParam, true );
		}

		virtual ~TorrentSessionImpl() { SaveSetting(); }

		void Update();

		bool AddTorrent(std::string const & torrent);

		bool Puse();
		bool Resume();

		bool SetSessionSetting( std::vector<std::string> params, bool isFirst = false );
		void SaveSetting();

	private:
		bool LoadTorrent( std::string const & torrent );
		void LoadSetting();

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
		int allocation_mode_;

		bool start_dht_;
		bool start_upnp_;
		bool start_lsd_;

		int counters_[torrents_max];

		std::string monitor_dir_;
		std::string bind_to_interface_;
		std::string outgoing_interface_;
		std::string save_path_;

		// if non-empty, a peer that will be added to all torrents
		std::string peer_;

		ErrorHandler & error_handler_;

		std::deque<std::string> events_;

		// the string is the filename of the .torrent file, but only if
		// it was added through the directory monitor. It is used to
		// be able to remove torrents that were added via the directory
		// monitor when they're not in the directory anymore.
		typedef std::tr1::unordered_set<torrent_status> AllHandles;
		typedef std::vector<torrent_status const*> FilteredHandles;
		AllHandles all_handles_;
		FilteredHandles filtered_handles_;

		int active_torrent_;

		size_t tick_;
	};

	//////////////////////////////////////////////////////////////////////////
	//

	bool TorrentSessionImpl::AddTorrent( std::string const & torrent )
	{
		error_code ec;

		// match it against the <hash>@<tracker> format
		if (torrent.size() > 45
			&& is_hex(torrent.c_str(), 40)
			&& (strncmp(torrent.c_str() + 40, "@http://", 8) == 0
			|| strncmp(torrent.c_str() + 40, "@udp://", 7) == 0))
		{
			sha1_hash info_hash;
			from_hex(torrent.c_str(), 40, (char*)&info_hash[0]);

			add_torrent_params p;
			if (seed_mode_) p.flags |= add_torrent_params::flag_seed_mode;
			if (disable_storage_) p.storage = disabled_storage_constructor;
			if (share_mode_) p.flags |= add_torrent_params::flag_share_mode;
			p.trackers.push_back(torrent.c_str() + 41);
			p.info_hash = info_hash;
			p.save_path = save_path_;
			p.storage_mode = (storage_mode_t)allocation_mode_;
			p.flags |= add_torrent_params::flag_paused;
			p.flags &= ~add_torrent_params::flag_duplicate_is_error;
			p.flags |= add_torrent_params::flag_auto_managed;
			
			session_.async_add_torrent( p );
		}
		else if (std::strstr(torrent.c_str(), "http://") == torrent.c_str()
			|| std::strstr(torrent.c_str(), "https://") == torrent.c_str()
			|| std::strstr(torrent.c_str(), "magnet:") == torrent.c_str())
		{
			add_torrent_params p;
			if (seed_mode_) p.flags |= add_torrent_params::flag_seed_mode;
			if (disable_storage_) p.storage = disabled_storage_constructor;
			if (share_mode_) p.flags |= add_torrent_params::flag_share_mode;
			p.save_path = save_path_;
			p.storage_mode = (storage_mode_t)allocation_mode_;
			p.url = torrent;

			std::vector<char> buf;
			if (std::strstr(torrent.c_str(), "magnet:") == torrent.c_str())
			{
				add_torrent_params tmp;
				parse_magnet_uri(*i, tmp, ec);

				if (ec)
					return false;

				std::string filename = combine_path(save_path_, combine_path(".resume"
					, to_hex(tmp.info_hash.to_string()) + ".resume"));

				if (load_file(filename.c_str(), buf, ec) == 0)
					p.resume_data = &buf;
			}

			//printf("adding URL: %s\n", torrent.c_str());
			session_.async_add_torrent(p);
		}
		else
		{
			// if it's a torrent file, open it as usual
			return LoadTorrent( torrent );
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	//

	bool TorrentSessionImpl::LoadTorrent( std::string const & torrent )
	{
		boost::intrusive_ptr<torrent_info> t;
		error_code ec;

		t = new torrent_info(torrent.c_str(), ec);

		if (ec)
		{
			error_handler_( 0, (torrent + ": " + ec.message()).c_str() );
			return false;
		}

		//printf("%s\n", t->name().c_str());

		add_torrent_params p;
		if (seed_mode_) p.flags |= add_torrent_params::flag_seed_mode;
		if (disable_storage_) p.storage = disabled_storage_constructor;
		if (share_mode_) p.flags |= add_torrent_params::flag_share_mode;
		lazy_entry resume_data;

		std::string filename = combine_path(save_path_, combine_path(".resume", to_hex(t->info_hash().to_string()) + ".resume"));

		std::vector<char> buf;
		if (load_file(filename.c_str(), buf, ec) == 0)
			p.resume_data = &buf;

		p.ti = t;
		p.save_path = save_path_;
		p.storage_mode = (storage_mode_t)allocation_mode_;
		p.flags |= add_torrent_params::flag_paused;
		p.flags &= ~add_torrent_params::flag_duplicate_is_error;
		p.flags |= add_torrent_params::flag_auto_managed;
		p.userdata = (void*)strdup(torrent.c_str());
		session_.async_add_torrent(p);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	//

	bool TorrentSessionImpl::SetSessionSetting( std::vector<std::string> params, bool isFirst )
	{
		error_code ec;

		// create directory for resume files
		create_directory(combine_path(save_path_, ".resume"), ec);

		if (ec)
		{
			error_handler_( 0, (std::string("failed to create resume file directory: ") + ec.message() ).c_str() );
			return false;
		}

		if (start_lsd_)
			session_.start_lsd();
		else
			session_.stop_lsd();

		if (start_upnp_)
		{
			session_.start_upnp();
			session_.start_natpmp();
		}
		else
		{
			session_.stop_upnp();
			session_.stop_natpmp();
		}

		session_.set_proxy(proxy_settings_);

		if( isFirst )
		{
			session_.listen_on( std::make_pair(listen_port_, listen_port_), ec, bind_to_interface_.c_str() );

			if (ec)
			{
				char msg[1024];

				sprintf_s( msg, sizeof(msg), "failed to listen on %s on ports %d-%d: %s\n"
					, bind_to_interface_.c_str(), listen_port_, listen_port_+1, ec.message().c_str() );

				error_handler_( 0, msg );

				return false;
			}
		}			

#ifndef TORRENT_DISABLE_DHT
		if (start_dht_)
		{
			session_settings_.use_dht_as_fallback = false;

			session_.add_dht_router(std::make_pair(
				std::string("router.bittorrent.com"), 6881));
			session_.add_dht_router(std::make_pair(
				std::string("router.utorrent.com"), 6881));
			session_.add_dht_router(std::make_pair(
				std::string("router.bitcomet.com"), 6881));

			session_.start_dht();
		}
#endif

		session_settings_.user_agent = "client_test/" LIBTORRENT_VERSION;
		session_settings_.choking_algorithm = session_settings::auto_expand_choker;
		session_settings_.disk_cache_algorithm = session_settings::avoid_readback;
		session_settings_.volatile_read_cache = false;

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	//

	void TorrentSessionImpl::LoadSetting()
	{
		std::vector<char> in;
		error_code ec;
		if (load_file(".ses_state", in, ec) == 0)
		{
			lazy_entry e;
			if (lazy_bdecode(&in[0], &in[0] + in.size(), e, ec) == 0)
				session_.load_state(e);
		}

#ifndef TORRENT_DISABLE_GEO_IP
		session_.load_asnum_db("GeoIPASNum.dat");
		session_.load_country_db("GeoIP.dat");
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	//

	void TorrentSessionImpl::Update()
	{
		++tick_;

		session_.post_torrent_updates();

		if (active_torrent_ >= int(filtered_handles_.size())) active_torrent_ = filtered_handles_.size() - 1;
		if (active_torrent_ >= 0)
		{
			// ask for distributed copies for the selected torrent. Since this
			// is a somewhat expensive operation, don't do it by default for
			// all torrents
			torrent_status const& h = *filtered_handles_[active_torrent_];
			h.handle.status(
				torrent_handle::query_distributed_copies
				| torrent_handle::query_pieces
				| torrent_handle::query_verified_pieces);
		}

		std::vector<feed_handle> feeds;
		session_.get_feeds(feeds);

		counters_[torrents_feeds] = feeds.size();

		std::sort(filtered_handles_.begin(), filtered_handles_.end(), &compare_torrent);


	}

	//////////////////////////////////////////////////////////////////////////
}