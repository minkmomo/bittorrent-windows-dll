#pragma once
#include <string>
#include <vector>
#include <functional>

#ifdef _COMPLING_BITTORRENT_DLL_
#define LIBSPEC __declspec(dllexport)
#else
#define LIBSPEC __declspec(dllimport)
#endif

namespace libtorrent
{
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
		, std::function< void(int,char const *) > error_handler
		, std::function< void(int,int) > print_progress
		, std::vector<std::string> const & web_seeds, std::vector<std::string> const & trackers
		, bool use_merklefile = true, std::string root_cert = ""
		, int pad_file_limit = -1, int piece_size = 0, bool inc_sha1_hash = true
		, bool dont_follow_symlinks = false );

	//////////////////////////////////////////////////////////////////////////
}