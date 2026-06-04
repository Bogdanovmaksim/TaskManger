#include <stdexcept>
#include <string>
#include <vector>

#include "doctest.h"
#include "task.h"

using namespace task_manager;

namespace {

Task makeTask(int id, const std::string& description, const std::string& deadline,
              const std::string& category, Importance importance = Importance::Medium,
              Status status = Status::Active) {
  return Task{id, description, parseDate(deadline), category, importance, status};
}

void requireTaskEquals(const Task& task, int id, const std::string& description,
                       const std::string& deadline, const std::string& category,
                       Importance importance, Status status) {
  REQUIRE(task.id == id);
  REQUIRE(task.description == description);
  REQUIRE(dateToString(task.deadline) == deadline);
  REQUIRE(task.category == category);
  REQUIRE(task.importance == importance);
  REQUIRE(task.status == status);
}

}  // namespace

/*!
\brief Проверка основного формата даты.
\details Проверяется, что строка формата ДД.ММ.ГГГГ
правильно разбирается на год, месяц и день.
*/
TEST_CASE("parseDate accepts dotted user format") {
  const Date date = parseDate("05.06.2026");

  REQUIRE(date.year == 2026);
  REQUIRE(date.month == 6);
  REQUIRE(date.day == 5);
}

/*!
\brief Проверка дополнительных форматов даты.
\details Формат с пробелами нужен для удобного ввода,
а старый формат YYYY-MM-DD нужен для совместимости
с ранее сохраненными задачами.
*/
TEST_CASE("parseDate accepts space format and old file format") {
  REQUIRE(dateToString(parseDate("15 06 2026")) == "15.06.2026");
  REQUIRE(dateToString(parseDate("2026-06-15")) == "15.06.2026");
}

/*!
\brief Проверка високосных годов.
\details Функция parseDate косвенно проверяет логику
високосного года: 29 февраля должно существовать только
в корректные високосные годы.
*/
TEST_CASE("parseDate validates leap years") {
  REQUIRE(dateToString(parseDate("29.02.2024")) == "29.02.2024");
  REQUIRE(dateToString(parseDate("29.02.2000")) == "29.02.2000");

  REQUIRE_THROWS_AS(parseDate("29.02.2023"), std::invalid_argument);
  REQUIRE_THROWS_AS(parseDate("29.02.1900"), std::invalid_argument);
}

/*!
\brief Проверка количества дней в месяцах.
\details Проверяются реальные календарные границы:
месяцы с 31 днем, месяц с 30 днями и февраль в
обычном будущем году.
*/
TEST_CASE("parseDate validates month day limits") {
  REQUIRE(dateToString(parseDate("31.01.2001")) == "31.01.2001");
  REQUIRE(dateToString(parseDate("30.04.2027")) == "30.04.2027");
  REQUIRE(dateToString(parseDate("31.12.2029")) == "31.12.2029");

  REQUIRE_THROWS_AS(parseDate("31.04.2027"), std::invalid_argument);
  REQUIRE_THROWS_AS(parseDate("31.11.2029"), std::invalid_argument);
  REQUIRE_THROWS_AS(parseDate("29.02.2027"), std::invalid_argument);
}

/*!
\brief Проверка отклонения неверного формата даты.
\details Дата должна содержать корректные разделители,
полный год и только цифры в числовых частях.
*/
TEST_CASE("parseDate rejects wrong date format") {
  REQUIRE_THROWS_AS(parseDate("2026/06/05"), std::invalid_argument);
  REQUIRE_THROWS_AS(parseDate("5.06.2026"), std::invalid_argument);
  REQUIRE_THROWS_AS(parseDate("05.6.2026"), std::invalid_argument);
  REQUIRE_THROWS_AS(parseDate("05.06.26"), std::invalid_argument);
  REQUIRE_THROWS_AS(parseDate("aa.06.2026"), std::invalid_argument);
}

/*!
\brief Проверка форматирования даты.
\details Проверяется, что дата преобразуется в строку
формата ДД.ММ.ГГГГ с ведущими нулями.
*/
TEST_CASE("dateToString formats date as dd mm yyyy") {
  REQUIRE(dateToString(Date{2026, 1, 5}) == "05.01.2026");
  REQUIRE(dateToString(Date{2029, 12, 31}) == "31.12.2029");
}

