// Copyright (c) 2026 Terence Tarvis
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <string_view>
#include "CLI11.hpp"
#include "processor.h"
#include "simdjson.h"

#ifdef DEBUG
#define LOG_DEBUG(msg) std::cout << msg << std::endl
#else
#define LOG_DEBUG(msg)
#endif

using namespace simdjson;

auto non_empty_file = CLI::Validator([](const std::string& filename) -> std::string {
    std::ifstream f(filename, std::ios::binary | std::ios::ate);
    if (!f) return "cannot open file: " + filename;
    if (f.tellg() == 0) return "file is empty: " + filename;
    return "";  // empty string = valid
}, "NON_EMPTY_FILE");

void setup_cli(CLI::App& app, ProgramOptions& options) {
	app.require_subcommand(1);

	// We're going to reuse this variable so no extra conditionals need to be
	// created to get the subcommand at the end

	// fingerprint subcommand
	auto cmd = app.add_subcommand("fingerprint", "fingerprints files");
	cmd->add_option("files", options.input_files, "Files to fingerprint")
		->required()
		->check(CLI::ExistingFile)
		->check(non_empty_file);
	cmd->add_flag("-T,--single", options.single, "run fingerprint in single core mode"); 

	// hash subcommand
	cmd = app.add_subcommand("hash", "hash files");
	cmd->add_option("files", options.input_files, "files to hash")
		->required()
		->check(CLI::ExistingFile)
		->check(non_empty_file);

	// compare subcommand
	cmd = app.add_subcommand("compare", "similarity of files");
	cmd->add_option("files", options.input_files, "files to compare")
		->required()
		->check(CLI::ExistingFile)
		->check(non_empty_file)
		->expected(2);
}

int main(int argc, char *argv[]) {
	CLI::App app{"dataprint CLI options"};
	ProgramOptions options;

	setup_cli(app, options);
	CLI11_PARSE(app, argc, argv);

	for (auto* subcom : app.get_subcommands()) {
		options.cmd = subcom->get_name();
	}
	
	Processor processor(options);
	processor.run();

	return 0;
}
