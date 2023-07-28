#pragma once

struct Log
{
	enum class ESource
	{
		System,
		Client,
		Server,
	};

	enum class ELevel
	{
		Error,
		Warning,
		Info,

#ifndef NDEBUG
		Debug,
#endif
	};

	static ELevel s_setting;

	template<typename ... TArgs>
	inline static void Write(ELevel level, ESource source, char const* fmt, TArgs... args)
	{
#ifdef NDEBUG
		if (s_setting >= level)
#endif
		{
			DoWrite(level, source, fmt, args...);
		}
	}

	template<typename ... TArgs>
	inline static void Info(char const* fmt, TArgs... args)       { Write(ELevel::Info, ESource::System, fmt, args...); }
	template<typename ... TArgs>
	inline static void InfoServer(char const* fmt, TArgs... args) { Write(ELevel::Info, ESource::Server, fmt, args...); }
	template<typename ... TArgs>
	inline static void InfoClient(char const* fmt, TArgs... args) { Write(ELevel::Info, ESource::Client, fmt, args...); }

	template<typename ... TArgs>
	inline static void Warn(char const* fmt, TArgs... args)       { Write(ELevel::Warning, ESource::System, fmt, args...); }
	template<typename ... TArgs>
	inline static void WarnServer(char const* fmt, TArgs... args) { Write(ELevel::Warning, ESource::Server, fmt, args...); }
	template<typename ... TArgs>
	inline static void WarnClient(char const* fmt, TArgs... args) { Write(ELevel::Warning, ESource::Client, fmt, args...); }

	template<typename ... TArgs>
	inline static void Error(char const* fmt, TArgs... args)       { Write(ELevel::Error, ESource::System, fmt, args...); }
	template<typename ... TArgs>
	inline static void ErrorServer(char const* fmt, TArgs... args) { Write(ELevel::Error, ESource::Server, fmt, args...); }
	template<typename ... TArgs>
	inline static void ErrorClient(char const* fmt, TArgs... args) { Write(ELevel::Error, ESource::Client, fmt, args...); }

#ifndef NDEBUG
	template<typename ... TArgs>
	inline static void Debug(char const* fmt, TArgs... args)       { Write(ELevel::Debug, ESource::System, fmt, args...); }
	template<typename ... TArgs>
	inline static void DebugServer(char const* fmt, TArgs... args) { Write(ELevel::Debug, ESource::Server, fmt, args...); }
	template<typename ... TArgs>
	inline static void DebugClient(char const* fmt, TArgs... args) { Write(ELevel::Debug, ESource::Client, fmt, args...); }
#else
	template<typename ... TArgs>
	inline static void Debug(char const*, TArgs...)       { }
	template<typename ... TArgs>
	inline static void DebugServer(char const*, TArgs...) { }
	template<typename ... TArgs>
	inline static void DebugClient(char const*, TArgs...) { }
#endif

private:
	static void DoWrite(ELevel level, ESource source, char const* fmt, ...);
};