/*!
\brief Проверка защиты форматирования от некорректных дат.
\details Некорректная дата не должна превращаться
в строку, потому что это привело бы к сохранению
ошибочных данных.
*/
TEST_CASE("dateToString rejects invalid date") {
  REQUIRE_THROWS_AS(dateToString(Date{2026, 13, 1}), std::invalid_argument);
  REQUIRE_THROWS_AS(dateToString(Date{2026, 2, 30}), std::invalid_argument);
}

/*!
\brief Проверка сравнения дат.
\details Проверяется, что функция правильно определяет
раннюю дату, равные даты и более позднюю дату.
*/
TEST_CASE("compareDates returns correct ordering") {
  REQUIRE(compareDates(parseDate("05.06.2026"), parseDate("10.06.2026")) < 0);
  REQUIRE(compareDates(parseDate("10.06.2026"), parseDate("10.06.2026")) == 0);
  REQUIRE(compareDates(parseDate("20.06.2026"), parseDate("10.06.2026")) > 0);
}

/*!
\brief Проверка обработки ошибок при сравнении дат.
\details Сравнение не должно работать с невозможными
календарными значениями.
*/
TEST_CASE("compareDates rejects invalid dates") {
  REQUIRE_THROWS_AS(compareDates(Date{2026, 0, 1}, parseDate("10.06.2026")), std::invalid_argument);
  REQUIRE_THROWS_AS(compareDates(parseDate("10.06.2026"), Date{2026, 6, 31}),
                    std::invalid_argument);
}

/*!
\brief Проверка подсчета расстояния между датами.
\details Функция используется для фильтрации задач
на ближайшие дни, поэтому проверяются обычный случай,
обратный порядок и високосный февраль.
*/
TEST_CASE("daysBetween counts date distance") {
  REQUIRE(daysBetween(parseDate("01.06.2026"), parseDate("10.06.2026")) == 9);
  REQUIRE(daysBetween(parseDate("10.06.2026"), parseDate("01.06.2026")) == -9);
  REQUIRE(daysBetween(parseDate("28.02.2024"), parseDate("01.03.2024")) == 2);
}

/*!
\brief Проверка преобразования важности.
\details Важность должна корректно сохраняться в файл
и восстанавливаться из пользовательского ввода.
*/
TEST_CASE("importance conversion supports file and user values") {
  REQUIRE(importanceToString(Importance::Low) == "low");
  REQUIRE(importanceToString(Importance::Medium) == "medium");
  REQUIRE(importanceToString(Importance::High) == "high");

  REQUIRE(importanceFromString("низкая") == Importance::Low);
  REQUIRE(importanceFromString("с") == Importance::Medium);
  REQUIRE(importanceFromString("HIGH") == Importance::High);
}

/*!
\brief Проверка ошибок преобразования важности.
\details Неизвестная важность не должна приниматься,
иначе задача получила бы неопределенный приоритет.
*/
TEST_CASE("importance conversion rejects unknown value") {
  REQUIRE_THROWS_AS(importanceFromString("срочно"), std::invalid_argument);
  REQUIRE_THROWS_AS(importanceToString(static_cast<Importance>(100)), std::invalid_argument);
}

/*!
\brief Проверка преобразования статуса.
\details Статус должен корректно сохраняться в файл
и восстанавливаться из пользовательского ввода.
*/
TEST_CASE("status conversion supports file and user values") {
  REQUIRE(statusToString(Status::Active) == "active");
  REQUIRE(statusToString(Status::Done) == "done");

  REQUIRE(statusFromString("активна") == Status::Active);
  REQUIRE(statusFromString("г") == Status::Done);
  REQUIRE(statusFromString("DONE") == Status::Done);
}

/*!
\brief Проверка ошибок преобразования статуса.
\details Неизвестный статус должен приводить
к исключению, потому что допустимы только активные
и выполненные задачи.
*/
TEST_CASE("status conversion rejects unknown value") {
  REQUIRE_THROWS_AS(statusFromString("ожидание"), std::invalid_argument);
  REQUIRE_THROWS_AS(statusToString(static_cast<Status>(100)), std::invalid_argument);
}

