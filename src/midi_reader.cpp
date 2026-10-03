#include "midi_reader.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <iostream>

namespace {
inline uint16_t rd16(const uint8_t* p){ return (uint16_t)((p[0]<<8)|p[1]); }
inline uint32_t rd32(const uint8_t* p){
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
uint32_t read_vlq(const uint8_t* d, size_t end, size_t& p){
    uint32_t v=0;
    while (p<end){ uint8_t b=d[p++]; v=(v<<7)|(b&0x7F); if(!(b&0x80)) break; }
    return v;
}
struct TempoEv { double tick; double bpm; };
struct RawNote { int pitch, vel, ch, prog, tr; double t0, t1; };
} // namespace

bool load_midi_w(const std::wstring& path, MidiFile& out) {
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) { std::wcerr << L"Cannot open: " << path << L"\n"; return false; }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 14) { fclose(f); std::cerr << "File too small\n"; return false; }

    std::vector<uint8_t> buf((size_t)sz);
    if (fread(buf.data(), 1, (size_t)sz, f) != (size_t)sz) { fclose(f); return false; }
    fclose(f);

    if (memcmp(buf.data(), "MThd", 4) != 0) {
        std::cerr << "Not a Standard MIDI File\n"; return false;
    }
    uint16_t ntrks = rd16(&buf[10]);
    uint16_t div   = rd16(&buf[12]);
    if (div & 0x8000) { std::cerr << "SMPTE timing not supported\n"; return false; }

    out.tracks = ntrks;
    out.ticks_per_qn = div;
    double tpq = (double)div;

    size_t pos = 14;
    std::vector<TempoEv> tempos;
    std::vector<RawNote> raw;
    std::vector<RawNote> pending;

    int program[16]; for (int i=0;i<16;++i) program[i]=0;

    for (int tr = 0; tr < ntrks && pos + 8 <= buf.size(); ++tr) {
        if (memcmp(&buf[pos], "MTrk", 4) != 0) break;
        uint32_t trklen = rd32(&buf[pos+4]);
        pos += 8;
        size_t trkend = pos + trklen;
        if (trkend > buf.size()) trkend = buf.size();

        double cur_tick = 0.0;
        uint8_t running = 0;

        while (pos < trkend) {
            uint32_t dt = read_vlq(buf.data(), trkend, pos);
            cur_tick += dt;
            if (pos >= trkend) break;

            uint8_t b = buf[pos];
            uint8_t status;
            if (b & 0x80) { status = b; pos++; if (status < 0xF0) running = status; }
            else          { status = running; }

            if (status == 0xFF) {
                if (pos >= trkend) break;
                uint8_t mt = buf[pos++];
                uint32_t ml = read_vlq(buf.data(), trkend, pos);
                if (mt == 0x51 && ml == 3 && pos+3 <= trkend) {
                    uint32_t uspq = ((uint32_t)buf[pos]<<16)|((uint32_t)buf[pos+1]<<8)|buf[pos+2];
                    if (uspq) tempos.push_back({cur_tick, 60000000.0/uspq});
                }
                pos += ml;
            } else if (status == 0xF0 || status == 0xF7) {
                uint32_t ml = read_vlq(buf.data(), trkend, pos);
                pos += ml;
            } else {
                uint8_t cmd = status & 0xF0;
                uint8_t ch  = status & 0x0F;
                int dlen = (cmd==0xC0 || cmd==0xD0) ? 1 : 2;
                if (pos + dlen > trkend) break;
                uint8_t d1 = buf[pos];
                uint8_t d2 = (dlen==2) ? buf[pos+1] : 0;
                pos += dlen;

                if (cmd == 0xC0) {
                    program[ch] = d1;
                } else if (cmd == 0x90 && d2 > 0) {
                    pending.push_back({d1,(int)d2,(int)ch,program[ch],tr,cur_tick,0});
                } else if (cmd == 0x80 || (cmd == 0x90 && d2 == 0)) {
                    for (auto it = pending.begin(); it != pending.end(); ++it) {
                        if (it->pitch==d1 && it->ch==ch && it->tr==tr) {
                            it->t1 = cur_tick;
                            raw.push_back(*it);
                            pending.erase(it);
                            break;
                        }
                    }
                }
            }
        }
        pos = trkend;
    }

    if (tempos.empty()) tempos.push_back({0.0,120.0});
    std::sort(tempos.begin(), tempos.end(),
              [](const TempoEv&a, const TempoEv&b){return a.tick<b.tick;});
    out.tempo_bpm = tempos[0].bpm;

    auto tick_to_sec = [&](double tick) -> double {
        double sec=0, prev=0, bpm=tempos[0].bpm;
        for (size_t i=1;i<tempos.size();++i){
            if (tempos[i].tick >= tick) break;
            sec += (tempos[i].tick - prev)/tpq*(60.0/bpm);
            prev = tempos[i].tick;
            bpm  = tempos[i].bpm;
        }
        sec += (tick - prev)/tpq*(60.0/bpm);
        return sec;
    };

    out.notes.reserve(raw.size());
    for (auto& r : raw) {
        MidiNote mn;
        mn.pitch=r.pitch; mn.velocity=r.vel; mn.channel=r.ch;
        mn.program=r.prog; mn.track=r.tr;
        mn.start_sec = tick_to_sec(r.t0);
        double t1 = tick_to_sec(r.t1);
        mn.dur_sec = t1 - mn.start_sec;
        out.notes.push_back(mn);
        out.total_sec = std::max(out.total_sec, t1);
    }
    return true;
}
