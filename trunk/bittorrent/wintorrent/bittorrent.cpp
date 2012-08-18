#include "bittorrent.h"

#ifdef _DEBUG
#pragma comment (lib, "bittorrent_d.lib")
#else
#pragma comment (lib, "bittorrent.lib")
#endif

#pragma comment (lib, "WS2_32.lib")
#pragma comment (lib, "4758cca.lib")
#pragma comment (lib, "aep.lib")
#pragma comment (lib, "atalla.lib")
#pragma comment (lib, "capi.lib")
#pragma comment (lib, "chil.lib")
#pragma comment (lib, "cswift.lib")
#pragma comment (lib, "gost.lib")
#pragma comment (lib, "libeay32.lib")
#pragma comment (lib, "nuron.lib")
#pragma comment (lib, "padlock.lib")
#pragma comment (lib, "ssleay32.lib")
#pragma comment (lib, "sureware.lib")
#pragma comment (lib, "ubsec.lib")

#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/storage.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/file_pool.hpp"

#include <boost/bind.hpp>

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
	
	bool make( std::string target_file, std::string outfile
		, std::function< void(int,char const *) > error_handler
		, std::function< void(int,int) > print_progress
		, std::vector<std::string> const & web_seeds, std::vector<std::string> const & trackers
		, bool use_merklefile /*= false*/, std::string root_cert /*= ""*/
		, int pad_file_limit /*= -1*/, int piece_size /*= 0*/, bool inc_sha1_hash /*= true*/
		, bool dont_follow_symlinks /*= false*/ )
	{
		char const* creator_str = "libtorrent";

		int flags = 0;

#ifndef BOOST_NO_EXCEPTIONS
		try
		{
#endif
			std::string merklefile;

			if( use_merklefile )
			{
				merklefile = outfile + "_mkl";
				flags |= create_torrent::merkle;
			}
			
			if( inc_sha1_hash )
			{
				flags |= create_torrent::calculate_file_hashes;
			}

			if( pad_file_limit > 0 )
			{
				flags |= create_torrent::optimize;
			}

			if( dont_follow_symlinks )
			{
				flags |= create_torrent::symlinks;
			}

			if( trackers.empty() )
			{
				trackers.push_back( "http://exodus.desync.com:6969/announce" );
				trackers.push_back( "http://tracker.publicbt.com/announce" );
				trackers.push_back( "http://tpb.tracker.prq.to/announce.php" );
				trackers.push_back( "http://tv.tracker.prq.to/announce" );
				trackers.push_back( "http://tracker.prq.to/announce.php" );
				trackers.push_back( "http://cpleft.com:2710/announce" );
				trackers.push_back( "http://i.bandito.org/announce" );
				trackers.push_back( "http://tracker.ex.ua/announce" );
				trackers.push_back( "http://tracker.tfile.me/announce" );
				trackers.push_back( "udp://tracker.openbittorrent.com:80/announce" );
				trackers.push_back( "udp://tracker.publicbt.com:80/announce" );
				trackers.push_back( "udp://tracker.publicbt.com:80/announce" );
				trackers.push_back( "http://open.nyaatorrents.info:6544/announce" );
				trackers.push_back( "udp://tracker.publicbt.com:80/announce" );
				trackers.push_back( "http://announce.torrentsmd.com:8080/announce" );
				trackers.push_back( "http://announce.torrentsmd.com:8080/announce.php" );
				trackers.push_back( "http://bt.home-ix.ru/announce" );
				trackers.push_back( "http://bt.home-ix.ru/announce.php" );
				trackers.push_back( "http://cpleft.com:2710/announce" );
				trackers.push_back( "http://exodus.desync.com:6969/announceexodus.desync.com:6969/announce" );
				trackers.push_back( "http://ipv4.tracker.harry.lu.nyud.net/announce" );
				trackers.push_back( "http://ipv4.tracker.harry.lu/announce" );
				trackers.push_back( "http://ix3.rutracker.net/announce" );
				trackers.push_back( "http://openbt.my-hit.ru:2710/announce" );
				trackers.push_back( "http://retracker.kld.ru/announce" );
				trackers.push_back( "http://retracker.kld.ru:2710/announce" );
				trackers.push_back( "http://tracker.ccc.de/announce" );
				trackers.push_back( "http://tracker.ex.ua/announce" );
				trackers.push_back( "http://tracker.torrentbay.to:6969/announce" );
			}

			file_storage fs;
			file_pool fp;
			std::string full_path = libtorrent::complete( target_file );

			add_files(fs, full_path, [](std::string const& f)->bool
				{
					if (filename(f)[0] == '.') return false;
					//fprintf(stderr, "%s\n", f.c_str());
					return true;
				}
			, flags);

			if (fs.num_files() == 0)
			{
				error_handler( 0, "no files specified.\n" );
				return false;
			}

			create_torrent t(fs, piece_size, pad_file_limit, flags);
			for (std::vector<std::string>::const_iterator i = trackers.begin()
				, end(trackers.end()); i != end; ++i)
				t.add_tracker(*i);

			for (std::vector<std::string>::const_iterator i = web_seeds.begin()
				, end(web_seeds.end()); i != end; ++i)
				t.add_url_seed(*i);

			error_code ec;
			set_piece_hashes(t, parent_path(full_path)
				, boost::bind(print_progress, _1, t.num_pieces()), ec);

			if (ec)
			{
				error_handler( 1, ec.message().c_str() );
				return false;
			}

			t.set_creator(creator_str);

			if (!root_cert.empty())
			{
				std::vector<char> pem;
				load_file(root_cert, pem, ec, 10000);
				if (ec)
				{
					error_handler( 2, (ec.message() + ": failed to load root certificate for tracker.").c_str() );
					return false;
				}
				else
				{
					t.set_root_cert(std::string(&pem[0], pem.size()));
				}
			}

			// create the torrent and print it to stdout
			std::vector<char> torrent;
			bencode(back_inserter(torrent), t.generate());
			FILE* output = stdout;
			if (!outfile.empty())
				output = fopen(outfile.c_str(), "wb+");
			fwrite(&torrent[0], 1, torrent.size(), output);

			if (output != stdout)
				fclose(output);

			if (!merklefile.empty())
			{
				output = fopen(merklefile.c_str(), "wb+");
				int ret = fwrite(&t.merkle_tree()[0], 20, t.merkle_tree().size(), output);
				if (ret != t.merkle_tree().size() * 20)
				{
					error_handler( 3, (merklefile + " : failed to write.").c_str() );
					fclose(output);
					return false;
				}				
			}

			fclose(output);

#ifndef BOOST_NO_EXCEPTIONS
		}
		catch (std::exception& e)
		{
			error_handler( 4, e.what() );
			return false;
		}
#endif
		return true;
	}
}