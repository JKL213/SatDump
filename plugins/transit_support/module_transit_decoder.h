#pragma once
#include "pipeline/module.h"
#include "protocol.h"
#include <mutex>

namespace transit
{
    class TransitDecoderModule : public satdump::pipeline::ProcessingModule
    {
        std::atomic<uint64_t> bytes{0}, frames{0};
        std::atomic<uint64_t> filesize{0};
        std::mutex ui_mutex;
        Frame latest;
        std::array<int, 42> channels{};
        std::vector<float> counter_history, sun_history, pam_history;
        uint64_t healthy = 0, rotated = 0, suspect = 0;
        double baud;
    public:
        TransitDecoderModule(std::string, std::string, nlohmann::json);
        void process() override;
        void drawUI(bool window) override;
        nlohmann::json getModuleStats() override;
        static std::string getID() { return "transit_telemetry_decoder"; }
        std::string getIDM() override { return getID(); }
        static nlohmann::json getParams() { return {}; }
        static std::shared_ptr<ProcessingModule> getInstance(std::string i, std::string o, nlohmann::json p)
        { return std::make_shared<TransitDecoderModule>(i, o, p); }
    };
}
