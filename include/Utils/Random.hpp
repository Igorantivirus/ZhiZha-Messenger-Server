#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace utils
{
namespace
{
template <typename T>
constexpr bool use_short_distribution_v = std::is_same_v<T, bool> || std::is_same_v<T, char> || std::is_same_v<T, unsigned char>;
}

template <
    typename ResType,
    typename DistType = std::conditional_t<
        std::is_floating_point_v<ResType>,
        std::uniform_real_distribution<ResType>,
        std::conditional_t<
            use_short_distribution_v<ResType>,
            std::uniform_int_distribution<short>,
            std::uniform_int_distribution<ResType>>>,
    typename EngineType = std::mt19937_64>
class Random
{
public:
    Random() : rng(dev())
    {
    }
    explicit Random(const unsigned long long seed) : rng(seed)
    {
    }

    ResType generate(const ResType minVal, const ResType maxVal)
    {
        DistType dist(minVal, maxVal);
        return static_cast<ResType>(dist(rng));
    }
    ResType generate()
    {
        return generate(minValue(), maxValue());
    }

    ResType operator()(const ResType minVal, const ResType maxVal)
    {
        return generate(minVal, maxVal);
    }
    ResType operator()()
    {
        return generate();
    }

    template <typename Iter>
    void fill(Iter first, Iter last, const ResType minVal, const ResType maxVal)
    {
        DistType dist(minVal, maxVal);
        for (; first != last; ++first)
            *first = static_cast<ResType>(dist(rng));
    }

    template <typename Iter>
    void fill(Iter first, Iter last)
    {
        fill(first, last, minValue(), maxValue());
    }

    void setSeed(unsigned long long seed)
    {
        rng.seed(seed);
    }
    std::random_device &getDevice()
    {
        return dev;
    }
    EngineType &getEngine()
    {
        return rng;
    }

    constexpr static ResType maxValue()
    {
        return (std::numeric_limits<ResType>::max)();
    }
    constexpr static ResType minValue()
    {
        return (std::numeric_limits<ResType>::min)();
    }

private:
    std::random_device dev;
    EngineType rng;
};

// === Настройки генерации строк ===
// Создаются только через фабрики. Конструктор приватный,
// чтобы пользователь не мог обойти валидацию.
class StringSettings
{
public:
    const std::string &charset() const noexcept
    {
        return charset_;
    }
    bool empty() const noexcept
    {
        return charset_.empty();
    }

    // Разрешающий режим: явно перечисляем, что должно быть в алфавите.
    // specials не должен пересекаться с включёнными категориями и не должен
    // содержать дубликаты — иначе throw.
    static StringSettings allow(bool uppercase = true,
                                bool lowercase = true,
                                bool digits = true,
                                const std::string &specials = "")
    {
        std::string cs;
        if (uppercase)
            cs += UPPERCASE;
        if (lowercase)
            cs += LOWERCASE;
        if (digits)
            cs += DIGITS;
        cs += specials;

        if (cs.empty())
            throw std::invalid_argument("StringSettings::allow: empty charset");

        validateUnique(cs);
        return StringSettings(std::move(cs));
    }

    // Запрещающий режим: берём ASCII 32-126 и вырезаем указанные категории
    // и символы из specials. Дубликаты в specials не страшны (повторное
    // удаление — no-op), валидация не нужна.
    static StringSettings deny(bool uppercase = false,
                               bool lowercase = false,
                               bool digits = false,
                               const std::string &specials = "")
    {
        std::string all;
        all.reserve(95);
        for (char c = 32; c <= 126; ++c)
            all.push_back(c);

        std::string denied;
        if (uppercase)
            denied += UPPERCASE;
        if (lowercase)
            denied += LOWERCASE;
        if (digits)
            denied += DIGITS;
        denied += specials;

        std::string cs;
        cs.reserve(all.size());
        for (char c : all)
            if (denied.find(c) == std::string::npos)
                cs.push_back(c);

        if (cs.empty())
            throw std::invalid_argument("StringSettings::deny: empty charset");

        return StringSettings(std::move(cs));
    }

    // Произвольный набор символов. Валидация не выполняется —
    // ответственность за дубликаты и содержимое на пользователе.
    static StringSettings custom(std::string chars)
    {
        if (chars.empty())
            throw std::invalid_argument("StringSettings::custom: empty charset");
        return StringSettings(std::move(chars));
    }

    // Настройки по умолчанию: алфавитно-цифровые (A-Z, a-z, 0-9).
    static StringSettings defaultSettings()
    {
        return allow(); // upper + lower + digits, без спец-символов
    }

private:
    explicit StringSettings(std::string cs) : charset_(std::move(cs))
    {
    }

    static void validateUnique(const std::string &cs)
    {
        std::string copy = cs;
        std::sort(copy.begin(), copy.end());
        auto it = std::adjacent_find(copy.begin(), copy.end());
        if (it != copy.end())
            throw std::invalid_argument(
                std::string("StringSettings::allow: duplicate character '") + *it +
                "' (specials overlap with categories or contains duplicates)");
    }

    static constexpr const char *UPPERCASE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static constexpr const char *LOWERCASE = "abcdefghijklmnopqrstuvwxyz";
    static constexpr const char *DIGITS = "0123456789";

    std::string charset_;
};

// === Специализация для std::string ===
template <typename DistType, typename EngineType>
class Random<std::string, DistType, EngineType>
{
public:
    Random()
        : rng(dev()), settings_(StringSettings::defaultSettings()), defaultLength_(10)
    {
    }

    explicit Random(unsigned long long seed)
        : rng(seed), settings_(StringSettings::defaultSettings()), defaultLength_(10)
    {
    }

    Random(StringSettings settings, std::size_t defaultLength = 10)
        : rng(dev()), settings_(std::move(settings)), defaultLength_(defaultLength)
    {
    }

    Random(unsigned long long seed, StringSettings settings, std::size_t defaultLength = 10)
        : rng(seed), settings_(std::move(settings)), defaultLength_(defaultLength)
    {
    }

    // --- Генерация строки ---
    std::string generate()
    {
        return generateImpl(defaultLength_, settings_.charset());
    }
    std::string generate(std::size_t length)
    {
        return generateImpl(length, settings_.charset());
    }
    std::string generate(const StringSettings &settings)
    {
        return generateImpl(defaultLength_, settings.charset());
    }
    std::string generate(std::size_t length, const StringSettings &settings)
    {
        return generateImpl(length, settings.charset());
    }

    // --- Генерация одного символа ---
    char generateChar()
    {
        return generateCharImpl(settings_.charset());
    }
    char generateChar(const StringSettings &settings)
    {
        return generateCharImpl(settings.charset());
    }

    // --- operator() ---
    std::string operator()()
    {
        return generate();
    }
    std::string operator()(std::size_t length)
    {
        return generate(length);
    }
    std::string operator()(const StringSettings &settings)
    {
        return generate(settings);
    }
    std::string operator()(std::size_t length, const StringSettings &settings)
    {
        return generate(length, settings);
    }

    // --- Заполнение последовательности строк ---
    template <typename Iter>
    void fill(Iter first, Iter last)
    {
        for (; first != last; ++first)
            *first = generateImpl(defaultLength_, settings_.charset());
    }
    template <typename Iter>
    void fill(Iter first, Iter last, std::size_t length)
    {
        for (; first != last; ++first)
            *first = generateImpl(length, settings_.charset());
    }
    template <typename Iter>
    void fill(Iter first, Iter last, const StringSettings &settings)
    {
        for (; first != last; ++first)
            *first = generateImpl(defaultLength_, settings.charset());
    }
    template <typename Iter>
    void fill(Iter first, Iter last, std::size_t length, const StringSettings &settings)
    {
        for (; first != last; ++first)
            *first = generateImpl(length, settings.charset());
    }

    // --- Управление настройками ---
    const StringSettings &getSettings() const noexcept
    {
        return settings_;
    }
    void setSettings(StringSettings settings)
    {
        settings_ = std::move(settings);
    }

    std::size_t getDefaultLength() const noexcept
    {
        return defaultLength_;
    }
    void setDefaultLength(std::size_t length)
    {
        defaultLength_ = length;
    }

    // --- Управление движком ---
    void setSeed(unsigned long long seed)
    {
        rng.seed(seed);
    }
    std::random_device &getDevice()
    {
        return dev;
    }
    EngineType &getEngine()
    {
        return rng;
    }

private:
    std::string generateImpl(std::size_t length, const std::string &charset)
    {
        if (charset.empty())
            throw std::invalid_argument("Random<std::string>::generate: empty charset");

        std::string result;
        result.reserve(length);
        DistType dist(0, charset.size() - 1);
        for (std::size_t i = 0; i < length; ++i)
            result.push_back(charset[dist(rng)]);
        return result;
    }

    char generateCharImpl(const std::string &charset)
    {
        if (charset.empty())
            throw std::invalid_argument("Random<std::string>::generateChar: empty charset");
        DistType dist(0, charset.size() - 1);
        return charset[dist(rng)];
    }

    std::random_device dev;
    EngineType rng;
    StringSettings settings_;
    std::size_t defaultLength_;
};

using RandomString = Random<std::string, std::uniform_int_distribution<std::size_t>, std::mt19937_64>;

} // namespace utils