#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <array>

#ifdef _COMPLING_BITTORRENT_DLL_
#define LIBSPEC __declspec(dllexport)
#else
#define LIBSPEC __declspec(dllimport)
#endif

namespace libtorrent
{
	//////////////////////////////////////////////////////////////////////////
	//

	struct TorrentStatus
	{
		TorrentStatus() {}
		~TorrentStatus() {}

		enum state_t
		{
			queued_for_checking,
			checking_files,
			downloading_metadata,
			downloading,
			finished,
			seeding,
			allocating,
			checking_resume_data
		};

		state_t state;
		bool paused;
		bool auto_managed;
		bool sequential_download;
		bool is_seeding;
		bool is_finished;
		bool has_metadata;

		float progress;
		// progress parts per million (progress * 1000000)
		// when disabling floating point operations, this is
		// the only option to query progress
		int progress_ppm;

		//size_t next_announce;
		//size_t announce_interval;

		std::string current_tracker;

		// transferred this session!
		// total, payload plus protocol
		__int64 total_download;
		__int64 total_upload;

		// payload only
		__int64 total_payload_download;
		__int64 total_payload_upload;

		// the amount of payload bytes that
		// has failed their hash test
		__int64 total_failed_bytes;

		// the number of payload bytes that
		// has been received redundantly.
		__int64 total_redundant_bytes;

		// current transfer rate
		// payload plus protocol
		int download_rate;
		int upload_rate;

		// the rate of payload that is
		// sent and received
		int download_payload_rate;
		int upload_payload_rate;

		// the number of peers this torrent is connected to
		// that are seeding.
		int num_seeds;

		// the number of peers this torrent
		// is connected to (including seeds).
		int num_peers;

		// if the tracker sends scrape info in its
		// announce reply, these fields will be
		// set to the total number of peers that
		// have the whole file and the total number
		// of peers that are still downloading
		int num_complete;
		int num_incomplete;

		// this is the number of seeds whose IP we know
		// but are not necessarily connected to
		int list_seeds;

		// this is the number of peers whose IP we know
		// (including seeds), but are not necessarily
		// connected to
		int list_peers;

		// the number of peers in our peerlist that
		// we potentially could connect to
		int connect_candidates;

		//bitfield pieces;
		//bitfield verified_pieces;

		// this is the number of pieces the client has
		// downloaded. it is equal to:
		// std::accumulate(pieces->begin(), pieces->end());
		int num_pieces;

		// the number of bytes of the file we have
		// including pieces that may have been filtered
		// after we downloaded them
		__int64 total_done;

		// the number of bytes we have of those that we
		// want. i.e. not counting bytes from pieces that
		// are filtered as not wanted.
		__int64 total_wanted_done;

		// the total number of bytes we want to download
		// this may be smaller than the total torrent size
		// in case any pieces are filtered as not wanted
		__int64 total_wanted;

		// the number of distributed copies of the file.
		// note that one copy may be spread out among many peers.
		//
		// the integer part tells how many copies
		//   there are of the rarest piece(s)
		//
		// the fractional part tells the fraction of pieces that
		//   have more copies than the rarest piece(s).

		// the number of full distributed copies (i.e. the number
		// of peers that have the rarest piece)
		int distributed_full_copies;

		// the fraction of pieces that more peers has than the
		// rarest pieces. This indicates how close the swarm is
		// to have one more full distributed copy
		int distributed_fraction;

		float distributed_copies;

		// the block size that is used in this torrent. i.e.
		// the number of bytes each piece request asks for
		// and each bit in the download queue bitfield represents
		int block_size;

		int num_uploads;
		int num_connections;
		int uploads_limit;
		int connections_limit;

		// true if the torrent is saved in compact mode
		// false if it is saved in full allocation mode
		//storage_mode_t storage_mode;

		int up_bandwidth_queue;
		int down_bandwidth_queue;

		// number of bytes downloaded since torrent was started
		// saved and restored from resume data
		__int64 all_time_upload;
		__int64 all_time_download;

		// the number of seconds of being active
		// and as being a seed, saved and restored
		// from resume data
		int active_time;
		int finished_time;
		int seeding_time;

		// higher value means more important to seed
		int seed_rank;

