#include "core/sha256.h"
#include <cryptopp/sha.h>
#include <format>
#include <utility>
namespace hexlith::core {
namespace {
constexpr std::size_t HexDigitsPerByte=2, HexDigestLength=64;
constexpr std::uint8_t LowNibbleMask=15;
Result<std::uint8_t> ParseHexDigit(char c){
 if(c>='0'&&c<='9')return static_cast<std::uint8_t>(c-'0');
 if(c>='a'&&c<='f')return static_cast<std::uint8_t>(c-'a'+10);
 if(c>='A'&&c<='F')return static_cast<std::uint8_t>(c-'A'+10);
 return Error{"invalid hexadecimal digit"};
}
}
struct Sha256::Context { CryptoPP::SHA256 digest; };
std::string Digest::ToHex() const {
    static constexpr std::string_view HexDigits = "0123456789abcdef";

    std::string text;
    text.reserve(HexDigestLength);
    for (const std::uint8_t byte : bytes) {
        text.push_back(HexDigits[byte >> 4U]);
        text.push_back(HexDigits[byte & LowNibbleMask]);
    }
    return text;
}

Result<Digest> ParseDigestHex(std::string_view text) {
    if (text.size() != HexDigestLength) {
        return Error{std::format(
            "a SHA-256 digest is {} hexadecimal digits, not {}", HexDigestLength, text.size())};
    }

    Digest digest;
    for (std::size_t index = 0; index < Digest::SizeInBytes; ++index) {
        const Result<std::uint8_t> high = ParseHexDigit(text[index * HexDigitsPerByte]);
        if (!high) {
            return high.Failure();
        }
        const Result<std::uint8_t> low = ParseHexDigit(text[(index * HexDigitsPerByte) + 1]);
        if (!low) {
            return low.Failure();
        }
        digest.bytes[index] = static_cast<std::uint8_t>((high.Value() << 4U) | low.Value());
    }
    return digest;
}

Result<Digest> ReadDigestAt(ByteSpan data, std::size_t offset) {
    const Result<ByteSpan> stored = ReadBytesAt(data, offset, Digest::SizeInBytes);
    if (!stored) {
        return stored.Failure();
    }

    Digest digest;
    for (std::size_t index = 0; index < Digest::SizeInBytes; ++index) {
        digest.bytes[index] = stored.Value()[index];
    }
    return digest;
}

Sha256::Sha256():context_(std::make_unique<Context>()){}
Sha256::~Sha256()=default;
Sha256::Sha256(Sha256&&) noexcept=default;
Sha256& Sha256::operator=(Sha256&&) noexcept=default;
void Sha256::Update(ByteSpan data){context_->digest.Update(data.data(),data.size());}
Digest Sha256::Finish(){Digest d;context_->digest.Final(d.bytes.data());return d;}
Digest Sha256::Of(ByteSpan b){Sha256 s;s.Update(b);return s.Finish();}
}
