#pragma once

#include "pipeline/pipeline.h"

namespace satdump::recorder
{
    // Match LivePipeline's parameter precedence for its first IQ-processing step.
    inline double pipeline_frequency_shift(const pipeline::Pipeline &pipeline, const nlohmann::json &parameters)
    {
        if (parameters.contains("freq_shift"))
            return parameters.at("freq_shift").get<double>();

        if (!pipeline.live_cfg.normal_live.empty())
        {
            int step = pipeline.live_cfg.normal_live.front();
            if (step >= 0 && step < (int)pipeline.steps.size() && pipeline.steps[step].parameters.contains("freq_shift"))
                return pipeline.steps[step].parameters.at("freq_shift").get<double>();
        }
        return 0;
    }

    inline double pipeline_frequency(double input_frequency, double shift)
    {
        // A positive mixer shift selects a signal below the input centre.
        return input_frequency - shift;
    }

    inline bool load_source_dc_blocking(const nlohmann::json &dcfg, const std::string &source_name)
    {
        if (dcfg.is_object() && dcfg.contains(source_name) && dcfg[source_name].is_object())
        {
            const auto &cfg = dcfg[source_name];
            if (cfg.contains("dc_blocking") && cfg["dc_blocking"].is_boolean())
                return cfg["dc_blocking"].get<bool>();
        }
        return false;
    }

    inline void save_source_dc_blocking(nlohmann::json &rec_cfg, const std::string &source_name, bool dc_blocking)
    {
        if (!rec_cfg.is_object())
            rec_cfg = nlohmann::json::object();
        rec_cfg.erase("dc_blocking");
        rec_cfg[source_name]["dc_blocking"] = dc_blocking;
    }
}