		// number of seconds since last scrape, or -1 if
		// there hasn't been a scrape
		int last_scrape;

		// true if there are incoming connections to this
		// torrent
		bool has_incoming;

		// the number of "holes" in the torrent
		int sparse_regions;

		// is true if this torrent is (still) in seed_mode
		bool seed_mode;

		// this is set to true when the torrent is blocked
		// from downloading, typically caused by a file
		// write operation failing
		bool upload_mode;

		// this is true if the torrent is in share-mode
		bool share_mode;

		// true if the torrent is in super seeding mode
		bool super_seeding;

		// the priority of this torrent
		int priority;

		// the time this torrent was added and completed
		time_t added_time;
		time_t completed_time;
		time_t last_seen_complete;

		// number of seconds since last upload or download activity
		int time_since_upload;
		int time_since_download;

		// the position in the download queue where this torrent is
		// this is -1 for seeds and finished torrents
		int queue_position;

		// true if this torrent has had changes since the last
		// time resume data was saved
		bool need_save_resume;

		// defaults to true. Determines whether the session
		// IP filter applies to this torrent or not
		bool ip_filter_applies;

		// the info-hash for this torrent
		//sha1_hash info_hash;

		// if this torrent has its own listen socket, this is
		// the port it's listening on. Otherwise it's 0
		int listen_port;
	};

	//////////////////////////////////////////////////////////////////////////
	//

	enum eTorrentEvent
	{
		torrent_event_finished,
		torrent_event_add_failed,
		torrent_event_max
	};

	//////////////////////////////////////////////////////////////////////////
	//

	typedef std::function< void(int,char const *) > ErrorHandler;
	typedef std::function< void(int,int) > ProgressHandler;
	typedef std::function< void(eTorrentEvent,std::string const &) > EventHandler;

	//////////////////////////////////////////////////////////////////////////
	//

	class TorrentSessionImplBase;

	//////////////////////////////////////////////////////////////////////////
	//

	class LIBSPEC TorrentSession
	{
	public:
		
		// CLIENT OPTIONS
		// -s <path>				sets the save path for downloads
		// -m <path>				sets the .torrent monitor directory
		// -t <seconds>				sets the scan interval of the monitor dir
		// -k						enable high performance settings. This overwrites any other
		//							previous command line options, so be sure to specify this first
		// -G						Add torrents in seed-mode (i.e. assume all pieces are present and check hashes on-demand)
		// BITTORRENT OPTIONS
		// -c <limit>				sets the max number of connections
		// -T <limit>				sets the max number of connections per torrent
		// -U <rate>				sets per-torrent upload rate
		// -D <rate>				sets per-torrent download rate
		// -d <rate>				limits the download rate
		// -u <rate>				limits the upload rate
		// -S <limit>				limits the upload slots
		// -A <num pieces>			allowed pieces set size
		// -H						Don't start DHT
		// -X						Don't start local peer discovery
		// -n						announce to trackers in all tiers
		// -W <num peers>			Set the max number of peers to keep in the peer list
		// -B <seconds>				sets the peer timeout
		// -Q						enables share mode. Share mode attempts to maximize
		//							share ratio rather than downloading
		// -r <IP:port>				connect to specified peer
#ifndef TORRENT_DISABLE_ENCRYPTION
		// -e						force encrypted bittorrent connections
#endif
		// QUEING OPTIONS
		// -v <limit>				Set the max number of active downloads
		// -^ <limit>				Set the max number of active seeds
		// NETWORK OPTIONS
		// -p <port>				sets the listen port
		// -o <limit>				limits the number of simultaneous half-open TCP connections to the given number.
		// -w <seconds>				sets the retry time for failed web seeds
		// -x <file>				loads an emule IP-filter file
		// -P <host:port>			Use the specified SOCKS5 proxy
		// -L <user:passwd>			Use the specified username and password for the proxy specified by -P
		// -h						allow multiple connections from the same IP
		// -M						Disable TCP/uTP bandwidth balancing
		// -N						Do not attempt to use UPnP and NAT-PMP to forward ports
		// -Y						Rate limit local peers
		// -y						Disable TCP connections (disable outgoing TCP and reject incoming TCP connections)
		// -b <IP>					sets IP of the interface to bind the listen socket to
		// -I <IP>					sets the IP of the interface to bind outgoing peer connections to
#if TORRENT_USE_I2P
		// -i <i2p-host>			the hostname to an I2P SAM bridge to use
#endif
		// -l <limit>				sets the listen socket queue size
		// DISK OPTIONS
		// -a <mode>				sets the allocation mode. [sparse|full]
		// -R <num blocks>			number of blocks per read cache line
		// -C <limit>				sets the max cache size. Specified in 16kB blocks
		// -O						Disallow disk job reordering
		// -j						disable disk read-ahead
		// -z						disable piece hash checks (used for benchmarking)
		// -0						disable disk I/O, read garbage and don't flush to disk
		//
		// TORRENT is a path to a .torrent file
		// MAGNETURL is a magnet link
		// URL is a url to a torrent file
		// Example for running benchmark:
		// -k -z -N -h -H -M -l 2000 -S 1000 -T 1000 -c 1000
		TorrentSession( std::vector< std::string > const & sessionSettingParam, ErrorHandler error_handler, EventHandler event_handler, int listenPort = 6881 );
		~TorrentSession();

