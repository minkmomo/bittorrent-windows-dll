#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>

#ifdef _COMPLING_BITTORRENT_DLL_
#define LIBSPEC __declspec(dllexport)
#else
#define LIBSPEC __declspec(dllimport)
#endif

namespace libtorrent
{
	//////////////////////////////////////////////////////////////////////////
	//

	typedef std::function< void(int,char const *) > ErrorHandler;
	typedef std::function< void(int,int) > ProgressHandler;

	//////////////////////////////////////////////////////////////////////////
	//

	class TorrentSessionImplBase {
	public:
		virtual ~TorrentSessionImplBase() = 0 {};
		virtual void Update() = 0;
		virtual bool AddTorrent(std::string const & torrent) = 0;
		virtual bool SetSessionSetting( std::vector<std::string> const & params, bool isFirst = false ) = 0;
	};

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
		TorrentSession( std::vector< std::string > const & sessionSettingParam, ErrorHandler error_handler, int listenPort = 6881 );
		~TorrentSession();

		void Update();
		bool AddTorrent(std::string torrent);
		bool SetSessionSetting( std::vector<std::string> const & params );

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
}