#ifndef ENCODER_H
#define ENCODER_H

#include <QByteArray>
#include <QString>

/**
 * @brief Singleton-модуль шифрования/дешифрования файлов (AES-256-GCM).
 *
 * Формат зашифрованного файла:
 * - magic (8 байт)
 * - version (1 байт)
 * - salt (16 байт)
 * - iv (12 байт)
 * - tag (16 байт)
 * - ciphertext (N байт)
 */
class Encoder final {
 public:
  /**
   * @brief Код результата операции над файлом.
   */
  enum class Result {
    kSuccess,          /**< Операция завершена успешно. */
    kAlreadyEncrypted, /**< Файл уже зашифрован. */
    kNotEncrypted,     /**< Файл не содержит заголовок шифратора. */
    kEmptyFile,        /**< Файл пустой. */
    kOpenError,        /**< Ошибка открытия файла. */
    kReadError,        /**< Ошибка чтения файла. */
    kWriteError,       /**< Ошибка записи файла. */
    kCryptoError,      /**< Криптографическая ошибка OpenSSL. */
    kInvalidFormat     /**< Неверный формат заголовка файла. */
  };

  /**
   * @brief Получить singleton-экземпляр шифратора.
   * @return Ссылка на Encoder.
   */
  static Encoder& instance();

  /**
   * @brief Зашифровать файл по пути.
   * @param[in] filePath Абсолютный/относительный путь к файлу.
   * @param[in] password Пароль для получения ключа.
   * @return Код результата операции.
   * @throws Не выбрасывает исключений; ошибки возвращаются через Result.
   */
  Result encryptFile(const QString& filePath, const QString& password) const;

  /**
   * @brief Дешифровать файл по пути.
   * @param[in] filePath Абсолютный/относительный путь к файлу.
   * @param[in] password Пароль для получения ключа.
   * @return Код результата операции.
   * @throws Не выбрасывает исключений; ошибки возвращаются через Result.
   */
  Result decryptFile(const QString& filePath, const QString& password) const;

  /**
   * @brief Проверить наличие сигнатуры зашифрованного файла.
   * @param[in] filePath Путь к файлу.
   * @return `true`, если файл начинается с корректного заголовка.
   * @throws Не выбрасывает исключений.
   */
  bool isEncryptedFile(const QString& filePath) const;

 private:
  Encoder() = default;
  ~Encoder() = default;

  Encoder(const Encoder&) = delete;
  Encoder& operator=(const Encoder&) = delete;

  /**
   * @brief Прочитать файл целиком.
   * @param[in] filePath Путь к файлу.
   * @param[out] data Прочитанные данные.
   * @return Код результата операции.
   */
  Result readAll(const QString& filePath, QByteArray& data) const;

  /**
   * @brief Записать данные в файл атомарно.
   * @param[in] filePath Путь целевого файла.
   * @param[in] data Буфер для записи.
   * @return Код результата операции.
   */
  Result writeAllAtomic(const QString& filePath, const QByteArray& data) const;

  /**
   * @brief Проверить, содержит ли буфер корректный заголовок.
   * @param[in] data Буфер данных файла.
   * @return `true`, если заголовок корректен.
   */
  bool hasValidHeader(const QByteArray& data) const;

  /**
   * @brief Сформировать бинарный заголовок зашифрованного файла.
   * @param[in] salt Соль PBKDF2.
   * @param[in] iv Вектор инициализации GCM.
   * @param[in] tag Тег аутентичности GCM.
   * @return Бинарный заголовок.
   */
  QByteArray buildHeader(const QByteArray& salt, const QByteArray& iv,
                         const QByteArray& tag) const;

  /**
   * @brief Распарсить заголовок зашифрованного файла.
   * @param[in] data Буфер файла.
   * @param[out] salt Прочитанная соль.
   * @param[out] iv Прочитанный IV.
   * @param[out] tag Прочитанный тег.
   * @param[out] payload Зашифрованные данные без заголовка.
   * @return Код результата операции.
   */
  Result parseHeader(const QByteArray& data, QByteArray& salt, QByteArray& iv,
                     QByteArray& tag, QByteArray& payload) const;

  /**
   * @brief Получить ключ из пароля через PBKDF2-HMAC-SHA256.
   * @param[in] password Пароль пользователя.
   * @param[in] salt Случайная соль.
   * @return Ключ фиксированного размера или пустой буфер при ошибке.
   */
  QByteArray deriveKey(const QString& password, const QByteArray& salt) const;

  /**
   * @brief Выполнить шифрование буфера AES-256-GCM.
   * @param[in] plainText Открытый текст.
   * @param[in] key Ключ шифрования.
   * @param[in] iv IV (nonce).
   * @param[out] cipherText Выходной шифртекст.
   * @param[out] tag Выходной тег аутентичности.
   * @return Код результата операции.
   */
  Result encryptBytes(const QByteArray& plainText, const QByteArray& key,
                      const QByteArray& iv, QByteArray& cipherText,
                      QByteArray& tag) const;

  /**
   * @brief Выполнить дешифрование буфера AES-256-GCM.
   * @param[in] cipherText Шифртекст.
   * @param[in] key Ключ шифрования.
   * @param[in] iv IV (nonce).
   * @param[in] tag Тег аутентичности.
   * @param[out] plainText Расшифрованный текст.
   * @return Код результата операции.
   */
  Result decryptBytes(const QByteArray& cipherText, const QByteArray& key,
                      const QByteArray& iv, const QByteArray& tag,
                      QByteArray& plainText) const;

  /**
   * @brief Получить последнее текстовое описание ошибки OpenSSL.
   * @return Описание ошибки.
   */
  QString lastOpenSslError() const;
};

#endif  // ENCODER_H