		void update();
		bool add(std::string torrent);
		bool del( std::string torrent, bool delete_torrent_file, bool delete_download_file );
		bool pause( std::string torrent );
		bool resume( std::string torrent );
		bool setting( std::vector<std::string> const & params );

		void set_print_debug( bool print );
		std::string get_tag( std::string torrent );
		std::string get_magnet( std::string torrent );
		void get_metadata( std::string torrent, std::vector<char> & data );

		void set_sequential_down( std::string torrent, bool sequential );
		// prograss, state, tracker, peer, speed, etc info.
		bool get_status( std::string torrent, TorrentStatus & ts );

	private:
		TorrentSessionImplBase * impl_;
	};

	//////////////////////////////////////////////////////////////////////////
	//
	// target_file				target file
	// outfile					specifies the output filename of the torrent file
	// web_seeds				adds a web seed to the torrent with the specified url
	// trackers					adds the specified tracker to the torrent
	// use_merklefile			generate a merkle hash tree torrent. merkle torrents require client support. the resulting full merkle tree is written to the specified file
	// root_cert				add root certificate to the torrent, to verify the HTTPS tracker
	// pad_file_limit bytes		enables padding files. Files larger than bytes will be piece-aligned
	// piece_size bytes			specifies a piece size for the torrent. This has to be a multiple of 16 kiB
	// inc_sha1_hash			include sha-1 file hashes in the torrent this helps supporting mixing sources from other networks
	// dont_follow_symlinks		Don't follow symlinks, instead encode them as links in the torrent file
	
	extern "C" LIBSPEC bool make( std::string target_file, std::string outfile
		, ErrorHandler error_handler
		, ProgressHandler print_progress
		, std::vector<std::string> const & web_seeds, std::vector<std::string> & trackers
		, bool use_merklefile = false, std::string root_cert = ""
		, int pad_file_limit = -1, int piece_size = 0, bool inc_sha1_hash = true
		, bool dont_follow_symlinks = false );

	//////////////////////////////////////////////////////////////////////////
	//

	class TorrentSessionImplBase {
	public:
		virtual ~TorrentSessionImplBase() = 0 {};
		virtual void update() = 0;
		virtual bool add(std::string const & torrent) = 0;
		virtual bool del(std::string const & torrent, bool delete_torrent_file, bool delete_download_file) = 0;
		virtual bool pause(std::string const & torrent) = 0;
		virtual bool resume(std::string const & torrent) = 0;
		virtual bool setting( std::vector<std::string> const & params, bool isFirst = false ) = 0;
		virtual void set_print_debug( bool print ) = 0;
		virtual std::string const & get_tag( std::string const & torrent ) = 0;
		virtual std::string get_magnet( std::string const & torrent ) = 0;
		virtual void get_metadata( std::string const & torrent, std::vector<char> & data ) = 0;
		virtual void set_sequential_down( std::string const & torrent, bool sequential ) = 0;
		virtual bool get_status( std::string const & torrent, TorrentStatus & ts ) = 0;
	};

	//////////////////////////////////////////////////////////////////////////
}