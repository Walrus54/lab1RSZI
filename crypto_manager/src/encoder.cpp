#include "encoder.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <memory>

#include <QFile>
#include <QSaveFile>

#include "logger.h"

namespace {

/// Сигнатура формата файла, по которой определяется, что файл уже зашифрован.
constexpr std::array<char, 8> kMagic = {'R', 'F', 'E', 'G', 'C', 'M', '0', '1'};
constexpr int kMagicSize =
    static_cast<int>(kMagic.size());  ///< Размер сигнатуры kMagic в байтах.
constexpr quint8 kVersion = 1;  ///< Версия бинарного формата заголовка.
constexpr int kSaltSize = 16;  ///< Размер соли PBKDF2 (байт).
constexpr int kIvSize = 12;  ///< Рекомендованный размер nonce/IV для AES-GCM (байт).
constexpr int kTagSize = 16;  ///< Размер тега аутентичности GCM (байт).
constexpr int kKeySize = 32;  ///< Размер ключа AES-256 (байт).
constexpr int kPbkdf2Iterations = 120000;  ///< Число итераций PBKDF2 против brute-force.
constexpr int kVersionSize = 1;
constexpr int kHeaderSize =
    kMagicSize + kVersionSize + kSaltSize + kIvSize + kTagSize;  ///< Размер заголовка.

using EvpCipherCtxPtr = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

}  // namespace

Encoder& Encoder::instance() {
  static Encoder encoder;
  return encoder;
}

Encoder::Result Encoder::encryptFile(const QString& filePath,
                                     const QString& password) const {
  if (isEncryptedFile(filePath)) {
    return Result::kAlreadyEncrypted;
  }

  QByteArray plainText;
  Result result = readAll(filePath, plainText);
  if (result != Result::kSuccess) {
    return result;
  }
  if (plainText.isEmpty()) {
    return Result::kEmptyFile;
  }

  QByteArray salt(kSaltSize, 0);
  QByteArray iv(kIvSize, 0);
  if (RAND_bytes(reinterpret_cast<unsigned char*>(salt.data()), salt.size()) != 1 ||
      RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), iv.size()) != 1) {
    Logger::instance().error(
        QString("OpenSSL RAND_bytes failed: %1").arg(lastOpenSslError()));
    return Result::kCryptoError;
  }

  const QByteArray key = deriveKey(password, salt);
  if (key.isEmpty()) {
    Logger::instance().error(
        QString("PBKDF2 key derivation failed: %1").arg(lastOpenSslError()));
    return Result::kCryptoError;
  }

  QByteArray cipherText;
  QByteArray tag;
  result = encryptBytes(plainText, key, iv, cipherText, tag);
  if (result != Result::kSuccess) {
    return result;
  }

  QByteArray output = buildHeader(salt, iv, tag);
  // Формат итогового файла: [header][cipherText].
  output.append(cipherText);

  return writeAllAtomic(filePath, output);
}

Encoder::Result Encoder::decryptFile(const QString& filePath,
                                     const QString& password) const {
  QByteArray encrypted;
  Result result = readAll(filePath, encrypted);
  if (result != Result::kSuccess) {
    return result;
  }

  if (!hasValidHeader(encrypted)) {
    return Result::kNotEncrypted;
  }

  QByteArray salt;
  QByteArray iv;
  QByteArray tag;
  QByteArray payload;
  result = parseHeader(encrypted, salt, iv, tag, payload);
  if (result != Result::kSuccess) {
    return result;
  }

  const QByteArray key = deriveKey(password, salt);
  if (key.isEmpty()) {
    Logger::instance().error(
        QString("PBKDF2 key derivation failed: %1").arg(lastOpenSslError()));
    return Result::kCryptoError;
  }

  QByteArray plainText;
  result = decryptBytes(payload, key, iv, tag, plainText);
  if (result != Result::kSuccess) {
    return result;
  }

  return writeAllAtomic(filePath, plainText);
}

bool Encoder::isEncryptedFile(const QString& filePath) const {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }

  const QByteArray head = file.read(kHeaderSize);
  return hasValidHeader(head);
}

