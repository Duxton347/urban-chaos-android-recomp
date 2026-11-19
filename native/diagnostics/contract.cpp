#include "diagnostics/journal.h"
// Cross-compile the real journal implementation; no code is executed on the device here.
bool UrbanDiagnosticContract(const std::filesystem::path& private_directory) {
 auto& journal=urban::diagnostics::ProcessJournal();
 if(!journal.Start(private_directory,"contract",std::string(64,'a')))return false;
 journal.Emit(urban::diagnostics::Code::SessionBegin);
 journal.Observe(1,"renderer: frame %u, %u draws, %u empty frames","renderer: frame 60, 1 draws, 0 empty frames");
 bool healthy=journal.Healthy();journal.Finish();return healthy;
}
