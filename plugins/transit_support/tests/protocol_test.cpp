#include "protocol.h"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace transit;
static void require(bool b) { if (!b) throw std::runtime_error("TRANSIT test failed"); }
static Frame make_frame(int sf, unsigned counter)
{
    Frame f; f.sf = sf; f.raw[0] = sf ? 0xe25f : 0xe240;
    f.raw[6] = 0x52f1; f.raw[14] = 0x5591; f.raw[8] = counter;
    f.raw[12] = 0xb400;
    return f;
}
static std::vector<bool> bits(const Frame &f, bool inv = false)
{
    std::vector<bool> b;
    for (auto w : f.raw) for (int i = 15; i >= 0; --i) b.push_back(((w >> i) & 1) ^ inv);
    return b;
}
int main()
{
    for (unsigned b = 0; b < 16; ++b)
    {
        unsigned gray = b ^ (b >> 1), rev = 0;
        for (int i = 0; i < 4; ++i) rev = (rev << 1) | ((gray >> i) & 1);
        require(sun_gray(rev << 12) == b);
    }
    Frame f = make_frame(0, 65535); interpret(f);
    require(f.state == 'H' && adc_counts(f.words[6]) == 303);
    for (int i = 1; i < 16; ++i) f.raw[i] = uint16_t((f.raw[i] << 1) | (f.raw[i] >> 15));
    interpret(f); require(f.state == 'R' && f.words[8] == 65535 && f.words[6] == 0x52f1);
    f.raw[14] = 0; interpret(f); require(f.state == 'X');
    for (bool inv : {false, true})
    {
        Deframer d; std::vector<Frame> frames;
        auto out = [&](Frame v) { frames.push_back(v); };
        for (int i = 0; i < 37; ++i) d.push(false, out);
        for (auto frame : {make_frame(0, 65535), make_frame(1, 65535), make_frame(0, 0)})
            for (bool b : bits(frame, inv)) d.push(b, out);
        d.finish(out);
        require(frames.size() == 3 && frames[0].bit == 37 && frames[2].raw[8] == 0 && frames[2].inverted == inv);
    }
    {
        Deframer d; int count = 0; auto out = [&](Frame) { ++count; };
        for (bool b : bits(make_frame(0, 1))) d.push(b, out);
        auto next = bits(make_frame(1, 1));
        for (int i = 0; i < 100; ++i) d.push(next[i], out);
        d.finish(out); require(count == 1); // Never fabricate the partial tail.
    }
    {
        Deframer d; int count = 0; auto out = [&](Frame) { ++count; };
        for (bool b : bits(make_frame(0, 1))) d.push(b, out);
        for (bool b : bits(make_frame(0, 2))) d.push(b, out);
        d.finish(out); require(count == 0); // Same SF cannot confirm acquisition.
    }
    {
        Deframer d; std::vector<Frame> frames; auto out = [&](Frame f) { frames.push_back(f); };
        for (int k = 0; k < 2; ++k) for (bool b : bits(make_frame(k, 1))) d.push(b, out);
        for (int i = 0; i < 11; ++i) d.push(false, out);
        for (int k = 0; k < 3; ++k) for (bool b : bits(make_frame(k % 2, 8 + k / 2))) d.push(b, out);
        d.finish(out);
        require(frames.size() == 4 && frames[1].bit == 523);
    }
    {
        LivePreview preview;
        require(!preview.synced() && preview.subframes == 0);
        for (auto f : {make_frame(0, 42), make_frame(1, 42), make_frame(0, 43)})
            for (bool b : bits(f)) preview.push(b ? 90 : -90);
        require(preview.synced() && preview.subframes == 2 && preview.latest.words[8] == 42);
        preview.finish();
        require(preview.subframes == 3 && preview.latest.words[8] == 43);
        require(preview.waveform_size == 512 && preview.waveform_next == 256);
        for (int i = 0; i < 513; ++i) preview.push(-90);
        require(!preview.synced() && preview.subframes == 3); // History must not imply ongoing lock.
        for (auto f : {make_frame(0, 99), make_frame(1, 99)})
            for (bool b : bits(f)) preview.push(b ? -90 : 90);
        require(preview.synced() && preview.latest.inverted && preview.latest.words[8] == 99);
    }
    Commutator c;
    require(c.push(1647) == -1);
    for (int i = 1; i < 42; ++i) require(c.push(100 + i) == -1);
    require(c.push(1648) == 1);
    for (int i = 1; i < 42; ++i) require(c.push(100 + i) == (i + 1) % 42);
    require(c.push(100) == -1); // Missing marker loses numbering.
    c.reset(); require(c.push(1647) == -1);
    std::cout << "TRANSIT protocol tests passed\n";
}
