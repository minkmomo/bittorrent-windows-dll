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

	int save_file(std::string const& filename, std::vector<char>& v)
	{
		using namespace libtorrent;

		file f;
		error_code ec;
		if (!f.open(filename, file::write_only, ec)) return -1;
		if (ec) return -1;
		file::iovec_t b = {&v[0], v.size()};
		size_type written = f.writev(0, &b, 1, ec);
		if (written != int(v.size())) return -3;
		if (ec) return -3;
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	//

	void print_alert(libtorrent::alert const* a, std::string& str)
	{
		using namespace libtorrent;

#ifdef ANSI_TERMINAL_COLORS
		if (a->category() & alert::error_notification)
		{
			str += esc("31");
		}
		else if (a->category() & (alert::peer_notification | alert::storage_notification))
		{
			str += esc("33");
		}
#endif
		str += "[";
		str += time_now_string();
		str += "] ";
		str += a->message();
#ifdef ANSI_TERMINAL_COLORS
		str += esc("0");
#endif

		//if (g_log_file)
		//	fprintf(g_log_file, "[%s] %s\n", time_now_string(),  a->message().c_str());
	}

	//////////////////////////////////////////////////////////////////////////
	//

	bool yes(libtorrent::torrent_status const&)
	{ return true; }

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
				parse_magnet_uri(torrent, tmp, ec);

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

	bool TorrentSessionImpl::SetSessionSetting( std::vector<std::string> const & params, bool isFirst )
	{
		error_code ec;

		for (size_t i = 0; i < params.size(); ++i)
		{			
			if( params[i].empty() || params[i][0] != '-' )
				continue;

			// if there's a flag but no argument following, ignore it
			if (params.size() == (i+1))
				continue;

			char const* arg = params[i+1].c_str();

			switch (params[i][1])
			{
			case 'o': session_settings_.half_open_limit = atoi(arg); break;
			case 'h': session_settings_.allow_multiple_connections_per_ip = true; --i; break;
			case 'p': listen_port_ = atoi(arg); break;
			case 'k': session_settings_ = high_performance_seed(); --i; break;
			case 'j': session_settings_.use_disk_read_ahead = false; --i; break;
			case 'z': session_settings_.disable_hash_checks = true; --i; break;
			case 'B': session_settings_.peer_timeout = atoi(arg); break;
			case 'n': session_settings_.announce_to_all_tiers = true; --i; break;
			case 'G': seed_mode_ = true; --i; break;
			case 'd': session_settings_.download_rate_limit = atoi(arg) * 1000; break;
			case 'u': session_settings_.upload_rate_limit = atoi(arg) * 1000; break;
			case 'S': session_settings_.unchoke_slots_limit = atoi(arg); break;
			case 'a':
				if (strcmp(arg, "allocate") == 0) allocation_mode_ = storage_mode_allocate;
				if (strcmp(arg, "sparse") == 0) allocation_mode_ = storage_mode_sparse;
				break;
			case 's': save_path_ = arg; break;
			case 'U': torrent_upload_limit_ = atoi(arg) * 1000; break;
			case 'D': torrent_download_limit_ = atoi(arg) * 1000; break;
			case 'm': monitor_dir_ = arg; break;
			case 'Q': share_mode_ = true; --i; break;
			case 'b': bind_to_interface_ = arg; break;
			case 'w': session_settings_.urlseed_wait_retry = atoi(arg); break;
			case 't': poll_interval_ = atoi(arg); break;
			case 'H': start_dht_ = false; --i; break;
			case 'l': session_settings_.listen_queue_size = atoi(arg); break;
#ifndef TORRENT_DISABLE_ENCRYPTION
			case 'e':
				{
					pe_settings s;

					s.out_enc_policy = libtorrent::pe_settings::forced;
					s.in_enc_policy = libtorrent::pe_settings::forced;
					s.allowed_enc_level = pe_settings::rc4;
					s.prefer_rc4 = true;
					session_.set_pe_settings(s);
					break;
				}
#endif
			case 'W':
				session_settings_.max_peerlist_size = atoi(arg);
				session_settings_.max_paused_peerlist_size = atoi(arg) / 2;
				break;
			case 'x':
				{
					FILE* filter = fopen(arg, "r");
					if (filter)
					{
						ip_filter fil;
						unsigned int a,b,c,d,e,f,g,h, flags;
						while (fscanf(filter, "%u.%u.%u.%u - %u.%u.%u.%u %u\n", &a, &b, &c, &d, &e, &f, &g, &h, &flags) == 9)
						{
							address_v4 start((a << 24) + (b << 16) + (c << 8) + d);
							address_v4 last((e << 24) + (f << 16) + (g << 8) + h);
							if (flags <= 127) flags = ip_filter::blocked;
							else flags = 0;
							fil.add_rule(start, last, flags);
						}
						session_.set_ip_filter(fil);
						fclose(filter);
					}
				}
				break;
			case 'c': session_settings_.connections_limit = atoi(arg); break;
			case 'T': max_connections_per_torrent_ = atoi(arg); break;
#if TORRENT_USE_I2P
			case 'i':
				{
					proxy_settings ps;
					ps.hostname = arg;
					ps.port = 7656; // default SAM port
					ps.type = proxy_settings::i2p_proxy;
					session_.set_i2p_proxy(ps);
					break;
				}
#endif // TORRENT_USE_I2P
			case 'C':
				session_settings_.cache_size = atoi(arg);
				session_settings_.use_read_cache = session_settings_.cache_size > 0;
				session_settings_.cache_buffer_chunk_size = session_settings_.cache_size / 100;
				break;
			case 'A': session_settings_.allowed_fast_set_size = atoi(arg); break;
			case 'R': session_settings_.read_cache_line_size = atoi(arg); break;
			case 'O': session_settings_.allow_reordered_disk_operations = false; --i; break;
			case 'M': session_settings_.mixed_mode_algorithm = session_settings::prefer_tcp; --i; break;
			case 'y': session_settings_.enable_outgoing_tcp = false; session_settings_.enable_incoming_tcp = false; --i; break;
			case 'r': peer_ = arg; break;
			case 'P':
				{
					char* port = (char*) strrchr(arg, ':');
					if (port == 0)
					{
						fprintf(stderr, "invalid proxy hostname, no port found\n");
						break;
					}
					*port++ = 0;
					proxy_settings_.hostname = arg;
					proxy_settings_.port = atoi(port);
					if (proxy_settings_.port == 0) {
						fprintf(stderr, "invalid proxy port\n");
						break;
					}
					if (proxy_settings_.type == proxy_settings::none)
						proxy_settings_.type = proxy_settings::socks5;
				}
				break;
			case 'L':
				{
					char* pw = (char*) strchr(arg, ':');
					if (pw == 0)
					{
						fprintf(stderr, "invalid proxy username and password specified\n");
						break;
					}
					*pw++ = 0;
					proxy_settings_.username = arg;
					proxy_settings_.password = pw;
					proxy_settings_.type = proxy_settings::socks5_pw;
				}
				break;
			case 'I': outgoing_interface_ = arg; break;
			case 'N': start_upnp_ = false; --i; break;
			case 'X': start_lsd_ = false; --i; break;
			case 'Y': session_settings_.ignore_limits_on_local_network = false; --i; break;
			case 'v': session_settings_.active_downloads = atoi(arg);
				session_settings_.active_limit = (std::max)(atoi(arg) * 2, session_settings_.active_limit);
				break;
			case '^':
				session_settings_.active_seeds = atoi(arg);
				session_settings_.active_limit = (std::max)(atoi(arg) * 2, session_settings_.active_limit);
				break;
			case '0': disable_storage_ = true; --i;
			}
			++i; // skip the argument
		}

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

		session_settings_.user_agent = "libtorrent/" LIBTORRENT_VERSION;
		session_settings_.choking_algorithm = session_settings::auto_expand_choker;
		session_settings_.disk_cache_algorithm = session_settings::avoid_readback;
		session_settings_.volatile_read_cache = false;

		session_.set_settings( session_settings_ );

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
		session_.post_torrent_updates();

		// loop through the alert queue to see if anything has happened.
		std::deque<alert*> alerts;
		session_.pop_alerts(&alerts);
		std::string now = time_now_string();
		for (std::deque<alert*>::iterator i = alerts.begin(), end(alerts.end()); i != end; ++i)
		{
			bool need_resort = false;
			TORRENT_TRY
			{
				if (!handle_alert(session_, *i, files_, non_files_, all_handles_))
				{
					// if we didn't handle the alert, print it to the log
					std::string event_string;
					print_alert(*i, event_string);
					events_.push_back(event_string);
					if (events_.size() >= 20) events_.pop_front();
				}
			} TORRENT_CATCH(std::exception& e) {}

			delete *i;
		}
		alerts.clear();


		if (!monitor_dir_.empty()
			&& next_dir_scan_ < time_now())
		{
			ScanDir();

			next_dir_scan_ = time_now() + seconds(poll_interval_);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//

	void TorrentSessionImpl::ScanDir()
	{
		std::set<std::string> valid;

		using namespace libtorrent;

		if( monitor_dir_.empty() )
			return;

		error_code ec;

		for (directory i(monitor_dir_, ec); !i.done(); i.next(ec))
		{
			std::string file = combine_path(monitor_dir_, i.file());
			if (extension(file) != ".torrent") continue;

			handles_t::iterator k = files_.find(file);
			if (k != files_.end())
			{
				valid.insert(file);
				continue;
			}

			// the file has been added to the dir, start
			// downloading it.
			AddTorrent(file);
			valid.insert(file);
		}

		// remove the torrents that are no longer in the directory

		for (handles_t::iterator i = files_.begin(); !files_.empty() && i != files_.end();)
		{
			if (i->first.empty() || valid.find(i->first) != valid.end())
			{
				++i;
				continue;
			}

			torrent_handle& h = i->second;
			if (!h.is_valid())
			{
				files_.erase(i++);
				continue;
			}

			h.auto_managed(false);
			h.pause();
			// the alert handler for save_resume_data_alert
			// will save it to disk
			if (h.need_save_resume_data())
			{
				h.save_resume_data();
				++num_outstanding_resume_data_;
			}

			files_.erase(i++);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//

	TorrentSessionImpl::TorrentSessionImpl( int listenPort, std::vector<std::string> const & sessionSettingParam, ErrorHandler error_handler ) 
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
		//, active_torrent_(0)
		, next_dir_scan_(time_now())
		, num_outstanding_resume_data_(0)
		, session_(fingerprint("LT", LIBTORRENT_VERSION_MAJOR, LIBTORRENT_VERSION_MINOR, 0, 0)
			, session::add_default_plugins
			, alert::all_categories
			& ~(alert::dht_notification
			+ alert::progress_notification
			+ alert::debug_notification
			+ alert::stats_notification))
	{
		LoadSetting();

		SetSessionSetting( sessionSettingParam, true );
	}

	//////////////////////////////////////////////////////////////////////////
	//

	void TorrentSessionImpl::SaveSetting()
	{
		// keep track of the number of resume data
		// alerts to wait for
		int num_paused = 0;
		int num_failed = 0;

		session_.pause();

		// saving resume data
		std::vector<torrent_status> temp;
		session_.get_torrent_status(&temp, &yes, 0);

		for (std::vector<torrent_status>::iterator i = temp.begin(); i != temp.end(); ++i)
		{
			torrent_status& st = *i;
			if (!st.handle.is_valid())
			{
				//printf("  skipping, invalid handle\n");
				continue;
			}
			if (!st.has_metadata)
			{
				//printf("  skipping %s, no metadata\n", st.handle.name().c_str());
				continue;
			}
			if (!st.need_save_resume)
			{
				//printf("  skipping %s, resume file up-to-date\n", st.handle.name().c_str());
				continue;
			}

			// save_resume_data will generate an alert when it's done
			st.handle.save_resume_data();
			++num_outstanding_resume_data_;
			//printf("\r%d  ", num_outstanding_resume_data);
		}
		//printf("\nwaiting for resume data [%d]\n", num_outstanding_resume_data);


		while (num_outstanding_resume_data_ > 0)
		{
			alert const* a = session_.wait_for_alert(seconds(10));
			if (a == 0) continue;

			std::deque<alert*> alerts;
			session_.pop_alerts(&alerts);
			std::string now = time_now_string();
			for (std::deque<alert*>::iterator i = alerts.begin()
				, end(alerts.end()); i != end; ++i)
			{
				// make sure to delete each alert
				std::auto_ptr<alert> a(*i);

				torrent_paused_alert const* tp = alert_cast<torrent_paused_alert>(*i);
				if (tp)
				{
					++num_paused;
					//printf("\rleft: %d failed: %d pause: %d "
					//, num_outstanding_resume_data_, num_failed, num_paused);
					continue;
				}

				if (alert_cast<save_resume_data_failed_alert>(*i))
				{
					++num_failed;
					--num_outstanding_resume_data_;
					//printf("\rleft: %d failed: %d pause: %d "
					//, num_outstanding_resume_data_, num_failed, num_paused);
					continue;
				}

				save_resume_data_alert const* rd = alert_cast<save_resume_data_alert>(*i);
				if (!rd) continue;
				--num_outstanding_resume_data_;
				//printf("\rleft: %d failed: %d pause: %d "
				//, num_outstanding_resume_data_, num_failed, num_paused);

				if (!rd->resume_data) continue;

				torrent_handle h = rd->handle;
				std::vector<char> out;
				bencode(std::back_inserter(out), *rd->resume_data);
				save_file(combine_path(h.save_path(), combine_path(".resume", to_hex(h.info_hash().to_string()) + ".resume")), out);
			}
		}

		entry session_state;
		session_.save_state(session_state);

		std::vector<char> out;
		bencode(std::back_inserter(out), session_state);
		save_file(".ses_state", out);
	}

	//////////////////////////////////////////////////////////////////////////
	//

	bool TorrentSessionImpl::handle_alert( libtorrent::session& ses, libtorrent::alert* a
		, handles_t & files, std::set<libtorrent::torrent_handle>& non_files
		, TorrentHandles & all_handles )
	{
		using namespace libtorrent;

#ifdef TORRENT_USE_OPENSSL
		if (torrent_need_cert_alert* p = alert_cast<torrent_need_cert_alert>(a))
		{
			torrent_handle h = p->handle;
			error_code ec;
			file_status st;
			std::string cert = combine_path("certificates", to_hex(h.info_hash().to_string())) + ".pem";
			std::string priv = combine_path("certificates", to_hex(h.info_hash().to_string())) + "_key.pem";
			stat_file(cert, &st, ec);
			if (ec)
			{
				//char msg[256];
				//snprintf(msg, sizeof(msg), "ERROR. could not load certificate %s: %s\n", cert.c_str(), ec.message().c_str());
				//if (g_log_file) fprintf(g_log_file, "[%s] %s\n", time_now_string(), msg);
				return true;
			}
			stat_file(priv, &st, ec);
			if (ec)
			{
				//char msg[256];
				//snprintf(msg, sizeof(msg), "ERROR. could not load private key %s: %s\n", priv.c_str(), ec.message().c_str());
				//if (g_log_file) fprintf(g_log_file, "[%s] %s\n", time_now_string(), msg);
				return true;
			}

			//char msg[256];
			//snprintf(msg, sizeof(msg), "loaded certificate %s and key %s\n", cert.c_str(), priv.c_str());
			//if (g_log_file) fprintf(g_log_file, "[%s] %s\n", time_now_string(), msg);

			h.set_ssl_certificate(cert, priv, "certificates/dhparams.pem", "1234");
			h.resume();
		}
#endif

		if (metadata_received_alert* p = alert_cast<metadata_received_alert>(a))
		{
			// if we have a monitor dir, save the .torrent file we just received in it
			// also, add it to the files map, and remove it from the non_files list
			// to keep the scan dir logic in sync so it's not removed, or added twice
			torrent_handle h = p->handle;
			if (h.is_valid()) {
				torrent_info const& ti = h.get_torrent_info();
				create_torrent ct(ti);
				entry te = ct.generate();
				std::vector<char> buffer;
				bencode(std::back_inserter(buffer), te);
				std::string filename = ti.name() + "." + to_hex(ti.info_hash().to_string()) + ".torrent";
				filename = combine_path(monitor_dir_, filename);
				save_file(filename, buffer);

				files.insert(std::pair<std::string, libtorrent::torrent_handle>(filename, h));
				non_files.erase(h);
			}
		}
		else if (add_torrent_alert* p = alert_cast<add_torrent_alert>(a))
		{
			std::string filename;
			if (p->params.userdata)
			{
				filename = (char*)p->params.userdata;
				free(p->params.userdata);
			}

			if (p->error)
			{
				//fprintf(stderr, "failed to add torrent: %s %s\n", filename.c_str(), p->error.message().c_str());
				//char msg[1024];
				//sprintf_s( msg, sizeof(msg), "failed to add torrent: %s %s\n", filename.c_str(), p->error.message().c_str());
				//error_handler_( 0, msg );
			}
			else
			{
				torrent_handle h = p->handle;

				if (!filename.empty())
					files.insert(std::pair<const std::string, torrent_handle>(filename, h));
				else
					non_files.insert(h);

				h.set_max_connections(max_connections_per_torrent_);
				h.set_max_uploads(-1);
				h.set_upload_limit(torrent_upload_limit_);
				h.set_download_limit(torrent_download_limit_);
				h.use_interface(outgoing_interface_.c_str());
#ifndef TORRENT_DISABLE_RESOLVE_COUNTRIES
				h.resolve_countries(true);
#endif

				// if we have a peer specified, connect to it
				if (!peer_.empty())
				{
					char* port = (char*) strrchr((char*)peer_.c_str(), ':');
					if (port > 0)
					{
						*port++ = 0;
						char const* ip = peer_.c_str();
						int peer_port = atoi(port);
						error_code ec;
						if (peer_port > 0)
							h.connect_peer(tcp::endpoint(address::from_string(ip, ec), peer_port));
					}
				}
			}
		}
		else if (torrent_finished_alert* p = alert_cast<torrent_finished_alert>(a))
		{
			p->handle.set_max_connections(max_connections_per_torrent_ / 2);

			// write resume data for the finished torrent
			// the alert handler for save_resume_data_alert
			// will save it to disk
			torrent_handle h = p->handle;
			h.save_resume_data();
			++num_outstanding_resume_data_;
		}
		else if (save_resume_data_alert* p = alert_cast<save_resume_data_alert>(a))
		{
			--num_outstanding_resume_data_;
			torrent_handle h = p->handle;
			TORRENT_ASSERT(p->resume_data);
			if (p->resume_data)
			{
				std::vector<char> out;
				bencode(std::back_inserter(out), *p->resume_data);
				save_file(combine_path(h.save_path(), combine_path(".resume", to_hex(h.info_hash().to_string()) + ".resume")), out);
				if (h.is_valid()
					&& non_files.find(h) == non_files.end()
					&& std::find_if(files.begin(), files.end()
					, boost::bind(&handles_t::value_type::second, _1) == h) == files.end())
					ses.remove_torrent(h);
			}
		}
		else if (save_resume_data_failed_alert* p = alert_cast<save_resume_data_failed_alert>(a))
		{
			--num_outstanding_resume_data_;
			torrent_handle h = p->handle;
			if (h.is_valid()
				&& non_files.find(h) == non_files.end()
				&& std::find_if(files.begin(), files.end()
				, boost::bind(&handles_t::value_type::second, _1) == h) == files.end())
				ses.remove_torrent(h);
		}
		else if (torrent_paused_alert* p = alert_cast<torrent_paused_alert>(a))
		{
			// write resume data for the finished torrent
			// the alert handler for save_resume_data_alert
			// will save it to disk
			torrent_handle h = p->handle;
			h.save_resume_data();
			++num_outstanding_resume_data_;
		}
		else if (state_update_alert* p = alert_cast<state_update_alert>(a))
		{
			bool need_filter_update = false;
			for (std::vector<torrent_status>::iterator i = p->status.begin();
				i != p->status.end(); ++i)
			{
				TorrentHandles::iterator j = all_handles.find(*i);
				// don't add new entries here, that's done in the handler
				// for add_torrent_alert
				if (j == all_handles.end()) continue;
				if (j->state != i->state
					|| j->paused != i->paused
					|| j->auto_managed != i->auto_managed)
					need_filter_update = true;
				((torrent_status&)*j) = *i;
			}

			return true;
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	//

	TorrentSession::TorrentSession( std::vector< std::string > const & sessionSettingParam, ErrorHandler error_handler, int listenPort )
		: impl_( new TorrentSessionImpl( listenPort, sessionSettingParam, error_handler ) )
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