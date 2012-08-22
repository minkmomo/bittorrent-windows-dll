#pragma once

namespace libtorrent
{
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

	char const* esc(char const* code)
	{
#ifdef ANSI_TERMINAL_COLORS
		// this is a silly optimization
		// to avoid copying of strings
		enum { num_strings = 200 };
		static char buf[num_strings][20];
		static int round_robin = 0;
		char* ret = buf[round_robin];
		++round_robin;
		if (round_robin >= num_strings) round_robin = 0;
		ret[0] = '\033';
		ret[1] = '[';
		int i = 2;
		int j = 0;
		while (code[j]) ret[i++] = code[j++];
		ret[i++] = 'm';
		ret[i++] = 0;
		return ret;
#else
		return "";
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	//

	void print_peer_info(std::string& out, PeerInfos const& peers)
	{
		char str[1024];
		for (auto i = peers.begin(); i != peers.end(); ++i)
		{
			peer_info * peerInfo = i->get();

			if (peerInfo->flags & (peer_info::handshake | peer_info::connecting | peer_info::queued))
				continue;

			//if (print_ip)
			{
				snprintf(str, sizeof(str), "peer : %-30s %-22s", (print_endpoint(peerInfo->ip) +
					(peerInfo->connection_type == peer_info::bittorrent_utp ? " [uTP]" : "")).c_str()
					, print_endpoint(peerInfo->local_endpoint).c_str());
				out += str;
				out += "\n";
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//

	std::string const& progress_bar(int progress, int width, char const* code = "33")
	{
		static std::string bar;
		bar.clear();
		bar.reserve(width + 10);

		int progress_chars = (progress * width + 500) / 1000;
		bar = esc(code);
		std::fill_n(std::back_inserter(bar), progress_chars, '#');
		std::fill_n(std::back_inserter(bar), width - progress_chars, '-');
		bar += esc("0");
		return bar;
	}

	//////////////////////////////////////////////////////////////////////////
	//

	std::string to_string(int v, int width)
	{
		char buf[100];
		snprintf(buf, sizeof(buf), "%*d", width, v);
		return buf;
	}

	//////////////////////////////////////////////////////////////////////////
	//

	std::string& to_string(float v, int width, int precision = 3)
	{
		// this is a silly optimization
		// to avoid copying of strings
		enum { num_strings = 20 };
		static std::string buf[num_strings];
		static int round_robin = 0;
		std::string& ret = buf[round_robin];
		++round_robin;
		if (round_robin >= num_strings) round_robin = 0;
		ret.resize(20);
		int size = snprintf(&ret[0], 20, "%*.*f", width, precision, v);
		ret.resize((std::min)(size, width));
		return ret;
	}

	//////////////////////////////////////////////////////////////////////////
	//

	std::string add_suffix(float val, char const* suffix = 0)
	{
		std::string ret;
		if (val == 0)
		{
			ret.resize(4 + 2, ' ');
			if (suffix) ret.resize(4 + 2 + strlen(suffix), ' ');
			return ret;
		}

		const char* prefix[] = {"kB", "MB", "GB", "TB"};
		const int num_prefix = sizeof(prefix) / sizeof(const char*);
		for (int i = 0; i < num_prefix; ++i)
		{
			val /= 1000.f;
			if (std::fabs(val) < 1000.f)
			{
				ret = to_string(val, 4);
				ret += prefix[i];
				if (suffix) ret += suffix;
				return ret;
			}
		}
		ret = to_string(val, 4);
		ret += "PB";
		if (suffix) ret += suffix;
		return ret;
	}

	//////////////////////////////////////////////////////////////////////////
	//

	void clear_home()
	{
		CONSOLE_SCREEN_BUFFER_INFO si;
		HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(h, &si);
		COORD c = {0, 0};
		DWORD n;
		FillConsoleOutputCharacter(h, ' ', si.dwSize.X * si.dwSize.Y, c, &n);
		SetConsoleCursorPosition(h, c);
	}

	//////////////////////////////////////////////////////////////////////////
}