#pragma once
#include <iostream>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging
#include "spdlog/sinks/rotating_file_sink.h" // support for rotating file logging
#include "spdlog/sinks/stdout_sinks.h"


#define DEFAULT_LOG "default_log"
#define JSON_LOG "json_log"


namespace logging {

	typedef std::shared_ptr<spdlog::logger> log_ptr;

	inline void make_logger(std::string log_name = DEFAULT_LOG) {
		try {
			std::string log_filename = "logs/" + log_name + ".txt";

			spdlog::warn("console test warning");

			auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
			console_sink->set_level(spdlog::level::info);
			console_sink->set_pattern("[%^%l%$] %v");

			auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_filename, 1024*1024*25,1); // rotating 3 files, 5MB max
			file_sink->set_level(spdlog::level::trace);

			auto logger = std::make_shared<spdlog::logger>(log_name, spdlog::sinks_init_list({ console_sink, file_sink }));
			logger->warn("test warning");
			logger->flush_on(spdlog::level::info); // flush logger on every info that is being logged
			spdlog::register_logger(logger); // register logger to be used with spdlog::get
		}
		catch (const spdlog::spdlog_ex & ex) {
			std::cout << "Log initialization failed: " << ex.what() << std::endl;
		}
	}

	inline auto get_logger(std::string log_name = DEFAULT_LOG) {
		return spdlog::get(log_name);
	}

}