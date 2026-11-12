#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace umizip {

inline std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    if (n <= 0) return L"";
    std::wstring out((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n);
    return out;
}

inline std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return "";
    std::string out((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), out.data(), n, nullptr, nullptr);
    return out;
}

inline uint32_t ComputeCrc32(const uint8_t* data, size_t len, uint32_t seed = 0) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        init = true;
    }
    uint32_t c = ~seed;
    for (size_t i = 0; i < len; ++i) {
        c = table[(c ^ data[i]) & 0xFFu] ^ (c >> 8);
    }
    return ~c;
}

// Clean RFC-1951 raw DEFLATE decompressor
class DeflateDecoder {
public:
    static void Inflate(const uint8_t* src, size_t src_len, std::vector<uint8_t>& out, size_t expected_size) {
        out.clear();
        out.reserve(expected_size);
        BitReader br{src, src_len, 0, 0, 0};

        bool bfinal = false;
        while (!bfinal) {
            bfinal = br.GetBits(1) != 0;
            uint32_t btype = br.GetBits(2);
            if (btype == 0) {
                br.AlignByte();
                uint32_t len = br.GetBits(16);
                uint32_t nlen = br.GetBits(16);
                if ((len ^ 0xFFFFu) != nlen) {
                    throw std::runtime_error("Bozuk DEFLATE blok uzunlugu.");
                }
                br.CopyBytes(out, len);
            } else if (btype == 1 || btype == 2) {
                HuffTable litlen{}, dist{};
                if (btype == 1) {
                    BuildFixedTables(litlen, dist);
                } else {
                    BuildDynamicTables(br, litlen, dist);
                }
                DecodeCompressedBlock(br, litlen, dist, out);
            } else {
                throw std::runtime_error("Gecersiz DEFLATE blok tipi.");
            }
        }
    }

private:
    struct BitReader {
        const uint8_t* data;
        size_t size;
        size_t pos;
        uint64_t bitbuf;
        int bitcnt;

        void Ensure(int n) {
            while (bitcnt < n) {
                if (pos < size) {
                    bitbuf |= (uint64_t)data[pos++] << bitcnt;
                    bitcnt += 8;
                } else {
                    throw std::runtime_error("DEFLATE akisi beklenmedik sekilde kesildi.");
                }
            }
        }

        uint32_t Peek(int n) {
            while (bitcnt < n && pos < size) {
                bitbuf |= (uint64_t)data[pos++] << bitcnt;
                bitcnt += 8;
            }
            return (uint32_t)(bitbuf & ((1ULL << n) - 1ULL));
        }

        void Drop(int n) {
            bitbuf >>= n;
            bitcnt -= n;
        }

        uint32_t GetBits(int n) {
            if (n == 0) return 0;
            Ensure(n);
            uint32_t v = (uint32_t)(bitbuf & ((1ULL << n) - 1ULL));
            Drop(n);
            return v;
        }

        void AlignByte() {
            int rem = bitcnt & 7;
            if (rem) Drop(rem);
        }

        void CopyBytes(std::vector<uint8_t>& out, uint32_t count) {
            while (count > 0 && bitcnt > 0) {
                out.push_back((uint8_t)(bitbuf & 0xFFu));
                Drop(8);
                --count;
            }
            if (pos + count > size) {
                throw std::runtime_error("DEFLATE ham blok sinir hatasi.");
            }
            out.insert(out.end(), data + pos, data + pos + count);
            pos += count;
        }
    };

    struct HuffTable {
        static constexpr int FAST_BITS = 9;
        uint16_t fast[1 << FAST_BITS]; // (len << 10) | sym, 0 if slow
        uint16_t counts[16];
        uint16_t symbols[320];
        uint32_t maxcode[17];
        int32_t delta[17];

        static uint32_t ReverseBits(uint32_t v, int bits) {
            uint32_t r = 0;
            for (int i = 0; i < bits; ++i) {
                r = (r << 1) | (v & 1u);
                v >>= 1;
            }
            return r;
        }

