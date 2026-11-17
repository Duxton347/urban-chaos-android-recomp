#pragma once
#include "kernel/disc.h"
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace urban::game {
struct SourceError : std::runtime_error {
    std::string code;
    SourceError(std::string code_,std::string message):std::runtime_error(std::move(message)),code(std::move(code_)){}
};
struct Profile { std::string xbe_sha256;uint32_t title_id,entry; };
struct Metadata { std::string sha256;uint32_t title_id=0,version=0,entry=0;bool exact_revision=false; };
class Source {
    std::unique_ptr<kernel::Disc> disc_;
    bool activated_=false;
public:
    Metadata metadata;
    std::vector<uint8_t> xbe;
    std::filesystem::path game_directory;
    bool is_iso=false;
    Source()=default;
    Source(Source&&)=default;
    Source& operator=(Source&&)=default;
    static Source Open(const std::filesystem::path&,const Profile&);
    // Before Boot, with no guest threads from a previous installation still using D:.
    // Refusal leaves the mounted disc unchanged. Acceptance is not a gameplay qualification.
    void Activate(bool allow_other_revision=false);
};
Metadata InspectXbe(const std::vector<uint8_t>&);
}
