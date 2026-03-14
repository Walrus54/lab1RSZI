#ifndef LOGGER_H
#define LOGGER_H

#include <QFile>
#include <QMutex>
#include <QString>
#include <QTextStream>

/**
 * @brief Singleton-логер для консольного приложения.
 *
 * Логер выводит сообщения в консоль и (при успешной инициализации)
 * дублирует их в файл журнала.
 */
class Logger final {
 public:
  /**
   * @brief Уровень сообщения журнала.
   */
  enum class Level {
    kInfo,    /**< Информационное сообщение. */
    kWarning, /**< Предупреждение. */
    kError    /**< Ошибка. */
  };

  /**
   * @brief Получить единственный экземпляр логера.
   * @return Ссылка на singleton-экземпляр.
   */
  static Logger& instance();

  /**
   * @brief Инициализировать логирование в файл.
   * @param[in] logFilePath Путь к файлу журнала. Если пустой,
   *                          используется `./encryptor.log`.
   * @throws Не выбрасывает исключений (ошибки фиксируются в stderr).
   */
  void initialize(const QString& logFilePath);

  /**
   * @brief Записать информационное сообщение.
   * @param[in] message Текст сообщения.
   * @throws Не выбрасывает исключений.
   */
  void info(const QString& message);

  /**
   * @brief Записать предупреждение.
   * @param[in] message Текст сообщения.
   * @throws Не выбрасывает исключений.
   */
  void warning(const QString& message);

  /**
   * @brief Записать сообщение об ошибке.
   * @param[in] message Текст сообщения.
   * @throws Не выбрасывает исключений.
   */
  void error(const QString& message);

 private:
  /**
   * @brief Конструктор singleton-объекта.
   */
  Logger();

  /**
   * @brief Деструктор singleton-объекта.
   */
  ~Logger();

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  /**
   * @brief Записать сообщение указанного уровня.
   * @param[in] level Уровень сообщения.
   * @param[in] message Текст сообщения.
   * @throws Не выбрасывает исключений.
   */
  void log(Level level, const QString& message);

  /**
   * @brief Преобразовать уровень в строку.
   * @param[in] level Уровень сообщения.
   * @return Строковое имя уровня.
   */
  QString levelToString(Level level) const;

  /** @brief Мьютекс для потокобезопасной записи. */
  mutable QMutex mutex_;
  /** @brief Файл журнала. */
  QFile logFile_;
  /** @brief Поток для записи в файл журнала. */
  QTextStream logStream_;
  /** @brief Флаг успешной инициализации файла журнала. */
  bool isInitialized_;
};

#endif  // LOGGER_H