        void Build(const uint8_t* lengths, int num_syms) {
            std::memset(fast, 0, sizeof(fast));
            std::memset(counts, 0, sizeof(counts));
            for (int i = 0; i < num_syms; ++i) {
                if (lengths[i] > 15) throw std::runtime_error("Gecersiz Huffman bit uzunlugu.");
                counts[lengths[i]]++;
            }
            counts[0] = 0;

            uint32_t code = 0;
            uint32_t next_code[16]{};
            int sym_idx = 0;
            for (int bits = 1; bits <= 15; ++bits) {
                code = (code + counts[bits - 1]) << 1;
                next_code[bits] = code;
                delta[bits] = sym_idx - (int32_t)code;
                maxcode[bits] = code + counts[bits];
                sym_idx += counts[bits];
            }
            maxcode[16] = 0x10000u;

            int offsets[16]{};
            int cur = 0;
            for (int bits = 1; bits <= 15; ++bits) {
                offsets[bits] = cur;
                cur += counts[bits];
            }

            for (int s = 0; s < num_syms; ++s) {
                int len = lengths[s];
                if (len == 0) continue;
                uint32_t c = next_code[len]++;
                symbols[offsets[len]++] = (uint16_t)s;
                if (len <= FAST_BITS) {
                    uint32_t rev = ReverseBits(c, len);
                    uint16_t entry = (uint16_t)((len << 10) | s);
                    uint32_t step = 1u << len;
                    for (uint32_t idx = rev; idx < (1u << FAST_BITS); idx += step) {
                        fast[idx] = entry;
                    }
                }
            }
        }

        uint16_t Decode(BitReader& br) const {
            uint32_t look = br.Peek(15);
            uint16_t f = fast[look & ((1u << FAST_BITS) - 1u)];
            if (f != 0) {
                int len = f >> 10;
                if (br.bitcnt >= len) {
                    br.Drop(len);
                    return (uint16_t)(f & 0x3FFu);
                }
            }
            uint32_t code = 0;
            for (int bits = 1; bits <= 15; ++bits) {
                code = (code << 1) | br.GetBits(1);
                if (code < maxcode[bits]) {
                    return symbols[(int32_t)code + delta[bits]];
                }
            }
            throw std::runtime_error("Bozuk Huffman sembolu.");
        }
    };

    static void BuildFixedTables(HuffTable& litlen, HuffTable& dist) {
        uint8_t ll[288]{};
        for (int i = 0; i <= 143; ++i) ll[i] = 8;
        for (int i = 144; i <= 255; ++i) ll[i] = 9;
        for (int i = 256; i <= 279; ++i) ll[i] = 7;
        for (int i = 280; i <= 287; ++i) ll[i] = 8;
        litlen.Build(ll, 288);

        uint8_t dl[32]{};
        for (int i = 0; i < 32; ++i) dl[i] = 5;
        dist.Build(dl, 32);
    }

    static void BuildDynamicTables(BitReader& br, HuffTable& litlen, HuffTable& dist) {
        int hlit = (int)br.GetBits(5) + 257;
        int hdist = (int)br.GetBits(5) + 1;
        int hclen = (int)br.GetBits(4) + 4;

        static const int cl_order[19] = {
            16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
        };
        uint8_t cl_lens[19]{};
        for (int i = 0; i < hclen; ++i) {
            cl_lens[cl_order[i]] = (uint8_t)br.GetBits(3);
        }
        HuffTable cl_tab{};
        cl_tab.Build(cl_lens, 19);

        int total = hlit + hdist;
        uint8_t lens[320]{};
        int idx = 0;
        while (idx < total) {
            uint16_t sym = cl_tab.Decode(br);
            if (sym <= 15) {
                lens[idx++] = (uint8_t)sym;
            } else if (sym == 16) {
                if (idx == 0) throw std::runtime_error("DEFLATE tekrar kodu onceki sembol yokken geldi.");
                int rep = (int)br.GetBits(2) + 3;
                uint8_t prev = lens[idx - 1];
                while (rep-- > 0 && idx < total) lens[idx++] = prev;
            } else if (sym == 17) {
                int rep = (int)br.GetBits(3) + 3;
                while (rep-- > 0 && idx < total) lens[idx++] = 0;
            } else if (sym == 18) {
                int rep = (int)br.GetBits(7) + 11;
                while (rep-- > 0 && idx < total) lens[idx++] = 0;
            } else {
                throw std::runtime_error("Gecersiz kod uzunlugu sembolu.");
            }
        }
        litlen.Build(lens, hlit);
        dist.Build(lens + hlit, hdist);
    }

