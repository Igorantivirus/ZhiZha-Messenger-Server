#include <Tls/TlsProvider.hpp>

#include <fstream>
#include <memory>
#include <stdexcept>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

namespace
{

// RAII-обёртки над сырыми OpenSSL-хэндлами.
using BioPtr = std::unique_ptr<BIO, decltype(&BIO_free)>;
using X509Ptr = std::unique_ptr<X509, decltype(&X509_free)>;
using PkeyPtr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;

// Имена автогенерируемых файлов в рабочей директории.
constexpr const char *GENERATED_CERT_NAME = "tls-cert.pem";
constexpr const char *GENERATED_KEY_NAME = "tls-key.pem";
constexpr const char *FINGERPRINT_NAME = "tls-cert.sha256";

[[noreturn]] void fail(const std::string &what)
{
    throw std::runtime_error("TLS: " + what);
}

X509Ptr loadCert(const std::string &path)
{
    BioPtr bio(BIO_new_file(path.c_str(), "r"), &BIO_free);
    if (!bio)
        fail("cannot open certificate file: " + path);
    X509Ptr cert(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr), &X509_free);
    if (!cert)
        fail("cannot parse PEM certificate: " + path);
    return cert;
}

PkeyPtr loadKey(const std::string &path)
{
    BioPtr bio(BIO_new_file(path.c_str(), "r"), &BIO_free);
    if (!bio)
        fail("cannot open private key file: " + path);
    PkeyPtr key(PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr), &EVP_PKEY_free);
    if (!key)
        fail("cannot parse PEM private key: " + path);
    return key;
}

// Пара валидна: ключ соответствует сертификату, срок действия не истёк.
void validatePair(X509 &cert, EVP_PKEY &key, const std::string &certPath)
{
    if (X509_check_private_key(&cert, &key) != 1)
        fail("private key does not match certificate: " + certPath);
    if (X509_cmp_current_time(X509_get0_notAfter(&cert)) <= 0)
        fail("certificate is expired: " + certPath);
    if (X509_cmp_current_time(X509_get0_notBefore(&cert)) >= 0)
        fail("certificate is not yet valid: " + certPath);
}

// SHA-256 от DER-представления сертификата, lowercase hex.
std::string fingerprintOf(X509 &cert)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    if (X509_digest(&cert, EVP_sha256(), digest, &length) != 1)
        fail("cannot compute certificate fingerprint");

    static constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(length * 2);
    for (unsigned int i = 0; i < length; ++i)
    {
        result.push_back(hex[digest[i] >> 4]);
        result.push_back(hex[digest[i] & 0x0F]);
    }
    return result;
}

// Генерация самоподписанной пары: EC P-256, SHA-256, срок ~10 лет.
// Клиент доверяет по fingerprint из ссылки (pinning), поэтому содержимое
// subject/SAN роли для безопасности не играет — заполняем минимально.
void generateSelfSigned(const std::string &certPath, const std::string &keyPath, const std::string &commonName)
{
    PkeyPtr key(EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "P-256"), &EVP_PKEY_free);
    if (!key)
        fail("cannot generate EC P-256 key");

    X509Ptr cert(X509_new(), &X509_free);
    if (!cert)
        fail("cannot allocate certificate");

    X509_set_version(cert.get(), X509_VERSION_3);

    // Случайный положительный серийник — коллизии между перегенерациями не страшны,
    // но нулевой/повторяющийся серийник ломает некоторые TLS-стеки.
    std::uint64_t serial = 0;
    if (RAND_bytes(reinterpret_cast<unsigned char *>(&serial), sizeof(serial)) != 1)
        fail("cannot generate random serial number");
    ASN1_INTEGER_set_uint64(X509_get_serialNumber(cert.get()), serial >> 1);

    X509_gmtime_adj(X509_getm_notBefore(cert.get()), 0);
    X509_gmtime_adj(X509_getm_notAfter(cert.get()), 60L * 60 * 24 * 365 * 10); // 10 лет

    if (X509_set_pubkey(cert.get(), key.get()) != 1)
        fail("cannot set certificate public key");

    // Собираем subject отдельно: X509_set_subject_name/issuer_name копируют имя,
    // поэтому не упираемся в const-указатель из X509_get_subject_name.
    X509_NAME *name = X509_NAME_new();
    if (!name)
        fail("cannot allocate subject name");

    if (X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char *>(commonName.c_str()),
                                   -1, -1, 0) != 1)
    {
        X509_NAME_free(name);
        fail("cannot set certificate common name");
    }

    X509_set_subject_name(cert.get(), name);
    X509_set_issuer_name(cert.get(), name); // self-signed: issuer == subject
    X509_NAME_free(name);

    // Минимальный SAN — некоторые клиентские стеки требуют его наличия.
    X509V3_CTX extCtx;
    X509V3_set_ctx(&extCtx, cert.get(), cert.get(), nullptr, nullptr, 0);
    X509_EXTENSION *san = X509V3_EXT_conf_nid(nullptr, &extCtx, NID_subject_alt_name,
                                              "DNS:localhost,IP:127.0.0.1");
    if (!san)
        fail("cannot create subjectAltName extension");
    X509_add_ext(cert.get(), san, -1);
    X509_EXTENSION_free(san);

    if (X509_sign(cert.get(), key.get(), EVP_sha256()) == 0)
        fail("cannot sign generated certificate");

    // Сохраняем PEM на диск: пара переиспользуется при следующих запусках,
    // чтобы fingerprint (и ссылка-приглашение) оставался стабильным.
    BioPtr certBio(BIO_new_file(certPath.c_str(), "w"), &BIO_free);
    if (!certBio || PEM_write_bio_X509(certBio.get(), cert.get()) != 1)
        fail("cannot write generated certificate: " + certPath);

    BioPtr keyBio(BIO_new_file(keyPath.c_str(), "w"), &BIO_free);
    if (!keyBio || PEM_write_bio_PrivateKey(keyBio.get(), key.get(),
                                            nullptr, nullptr, 0, nullptr, nullptr) != 1)
        fail("cannot write generated private key: " + keyPath);
}

