#ifndef RECURSIVE_PROCESSOR_H
#define RECURSIVE_PROCESSOR_H

#include <QString>
#include <QStringList>

/**
 * @brief Рекурсивный обходчик каталога (обход через QDirIterator).
 *
 * Класс отвечает только за поиск и фильтрацию валидных файлов.
 * Шифрование/дешифрование выполняется отдельно, вне этого модуля.
 */
class RecursiveProcessor final {
 public:
  /**
   * @brief Создать обработчик.
   * @param[in] excludedFilePath Абсолютный/относительный путь к файлу,
   *                               который необходимо исключить из обработки
   *                               (например, активный лог-файл).
   * @param[in] skipSystemFiles Признак пропуска системных файлов.
   */
  RecursiveProcessor(QString excludedFilePath, bool skipSystemFiles);

  /**
   * @brief Собрать список валидных файлов из директории рекурсивно.
   * @param[in] rootDirectory Корневой каталог.
   * @return Список путей к файлам, прошедшим фильтры.
   * @throws Не выбрасывает исключений; ошибки фиксируются в журнале.
   */
  QStringList collectValidFilePaths(const QString& rootDirectory) const;

 private:
  /**
   * @brief Проверить, является ли файл ярлыком.
   * @param[in] filePath Путь к файлу.
   * @return `true`, если файл должен быть пропущен как ярлык.
   * @throws Не выбрасывает исключений.
   */
  bool isShortcutFile(const QString& filePath) const;

  /**
   * @brief Проверить, является ли файл системным.
   * @param[in] filePath Путь к файлу.
   * @return `true`, если файл должен быть пропущен при шифровании.
   * @throws Не выбрасывает исключений.
   */
  bool isSystemFile(const QString& filePath) const;

  /** @brief Нужно ли отбрасывать системные файлы во время обхода. */
  bool skipSystemFiles_;
  /** @brief Нормализованный путь к исключённому служебному файлу. */
  QString excludedFilePath_;
};

#endif  // RECURSIVE_PROCESSOR_H