/*!
\brief Проверка сериализации обычной задачи.
\details Задача должна превращаться в строку файла
с шестью полями, разделенными символом '|'.
*/
TEST_CASE("serializeTask creates file line") {
  const Task task = makeTask(1, "Сделать проект", "15.06.2026", "учеба", Importance::High);

  REQUIRE(serializeTask(task) == "1|Сделать проект|15.06.2026|учеба|high|active");
}

/*!
\brief Проверка экранирования при сериализации.
\details Символы '|', обратный слеш и переносы строк
должны сохраняться безопасно, чтобы не ломать формат файла.
*/
TEST_CASE("serializeTask escapes service characters") {
  const Task task{7,
                  "Текст | с разделителем\\и переносом\nстроки",
                  parseDate("15.06.2026"),
                  "учеба\rcpp",
                  Importance::High,
                  Status::Done};

  REQUIRE(serializeTask(task) ==
          "7|Текст \\| с разделителем\\\\и переносом\\nстроки|15.06.2026|учеба\\rcpp|high|done");
}

/*!
\brief Проверка валидации задачи при сериализации.
\details Задача без корректного id, описания, категории
или дедлайна не должна попадать в файл.
*/
TEST_CASE("serializeTask rejects invalid task") {
  REQUIRE_THROWS_AS(serializeTask(Task{0, "Bad", parseDate("01.06.2026"), "study", Importance::Low,
                                       Status::Active}),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(
      serializeTask(Task{1, "", parseDate("01.06.2026"), "study", Importance::Low, Status::Active}),
      std::invalid_argument);
  REQUIRE_THROWS_AS(serializeTask(Task{1, "Bad", parseDate("01.06.2026"), "   ", Importance::Low,
                                       Status::Active}),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(
      serializeTask(Task{1, "Bad", Date{2026, 2, 30}, "study", Importance::Low, Status::Active}),
      std::invalid_argument);
}

/*!
\brief Проверка восстановления задачи из строки.
\details Строка из файла должна полностью восстанавливать
id, описание, дедлайн, категорию, важность и статус.
*/
TEST_CASE("deserializeTask restores task from file line") {
  const Task task = deserializeTask("2|Доклад|05.06.2026|учеба|medium|active");

  requireTaskEquals(task, 2, "Доклад", "05.06.2026", "учеба", Importance::Medium, Status::Active);
}

/*!
\brief Проверка полного цикла сохранения и чтения.
\details Задача со служебными символами после сериализации
и десериализации должна сохранять исходные поля.
*/
TEST_CASE("deserializeTask restores escaped task after serialization") {
  const Task original{7,
                      "Текст | с разделителем\\и переносом\nстроки",
                      parseDate("15.06.2026"),
                      "учеба\rcpp",
                      Importance::High,
                      Status::Done};

  const Task restored = deserializeTask(serializeTask(original));

  requireTaskEquals(restored, original.id, original.description, "15.06.2026", original.category,
                    Importance::High, Status::Done);
}

/*!
\brief Проверка чтения старого формата даты.
\details Старые сохраненные строки с датой YYYY-MM-DD
должны продолжать открываться после перехода на формат
ДД.ММ.ГГГГ.
*/
TEST_CASE("deserializeTask supports old saved date format") {
  const Task task = deserializeTask("3|Старый формат|2026-06-05|архив|low|done");

  requireTaskEquals(task, 3, "Старый формат", "05.06.2026", "архив", Importance::Low, Status::Done);
}

/*!
\brief Проверка поврежденных строк файла.
\details Строки с неправильным количеством полей,
битым id, неправильной датой или неизвестными значениями
должны отклоняться.
*/
TEST_CASE("deserializeTask rejects broken file lines") {
  REQUIRE_THROWS_AS(deserializeTask("1|too|few"), std::invalid_argument);
  REQUIRE_THROWS_AS(deserializeTask("abc|Text|01.06.2026|study|low|active"), std::invalid_argument);
  REQUIRE_THROWS_AS(deserializeTask("1abc|Text|01.06.2026|study|low|active"),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(deserializeTask("1|Text|30.02.2026|study|low|active"), std::invalid_argument);
  REQUIRE_THROWS_AS(deserializeTask("1||01.06.2026|study|low|active"), std::invalid_argument);
  REQUIRE_THROWS_AS(deserializeTask("1|Text|01.06.2026||low|active"), std::invalid_argument);
  REQUIRE_THROWS_AS(deserializeTask("1|Text|01.06.2026|study|urgent|active"),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(deserializeTask("1|Text\\q|01.06.2026|study|low|active"),
                    std::invalid_argument);
}

/*!
\brief Проверка фильтрации задачи.
\details Проверяется, что ключевое слово, важность,
статус и диапазон ближайших дней применяются вместе.
*/
TEST_CASE("taskMatchesFilter applies combined filter") {
  const Task task = makeTask(3, "Write C++ Project", "05.06.2026", "study", Importance::High);

  TaskFilter filter;
  filter.keyword = "project";
  filter.importance = Importance::High;
  filter.status = Status::Active;
  filter.baseDate = parseDate("01.06.2026");
  filter.daysFromBaseDate = 7;

  REQUIRE(taskMatchesFilter(task, filter));

  filter.status = Status::Done;
  REQUIRE_FALSE(taskMatchesFilter(task, filter));
}

/*!
\brief Проверка ошибок фильтрации.
\details Некорректная базовая дата фильтра и некорректная
задача должны приводить к исключению.
*/
TEST_CASE("taskMatchesFilter rejects invalid filter or task") {
  const Task task = makeTask(3, "Задача", "05.06.2026", "учеба", Importance::High);

  TaskFilter filter;
  filter.baseDate = Date{2026, 2, 30};
  filter.daysFromBaseDate = 5;

  REQUIRE_THROWS_AS(taskMatchesFilter(task, filter), std::invalid_argument);
  REQUIRE_THROWS_AS(taskMatchesFilter(Task{0, "Bad", parseDate("05.06.2026"), "учеба",
                                           Importance::High, Status::Active},
                                      TaskFilter{}),
                    std::invalid_argument);
}

/*!
\brief Проверка сортировки задач по дедлайну.
\details Задачи должны упорядочиваться от ближайшего
дедлайна к более позднему.
*/
TEST_CASE("sortByDeadline sorts tasks by nearest deadline") {
  std::vector<Task> tasks{
      makeTask(3, "Позже", "20.06.2026", "учеба", Importance::Low),
      makeTask(2, "Середина", "15.06.2026", "учеба", Importance::High),
      makeTask(1, "Раньше", "10.06.2026", "учеба", Importance::Medium),
  };

  sortByDeadline(tasks);

  REQUIRE(tasks.at(0).id == 1);
  REQUIRE(tasks.at(1).id == 2);
  REQUIRE(tasks.at(2).id == 3);
}

/*!
\brief Проверка сортировки задач с одинаковым дедлайном.
\details Если дедлайн совпадает, дополнительным критерием
становится id задачи, чтобы порядок был предсказуемым.
*/
TEST_CASE("sortByDeadline sorts same deadline by id") {
  std::vector<Task> tasks{
      makeTask(5, "Пятое", "10.06.2026", "учеба"),
      makeTask(2, "Второе", "10.06.2026", "учеба"),
      makeTask(3, "Третье", "10.06.2026", "учеба"),
  };

  sortByDeadline(tasks);

  REQUIRE(tasks.at(0).id == 2);
  REQUIRE(tasks.at(1).id == 3);
  REQUIRE(tasks.at(2).id == 5);
}

/*!
\brief Проверка ошибки при сортировке.
\details Если в списке есть задача с невозможной датой,
сортировка должна выбросить исключение.
*/
TEST_CASE("sortByDeadline rejects task with invalid deadline") {
  std::vector<Task> tasks{
      Task{2, "Ошибка", Date{2026, 2, 30}, "учеба", Importance::Low, Status::Active},
      makeTask(1, "Нормальная", "10.06.2026", "учеба"),
  };

  REQUIRE_THROWS_AS(sortByDeadline(tasks), std::invalid_argument);
}
