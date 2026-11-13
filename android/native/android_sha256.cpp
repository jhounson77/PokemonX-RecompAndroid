#include "core/sha256.h"
#include <jni.h>
#include <algorithm>
#include <format>
#include <stdexcept>
#include <utility>
extern JavaVM* PxJavaVM;
namespace hexlith::core {
namespace {
constexpr std::size_t HexDigitsPerByte=2, HexDigestLength=64;
constexpr std::uint8_t FirstLetterValue=10, LowNibbleMask=15;
JNIEnv* Env(){JNIEnv* e=nullptr;if(PxJavaVM->GetEnv(reinterpret_cast<void**>(&e),JNI_VERSION_1_6)!=JNI_OK)throw std::runtime_error("Hash called from a detached Java thread");return e;}
void Check(JNIEnv*e){if(e->ExceptionCheck()){e->ExceptionClear();throw std::runtime_error("Android SHA-256 failed");}}
Result<std::uint8_t> ParseHexDigit(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return Error{"Invalid hex digit"};}
}
struct Sha256::Context {
 jobject digest=nullptr;jmethodID update=nullptr,finish=nullptr;
 Context(){auto*e=Env();auto cls=e->FindClass("java/security/MessageDigest");Check(e);auto method=e->GetStaticMethodID(cls,"getInstance","(Ljava/lang/String;)Ljava/security/MessageDigest;");auto name=e->NewStringUTF("SHA-256");auto obj=e->CallStaticObjectMethod(cls,method,name);Check(e);digest=e->NewGlobalRef(obj);update=e->GetMethodID(cls,"update","([B)V");finish=e->GetMethodID(cls,"digest","()[B");e->DeleteLocalRef(obj);e->DeleteLocalRef(name);e->DeleteLocalRef(cls);Check(e);}
 ~Context(){Env()->DeleteGlobalRef(digest);}
};
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
void Sha256::Update(ByteSpan data){auto*e=Env();while(!data.empty()){auto n=static_cast<jsize>(std::min<std::size_t>(data.size(),1024*1024));auto b=e->NewByteArray(n);Check(e);e->SetByteArrayRegion(b,0,n,reinterpret_cast<const jbyte*>(data.data()));e->CallVoidMethod(context_->digest,context_->update,b);e->DeleteLocalRef(b);Check(e);data=data.subspan(n);}}
Digest Sha256::Finish(){auto*e=Env();auto b=static_cast<jbyteArray>(e->CallObjectMethod(context_->digest,context_->finish));Check(e);Digest d;if(e->GetArrayLength(b)!=32)throw std::runtime_error("Invalid SHA size");e->GetByteArrayRegion(b,0,32,reinterpret_cast<jbyte*>(d.bytes.data()));e->DeleteLocalRef(b);Check(e);return d;}
Digest Sha256::Of(ByteSpan data){Sha256 h;h.Update(data);return h.Finish();}
}