    static void DecodeCompressedBlock(BitReader& br, const HuffTable& litlen, const HuffTable& dist, std::vector<uint8_t>& out) {
        static const uint16_t len_base[29] = {
            3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
            35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
        };
        static const uint8_t len_extra[29] = {
            0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
            3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
        };
        static const uint16_t dist_base[30] = {
            1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
            257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
        };
        static const uint8_t dist_extra[30] = {
            0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
            7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
        };

        for (;;) {
            uint16_t sym = litlen.Decode(br);
            if (sym < 256) {
                out.push_back((uint8_t)sym);
            } else if (sym == 256) {
                break;
            } else {
                int lidx = (int)sym - 257;
                if (lidx < 0 || lidx >= 29) throw std::runtime_error("Gecersiz uzunluk sembolu.");
                size_t match_len = len_base[lidx] + br.GetBits(len_extra[lidx]);
                uint16_t dsym = dist.Decode(br);
                if (dsym >= 30) throw std::runtime_error("Gecersiz mesafe sembolu.");
                size_t match_dist = dist_base[dsym] + br.GetBits(dist_extra[dsym]);
                if (match_dist > out.size()) throw std::runtime_error("DEFLATE pencere disi referans.");

                size_t src_pos = out.size() - match_dist;
                if (match_dist >= match_len) {
                    out.resize(out.size() + match_len);
                    std::memcpy(out.data() + out.size() - match_len, out.data() + src_pos, match_len);
                } else {
                    for (size_t i = 0; i < match_len; ++i) {
                        out.push_back(out[src_pos + i]);
                    }
                }
            }
        }
    }
};

struct ZipEntryInfo {
    std::string name_utf8;
    uint16_t method = 0;
    uint32_t crc32 = 0;
    uint64_t comp_size = 0;
    uint64_t uncomp_size = 0;
    uint64_t local_header_offset = 0;
    bool is_dir = false;
};

class ZipReader {
public:
    explicit ZipReader(const std::filesystem::path& zip_path) {
        hFile_ = CreateFileW(
            zip_path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (hFile_ == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Paket dosyasi acilamadi: " + WideToUtf8(zip_path.wstring()));
        }
        LARGE_INTEGER sz{};
        if (!GetFileSizeEx(hFile_, &sz) || sz.QuadPart < 22) {
            CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
            throw std::runtime_error("Gecersiz veya bozuk ZIP arsivi.");
        }
        fileSize_ = (uint64_t)sz.QuadPart;
        try {
            ParseCentralDirectory();
        } catch (...) {
            CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
            throw;
        }
    }

    ~ZipReader() {
        if (hFile_ != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile_);
        }
    }

    ZipReader(const ZipReader&) = delete;
    ZipReader& operator=(const ZipReader&) = delete;

    const std::vector<ZipEntryInfo>& Entries() const { return entries_; }

    const ZipEntryInfo* FindEntry(const std::string& name) const {
        for (const auto& e : entries_) {
            if (e.name_utf8 == name) return &e;
        }
        return nullptr;
    }

    std::vector<uint8_t> ExtractToMemory(const ZipEntryInfo& e, size_t max_prefix_bytes = 0) const {
        uint8_t lhdr[30]{};
        ReadExactAt(e.local_header_offset, lhdr, 30);
        if (ReadU32(lhdr) != 0x04034B50u) {
            throw std::runtime_error("Gecersiz yerel ZIP basligi: " + e.name_utf8);
        }
        uint16_t fn_len = ReadU16(lhdr + 26);
        uint16_t ex_len = ReadU16(lhdr + 28);
        uint64_t data_offset = e.local_header_offset + 30ULL + fn_len + ex_len;

        if (e.method == 0) {
            uint64_t to_read = e.uncomp_size;
            if (max_prefix_bytes > 0 && to_read > max_prefix_bytes) {
                to_read = max_prefix_bytes;
            }
            std::vector<uint8_t> buf((size_t)to_read);
            if (to_read > 0) {
                ReadExactAt(data_offset, buf.data(), (size_t)to_read);
            }
            if (max_prefix_bytes == 0) {
                uint32_t crc = ComputeCrc32(buf.data(), buf.size());
                if (crc != e.crc32) {
                    throw std::runtime_error("CRC-32 hatasi (bozuk arsiv verisi): " + e.name_utf8);
                }
            }
            return buf;
        } else if (e.method == 8) {
            std::vector<uint8_t> comp((size_t)e.comp_size);
            if (e.comp_size > 0) {
                ReadExactAt(data_offset, comp.data(), (size_t)e.comp_size);
            }
            std::vector<uint8_t> out;
            DeflateDecoder::Inflate(comp.data(), comp.size(), out, (size_t)e.uncomp_size);
            if (out.size() != e.uncomp_size) {
                throw std::runtime_error("Acilan dosya boyutu uyusmuyor: " + e.name_utf8);
            }
            uint32_t crc = ComputeCrc32(out.data(), out.size());
            if (crc != e.crc32) {
                throw std::runtime_error("CRC-32 hatasi (bozuk arsiv verisi): " + e.name_utf8);
            }
            if (max_prefix_bytes > 0 && out.size() > max_prefix_bytes) {
                out.resize(max_prefix_bytes);
            }
            return out;
        } else {
            throw std::runtime_error("Desteklenmeyen ZIP sikistirma yontemi: " + e.name_utf8);
        }
    }