Encoder::Result Encoder::readAll(const QString& filePath, QByteArray& data) const {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return Result::kOpenError;
  }

  const QByteArray readData = file.readAll();
  if (file.error() != QFile::NoError) {
    return Result::kReadError;
  }

  data = readData;
  return Result::kSuccess;
}

Encoder::Result Encoder::writeAllAtomic(const QString& filePath,
                                        const QByteArray& data) const {
  QSaveFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    return Result::kOpenError;
  }

  const qint64 written = file.write(data);
  if (written != data.size()) {
    file.cancelWriting();
    return Result::kWriteError;
  }

  if (!file.commit()) {
    return Result::kWriteError;
  }

  return Result::kSuccess;
}

bool Encoder::hasValidHeader(const QByteArray& data) const {
  if (data.size() < kHeaderSize) {
    return false;
  }

  if (data.left(kMagicSize) != QByteArray(kMagic.data(), kMagicSize)) {
    return false;
  }

  const quint8 version = static_cast<quint8>(data.at(kMagicSize));
  return version == kVersion;
}

QByteArray Encoder::buildHeader(const QByteArray& salt, const QByteArray& iv,
                                const QByteArray& tag) const {
  QByteArray header;
  header.reserve(kHeaderSize);
  header.append(kMagic.data(), kMagicSize);
  header.append(static_cast<char>(kVersion));
  header.append(salt);
  header.append(iv);
  header.append(tag);
  return header;
}

Encoder::Result Encoder::parseHeader(const QByteArray& data, QByteArray& salt,
                                     QByteArray& iv, QByteArray& tag,
                                     QByteArray& payload) const {
  if (!hasValidHeader(data)) {
    return Result::kInvalidFormat;
  }

  int offset = kMagicSize + 1;
  // Читаем поля строго в том же порядке, в котором они записывались.
  salt = data.mid(offset, kSaltSize);
  offset += kSaltSize;
  iv = data.mid(offset, kIvSize);
  offset += kIvSize;
  tag = data.mid(offset, kTagSize);
  offset += kTagSize;
  payload = data.mid(offset);

  if (salt.size() != kSaltSize || iv.size() != kIvSize || tag.size() != kTagSize) {
    return Result::kInvalidFormat;
  }

  return Result::kSuccess;
}

QByteArray Encoder::deriveKey(const QString& password,
                              const QByteArray& salt) const {
  const QByteArray passwordUtf8 = password.toUtf8();
  QByteArray key(kKeySize, 0);

  // Получаем детерминированный ключ из пароля и случайной соли.
  const int ok = PKCS5_PBKDF2_HMAC(
      passwordUtf8.constData(), passwordUtf8.size(),
      reinterpret_cast<const unsigned char*>(salt.constData()), salt.size(),
      kPbkdf2Iterations, EVP_sha256(), key.size(),
      reinterpret_cast<unsigned char*>(key.data()));

  if (ok != 1) {
    return {};
  }

  return key;
}