std::string readFingerprintFile(const std::filesystem::path &path)
{
    std::ifstream file(path);
    if (!file)
        return {};
    std::string value;
    file >> value;
    return value;
}

void writeFingerprintFile(const std::filesystem::path &path, const std::string &fingerprint)
{
    std::ofstream file(path, std::ios::trunc);
    if (file)
        file << fingerprint << '\n';
}

} // namespace

namespace tls
{

TlsMaterial TlsProvider::prepare(const app::TlsConfig &config, const std::filesystem::path &workDir)
{
    const std::filesystem::path dir = workDir.empty() ? std::filesystem::path(".") : workDir;

    TlsMaterial material;

    const bool hasCert = !config.certFile.empty();
    const bool hasKey = !config.keyFile.empty();
    if (hasCert != hasKey)
        fail("certFile and keyFile must be set together (or both left empty for self-signed)");

    if (hasCert)
    {
        // Пользовательская пара: fail-fast при любой проблеме.
        material.certFile = config.certFile;
        material.keyFile = config.keyFile;
        material.selfSigned = false;
    }
    else
    {
        // Автогенерация: переиспользуем ранее сгенерированную пару, если она
        // ещё валидна; иначе создаём новую.
        material.certFile = (dir / GENERATED_CERT_NAME).string();
        material.keyFile = (dir / GENERATED_KEY_NAME).string();
        material.selfSigned = true;

        const bool exists = std::filesystem::exists(material.certFile) &&
                            std::filesystem::exists(material.keyFile);
        bool usable = false;
        if (exists)
        {
            try
            {
                auto cert = loadCert(material.certFile);
                auto key = loadKey(material.keyFile);
                validatePair(*cert, *key, material.certFile);
                usable = true;
            }
            catch (const std::exception &)
            {
                // Битая/просроченная автогенерированная пара — перегенерируем.
                usable = false;
            }
        }
        if (!usable)
            generateSelfSigned(material.certFile, material.keyFile, config.scheme);
    }

    // Общая проверка и fingerprint (для пользовательской пары это и есть
    // fail-fast валидация; для свежесгенерированной — просто вычисление).
    auto cert = loadCert(material.certFile);
    auto key = loadKey(material.keyFile);
    validatePair(*cert, *key, material.certFile);
    material.fingerprint = fingerprintOf(*cert);

    // Детекция смены ссылки между запусками.
    const std::filesystem::path fingerprintPath = dir / FINGERPRINT_NAME;
    const std::string previous = readFingerprintFile(fingerprintPath);
    material.fingerprintChanged = !previous.empty() && previous != material.fingerprint;
    writeFingerprintFile(fingerprintPath, material.fingerprint);

    return material;
}

} // namespace tls
