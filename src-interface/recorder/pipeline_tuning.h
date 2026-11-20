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
}