    void ExtractToFile(const ZipEntryInfo& e, const std::filesystem::path& dst_path) const {
        uint8_t lhdr[30]{};
        ReadExactAt(e.local_header_offset, lhdr, 30);
        if (ReadU32(lhdr) != 0x04034B50u) {
            throw std::runtime_error("Gecersiz yerel ZIP basligi: " + e.name_utf8);
        }
        uint16_t fn_len = ReadU16(lhdr + 26);
        uint16_t ex_len = ReadU16(lhdr + 28);
        uint64_t data_offset = e.local_header_offset + 30ULL + fn_len + ex_len;

        std::error_code ec;
        std::filesystem::create_directories(dst_path.parent_path(), ec);

        HANDLE hOut = CreateFileW(
            dst_path.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (hOut == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Hedef dosya yazilamadi (izin veya kilit hatasi): " + WideToUtf8(dst_path.wstring()));
        }

        try {
            if (e.method == 0) {
                constexpr size_t CHUNK = 524288;
                std::vector<uint8_t> buf(CHUNK);
                uint64_t rem = e.uncomp_size;
                uint64_t off = data_offset;
                uint32_t crc = 0;
                while (rem > 0) {
                    size_t step = (size_t)std::min<uint64_t>(rem, CHUNK);
                    ReadExactAt(off, buf.data(), step);
                    crc = ComputeCrc32(buf.data(), step, crc);
                    WriteExact(hOut, buf.data(), step);
                    off += step;
                    rem -= step;
                }
                if (crc != e.crc32) {
                    throw std::runtime_error("CRC-32 hatasi: " + e.name_utf8);
                }
            } else if (e.method == 8) {
                std::vector<uint8_t> out = ExtractToMemory(e);
                if (!out.empty()) {
                    WriteExact(hOut, out.data(), out.size());
                }
            } else {
                throw std::runtime_error("Desteklenmeyen ZIP sikistirma yontemi: " + e.name_utf8);
            }
            CloseHandle(hOut);
        } catch (...) {
            CloseHandle(hOut);
            DeleteFileW(dst_path.c_str());
            throw;
        }
    }

private:
    HANDLE hFile_ = INVALID_HANDLE_VALUE;
    uint64_t fileSize_ = 0;
    std::vector<ZipEntryInfo> entries_;

    static uint16_t ReadU16(const uint8_t* p) {
        return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
    }
    static uint32_t ReadU32(const uint8_t* p) {
        return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    }
    static uint64_t ReadU64(const uint8_t* p) {
        return (uint64_t)ReadU32(p) | ((uint64_t)ReadU32(p + 4) << 32);
    }

    void ReadExactAt(uint64_t offset, uint8_t* dst, size_t len) const {
        if (offset + len > fileSize_) {
            throw std::runtime_error("ZIP arsivi beklenenden kisa (bozuk indirme).");
        }
        LARGE_INTEGER pos{};
        pos.QuadPart = (LONGLONG)offset;
        if (!SetFilePointerEx(hFile_, pos, nullptr, FILE_BEGIN)) {
            throw std::runtime_error("ZIP okuma konumu ayarlanamadi.");
        }
        size_t done = 0;
        while (done < len) {
            DWORD chunk = (DWORD)std::min<size_t>(len - done, 16 * 1024 * 1024);
            DWORD got = 0;
            if (!ReadFile(hFile_, dst + done, chunk, &got, nullptr) || got == 0) {
                throw std::runtime_error("ZIP dosyasi okunurken G/C hatasi olustu.");
            }
            done += got;
        }
    }

    static void WriteExact(HANDLE hOut, const uint8_t* src, size_t len) {
        size_t done = 0;
        while (done < len) {
            DWORD chunk = (DWORD)std::min<size_t>(len - done, 16 * 1024 * 1024);
            DWORD written = 0;
            if (!WriteFile(hOut, src + done, chunk, &written, nullptr) || written != chunk) {
                throw std::runtime_error("Disk yazma hatasi (bos alan veya izin kontrol edin).");
            }
            done += written;
        }
    }

    void ParseCentralDirectory() {
        uint64_t tail_len = std::min<uint64_t>(fileSize_, 65557ULL);
        uint64_t tail_off = fileSize_ - tail_len;
        std::vector<uint8_t> tail((size_t)tail_len);
        ReadExactAt(tail_off, tail.data(), (size_t)tail_len);

        int64_t eocd_rel = -1;
        for (int64_t i = (int64_t)tail_len - 22; i >= 0; --i) {
            if (ReadU32(tail.data() + i) == 0x06054B50u) {
                eocd_rel = i;
                break;
            }
        }
        if (eocd_rel < 0) {
            throw std::runtime_error("Gecersiz ZIP dosyasi (EOCD bulunamadi).");
        }

        const uint8_t* eocd = tail.data() + eocd_rel;
        uint64_t cd_entries = ReadU16(eocd + 10);
        uint64_t cd_size = ReadU32(eocd + 12);
        uint64_t cd_offset = ReadU32(eocd + 16);

        // Check ZIP64 EOCD Locator right before EOCD (20 bytes)
        if (eocd_rel >= 20 && ReadU32(tail.data() + eocd_rel - 20) == 0x07064B50u) {
            uint64_t z64_eocd_off = ReadU64(tail.data() + eocd_rel - 20 + 8);
            uint8_t z64[56]{};
            ReadExactAt(z64_eocd_off, z64, 56);
            if (ReadU32(z64) == 0x06064B50u) {
                cd_entries = ReadU64(z64 + 32);
                cd_size = ReadU64(z64 + 40);
                cd_offset = ReadU64(z64 + 48);
            }
        }

        std::vector<uint8_t> cd((size_t)cd_size);
        if (cd_size > 0) {
            ReadExactAt(cd_offset, cd.data(), (size_t)cd_size);
        }

        entries_.reserve((size_t)cd_entries);
        size_t p = 0;
        while (p + 46 <= cd.size()) {
            if (ReadU32(cd.data() + p) != 0x02014B50u) break;
            uint16_t flags = ReadU16(cd.data() + p + 8);
            uint16_t method = ReadU16(cd.data() + p + 10);
            uint32_t crc = ReadU32(cd.data() + p + 16);
            uint64_t csize = ReadU32(cd.data() + p + 20);
            uint64_t usize = ReadU32(cd.data() + p + 24);
            uint16_t fn_len = ReadU16(cd.data() + p + 28);
            uint16_t ex_len = ReadU16(cd.data() + p + 30);
            uint16_t cm_len = ReadU16(cd.data() + p + 32);
            uint64_t lhoff = ReadU32(cd.data() + p + 42);

            if (p + 46 + fn_len + ex_len + cm_len > cd.size()) break;

            std::string raw_name((const char*)(cd.data() + p + 46), fn_len);
            if ((flags & (1u << 11)) == 0) {
                // Convert CP437/ACP to UTF-8 if UTF-8 bit is not set
                int wn = MultiByteToWideChar(CP_ACP, 0, raw_name.data(), (int)raw_name.size(), nullptr, 0);
                if (wn > 0) {
                    std::wstring w((size_t)wn, L'\0');
                    MultiByteToWideChar(CP_ACP, 0, raw_name.data(), (int)raw_name.size(), w.data(), wn);
                    raw_name = WideToUtf8(w);
                }
            }
            for (char& ch : raw_name) {
                if (ch == '\\') ch = '/';
            }

            // Parse ZIP64 Extra Field (0x0001) if present
            const uint8_t* ex = cd.data() + p + 46 + fn_len;
            size_t ep = 0;
            while (ep + 4 <= ex_len) {
                uint16_t eid = ReadU16(ex + ep);
                uint16_t esz = ReadU16(ex + ep + 2);
                if (ep + 4 + esz > ex_len) break;
                if (eid == 0x0001u) {
                    size_t zp = ep + 4;
                    if (usize == 0xFFFFFFFFULL && zp + 8 <= ep + 4 + esz) {
                        usize = ReadU64(ex + zp);
                        zp += 8;
                    }
                    if (csize == 0xFFFFFFFFULL && zp + 8 <= ep + 4 + esz) {
                        csize = ReadU64(ex + zp);
                        zp += 8;
                    }
                    if (lhoff == 0xFFFFFFFFULL && zp + 8 <= ep + 4 + esz) {
                        lhoff = ReadU64(ex + zp);
                        zp += 8;
                    }
                }
                ep += 4 + esz;
            }

            ZipEntryInfo info;
            info.name_utf8 = raw_name;
            info.method = method;
            info.crc32 = crc;
            info.comp_size = csize;
            info.uncomp_size = usize;
            info.local_header_offset = lhoff;
            info.is_dir = !raw_name.empty() && raw_name.back() == '/';
            entries_.push_back(std::move(info));

            p += 46 + fn_len + ex_len + cm_len;
        }
    }
};

} // namespace umizip