Encoder::Result Encoder::encryptBytes(const QByteArray& plainText,
                                      const QByteArray& key,
                                      const QByteArray& iv,
                                      QByteArray& cipherText,
                                      QByteArray& tag) const {
  EvpCipherCtxPtr ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
  if (!ctx) {
    Logger::instance().error(
        QString("EVP_CIPHER_CTX_new failed: %1").arg(lastOpenSslError()));
    return Result::kCryptoError;
  }

  Result result = Result::kSuccess;

  do {
    // 1) Инициализация контекста AES-256-GCM.
    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
      result = Result::kCryptoError;
      break;
    }

    // 2) Установка размера IV перед передачей ключа/IV.
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) != 1) {
      result = Result::kCryptoError;
      break;
    }

    // 3) Передача ключа и IV.
    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
      result = Result::kCryptoError;
      break;
    }

    QByteArray localCipher(plainText.size() + EVP_MAX_BLOCK_LENGTH, 0);
    int produced = 0;
    int total = 0;

    // 4) Шифрование данных.
    if (EVP_EncryptUpdate(ctx.get(),
                          reinterpret_cast<unsigned char*>(localCipher.data()),
                          &produced,
                          reinterpret_cast<const unsigned char*>(
                              plainText.constData()),
                          plainText.size()) != 1) {
      result = Result::kCryptoError;
      break;
    }
    total += produced;

    // 5) Финализация (для GCM обычно без доп.данных, но обязательный вызов API).
    if (EVP_EncryptFinal_ex(
            ctx.get(),
            reinterpret_cast<unsigned char*>(localCipher.data() + total),
            &produced) != 1) {
      result = Result::kCryptoError;
      break;
    }
    total += produced;
    localCipher.resize(total);

    // 6) Получение тега аутентичности, необходимого для дальнейшего decrypt.
    QByteArray localTag(kTagSize, 0);
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, kTagSize,
                            reinterpret_cast<unsigned char*>(localTag.data())) != 1) {
      result = Result::kCryptoError;
      break;
    }

    cipherText = localCipher;
    tag = localTag;
  } while (false);

  if (result != Result::kSuccess) {
    Logger::instance().error(
        QString("AES-GCM encryption failed: %1").arg(lastOpenSslError()));
  }

  return result;
}

Encoder::Result Encoder::decryptBytes(const QByteArray& cipherText,
                                      const QByteArray& key,
                                      const QByteArray& iv,
                                      const QByteArray& tag,
                                      QByteArray& plainText) const {
  EvpCipherCtxPtr ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
  if (!ctx) {
    Logger::instance().error(
        QString("EVP_CIPHER_CTX_new failed: %1").arg(lastOpenSslError()));
    return Result::kCryptoError;
  }

  Result result = Result::kSuccess;

  do {
    // 1) Инициализация контекста AES-256-GCM для дешифрования.
    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
      result = Result::kCryptoError;
      break;
    }

    // 2) Установка длины IV и передача ключа/IV.
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) != 1) {
      result = Result::kCryptoError;
      break;
    }

    if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
      result = Result::kCryptoError;
      break;
    }

    QByteArray localPlain(cipherText.size() + EVP_MAX_BLOCK_LENGTH, 0);
    int produced = 0;
    int total = 0;

    // 3) Предварительное дешифрование шифртекста.
    if (EVP_DecryptUpdate(ctx.get(),
                          reinterpret_cast<unsigned char*>(localPlain.data()),
                          &produced,
                          reinterpret_cast<const unsigned char*>(
                              cipherText.constData()),
                          cipherText.size()) != 1) {
      result = Result::kCryptoError;
      break;
    }
    total += produced;

    // 4) Перед финализацией обязательно передаём ожидаемый GCM-тег.
    QByteArray mutableTag = tag;
    if (EVP_CIPHER_CTX_ctrl(
            ctx.get(), EVP_CTRL_GCM_SET_TAG, mutableTag.size(),
            reinterpret_cast<unsigned char*>(mutableTag.data())) != 1) {
      result = Result::kCryptoError;
      break;
    }

    // 5) Финализация одновременно проверяет целостность/подлинность данных.
    if (EVP_DecryptFinal_ex(
            ctx.get(),
            reinterpret_cast<unsigned char*>(localPlain.data() + total),
            &produced) != 1) {
      result = Result::kCryptoError;
      break;
    }
    total += produced;
    localPlain.resize(total);

    plainText = localPlain;
  } while (false);

  if (result != Result::kSuccess) {
    Logger::instance().error(
        QString("AES-GCM decrypt/auth failed (неверный пароль или файл повреждён): %1")
            .arg(lastOpenSslError()));
  }

  return result;
}

QString Encoder::lastOpenSslError() const {
  const unsigned long errorCode = ERR_get_error();
  if (errorCode == 0UL) {
    return QStringLiteral("unknown OpenSSL error");
  }

  std::array<char, 256> errorBuffer{};
  ERR_error_string_n(errorCode, errorBuffer.data(), errorBuffer.size());
  return QString::fromLatin1(errorBuffer.data());
}
