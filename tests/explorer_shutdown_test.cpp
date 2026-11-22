#include "core/plugin.h"
#include "explorer/explorer.h"
#include "handlers/dummy_handler.h"
#include "logger.h"
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>

// Model the recorder's destructor starting final processing. No SDR, GUI,
// settings writes or recordings are needed to exercise explorer ownership.
class FinalProcessing : public satdump::handlers::DummyHandler
{
    std::thread worker;
    bool &joined;

public:
    FinalProcessing(std::atomic<bool> &finished, bool &joined)
        : DummyHandler("final processing"), worker([&finished] { finished = true; }), joined(joined) {}
    ~FinalProcessing()
    {
        worker.join();
        joined = true;
    }
};

class Recorder : public satdump::handlers::DummyHandler
{
    std::atomic<bool> &finished;
    bool &joined;
    bool grouped;

public:
    Recorder(std::atomic<bool> &finished, bool &joined, bool grouped)
        : DummyHandler("recorder test"), finished(finished), joined(joined), grouped(grouped) {}
    std::string getID() override { return grouped ? "recorder" : "test_recorder"; }
    ~Recorder()
    {
        satdump::eventBus->fire_event<satdump::explorer::ExplorerAddHandlerEvent>(
            {std::make_shared<FinalProcessing>(finished, joined), false, true});
    }
};

int main()
{
    logger = std::make_shared<slog::Logger>();
    for (bool grouped : {false, true})
        for (bool selected : {false, true})
        {
            satdump::eventBus = std::make_shared<satdump::EventBus>();
            std::atomic<bool> finished{false};
            bool joined = false;
            {
                satdump::explorer::ExplorerApplication explorer;
                // Shutdown must drain pending creation before releasing handlers.
                explorer.tryOpenSomethingInExplorer([&](auto *app)
                {
                    app->addHandler(std::make_shared<Recorder>(finished, joined, grouped), selected);
                });
            }
            if (!finished || !joined)
                throw std::runtime_error("Final processing was not joined during explorer shutdown");
        }
    std::cout << "Explorer shutdown: pending loads, selected/grouped recorders and final processing passed\n";
}
