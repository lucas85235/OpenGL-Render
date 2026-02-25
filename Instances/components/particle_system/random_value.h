#include <random>

namespace ix::samsung::homecomponents {

    template <typename T>
    class RandomValue {
    private:
        T minValue, maxValue;
        bool isRange;

    public:
        // Default constructor
        RandomValue() {}
        // Constructor for a fixed value
        RandomValue(T value) : minValue(value), maxValue(value), isRange(false) {}
        // Constructor for two values ​​(range)
        RandomValue(T minValue, T maxValue) : minValue(minValue), maxValue(maxValue), isRange(true) {}

        // Function to generate value
        T getValue() {
            if (isRange) {
                return getRandomInRange(minValue, maxValue);
            } else return minValue;
        }

    private:
        // Random number generator for numeric types (int, float)
        template <typename U = T>
        typename std::enable_if<std::is_arithmetic<U>::value, U>::type
        getRandomInRange(U minValue, U maxValue) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            if constexpr (std::is_integral<U>::value) {
                std::uniform_int_distribution<U> dis(minValue, maxValue);
                return dis(gen);
            } else {
                std::uniform_real_distribution<U> dis(minValue, maxValue);
                return dis(gen);
            }
        }

        // Random number generator for vector types (float2, float3, float4)
        template <typename U = T>
        typename std::enable_if<!std::is_arithmetic<U>::value, U>::type
        getRandomInRange(U minValue, U maxValue) {
            U result;
            result.x = getRandomInRange(minValue.x, maxValue.x);
            result.y = getRandomInRange(minValue.y, maxValue.y);
            if constexpr (std::is_same<U, imp::float3>::value || std::is_same<U, imp::float4>::value) {
                result.z = getRandomInRange(minValue.z, maxValue.z);
            }
            if constexpr (std::is_same<U, imp::float4>::value) {
                result.w = getRandomInRange(minValue.w, maxValue.w);
            }
            return result;
        }
    };
}