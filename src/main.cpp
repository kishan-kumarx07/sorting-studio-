#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif
using SocketHandle = SOCKET;
constexpr SocketHandle invalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketHandle = int;
constexpr SocketHandle invalidSocket = -1;
#endif

namespace fs = std::filesystem;

struct TraceStep {
    std::string action;
    std::vector<int> indices;
    std::vector<int> array;
};

struct SortMetrics {
    long long comparisons = 0;
    long long swaps = 0;
};

struct SortResult {
    std::string algorithm;
    std::vector<int> input;
    std::vector<int> output;
    SortMetrics metrics;
    double milliseconds = 0.0;
    bool stable = false;
    std::vector<TraceStep> steps;
};

struct SortTracer {
    SortMetrics metrics;
    std::vector<TraceStep> steps;

    void snapshot(const std::vector<int>& current, std::string action, std::vector<int> indices = {}) {
        steps.push_back(TraceStep{std::move(action), std::move(indices), current});
    }
};

std::string trim(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string toLower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string escapeJson(const std::string& value) {
    std::ostringstream out;
    for (char ch : value) {
        switch (ch) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    out << "\\u";
                    out << std::hex << std::uppercase;
                    out.width(4);
                    out.fill('0');
                    out << static_cast<int>(static_cast<unsigned char>(ch));
                    out << std::dec << std::nouppercase;
                } else {
                    out << ch;
                }
        }
    }
    return out.str();
}

std::string jsonString(const std::string& value) {
    return '"' + escapeJson(value) + '"';
}

std::string jsonArray(const std::vector<int>& values) {
    std::ostringstream out;
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << values[i];
    }
    out << ']';
    return out.str();
}

std::string jsonArray(const std::vector<std::string>& values) {
    std::ostringstream out;
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << jsonString(values[i]);
    }
    out << ']';
    return out.str();
}

std::string jsonArray(const std::vector<TraceStep>& steps) {
    std::ostringstream out;
    out << '[';
    for (std::size_t i = 0; i < steps.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << '{';
        out << "\"action\":" << jsonString(steps[i].action) << ',';
        out << "\"indices\":" << jsonArray(steps[i].indices) << ',';
        out << "\"array\":" << jsonArray(steps[i].array);
        out << '}';
    }
    out << ']';
    return out.str();
}

std::optional<std::string> extractJsonString(const std::string& json, const std::string& key) {
    const std::string token = '"' + key + '"';
    const std::size_t keyPos = json.find(token);
    if (keyPos == std::string::npos) {
        return std::nullopt;
    }
    const std::size_t colonPos = json.find(':', keyPos + token.size());
    if (colonPos == std::string::npos) {
        return std::nullopt;
    }
    const std::size_t firstQuote = json.find('"', colonPos + 1);
    if (firstQuote == std::string::npos) {
        return std::nullopt;
    }
    const std::size_t secondQuote = json.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) {
        return std::nullopt;
    }
    return json.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

std::optional<std::vector<int>> extractJsonIntArray(const std::string& json, const std::string& key) {
    const std::string token = '"' + key + '"';
    const std::size_t keyPos = json.find(token);
    if (keyPos == std::string::npos) {
        return std::nullopt;
    }
    const std::size_t colonPos = json.find(':', keyPos + token.size());
    if (colonPos == std::string::npos) {
        return std::nullopt;
    }
    const std::size_t openBracket = json.find('[', colonPos + 1);
    if (openBracket == std::string::npos) {
        return std::nullopt;
    }
    const std::size_t closeBracket = json.find(']', openBracket + 1);
    if (closeBracket == std::string::npos) {
        return std::nullopt;
    }

    std::vector<int> values;
    std::string number;
    for (std::size_t i = openBracket + 1; i < closeBracket; ++i) {
        const char ch = json[i];
        if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '-' || ch == '+') {
            number.push_back(ch);
        } else if (!number.empty()) {
            values.push_back(std::stoi(number));
            number.clear();
        }
    }
    if (!number.empty()) {
        values.push_back(std::stoi(number));
    }
    return values;
}

std::string joinIntsAsText(const std::vector<int>& values) {
    std::ostringstream out;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << values[i];
    }
    return out.str();
}

std::vector<int> unshiftValues(const std::vector<int>& values, int offset) {
    std::vector<int> display(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        display[i] = values[i] - offset;
    }
    return display;
}

void mergeSortTrace(std::vector<int>& values, int left, int right, SortTracer& tracer) {
    if (left >= right) {
        return;
    }
    const int mid = left + (right - left) / 2;
    mergeSortTrace(values, left, mid, tracer);
    mergeSortTrace(values, mid + 1, right, tracer);

    std::vector<int> merged;
    merged.reserve(static_cast<std::size_t>(right - left + 1));
    int i = left;
    int j = mid + 1;
    while (i <= mid && j <= right) {
        ++tracer.metrics.comparisons;
        tracer.snapshot(values, "compare", {i, j});
        if (values[i] <= values[j]) {
            merged.push_back(values[i]);
            ++i;
        } else {
            merged.push_back(values[j]);
            ++j;
        }
    }
    while (i <= mid) {
        merged.push_back(values[i]);
        ++i;
    }
    while (j <= right) {
        merged.push_back(values[j]);
        ++j;
    }
    for (int index = 0; index < static_cast<int>(merged.size()); ++index) {
        values[left + index] = merged[static_cast<std::size_t>(index)];
        ++tracer.metrics.swaps;
        tracer.snapshot(values, "write", {left + index});
    }
}

int partitionTrace(std::vector<int>& values, int low, int high, SortTracer& tracer) {
    const int pivot = values[high];
    int i = low - 1;
    for (int j = low; j < high; ++j) {
        ++tracer.metrics.comparisons;
        tracer.snapshot(values, "compare", {j, high});
        if (values[j] <= pivot) {
            ++i;
            if (i != j) {
                std::swap(values[static_cast<std::size_t>(i)], values[static_cast<std::size_t>(j)]);
                ++tracer.metrics.swaps;
                tracer.snapshot(values, "swap", {i, j});
            } else {
                tracer.snapshot(values, "keep", {i, j});
            }
        }
    }
    if (i + 1 != high) {
        std::swap(values[static_cast<std::size_t>(i + 1)], values[static_cast<std::size_t>(high)]);
        ++tracer.metrics.swaps;
        tracer.snapshot(values, "swap", {i + 1, high});
    }
    return i + 1;
}

void quickSortTrace(std::vector<int>& values, int low, int high, SortTracer& tracer) {
    if (low < high) {
        const int pivotIndex = partitionTrace(values, low, high, tracer);
        quickSortTrace(values, low, pivotIndex - 1, tracer);
        quickSortTrace(values, pivotIndex + 1, high, tracer);
    }
}

void heapifyTrace(std::vector<int>& values, int heapSize, int rootIndex, SortTracer& tracer) {
    int largest = rootIndex;
    while (true) {
        const int leftChild = 2 * largest + 1;
        const int rightChild = 2 * largest + 2;
        int nextLargest = largest;

        if (leftChild < heapSize) {
            ++tracer.metrics.comparisons;
            tracer.snapshot(values, "compare", {nextLargest, leftChild});
            if (values[leftChild] > values[nextLargest]) {
                nextLargest = leftChild;
            }
        }
        if (rightChild < heapSize) {
            ++tracer.metrics.comparisons;
            tracer.snapshot(values, "compare", {nextLargest, rightChild});
            if (values[rightChild] > values[nextLargest]) {
                nextLargest = rightChild;
            }
        }
        if (nextLargest == largest) {
            break;
        }
        std::swap(values[static_cast<std::size_t>(largest)], values[static_cast<std::size_t>(nextLargest)]);
        ++tracer.metrics.swaps;
        tracer.snapshot(values, "swap", {largest, nextLargest});
        largest = nextLargest;
    }
}

void heapSortTrace(std::vector<int>& values, SortTracer& tracer) {
    const int heapSize = static_cast<int>(values.size());
    for (int i = heapSize / 2 - 1; i >= 0; --i) {
        heapifyTrace(values, heapSize, i, tracer);
    }
    for (int end = heapSize - 1; end > 0; --end) {
        std::swap(values[0], values[static_cast<std::size_t>(end)]);
        ++tracer.metrics.swaps;
        tracer.snapshot(values, "swap", {0, end});
        heapifyTrace(values, end, 0, tracer);
    }
}

void countingSortTrace(std::vector<int>& values, SortTracer& tracer) {
    if (values.empty()) {
        return;
    }
    const auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
    const int minValue = *minIt;
    const int maxValue = *maxIt;
    const int offset = minValue < 0 ? -minValue : 0;
    const int bucketCount = maxValue + offset + 1;

    std::vector<int> counts(static_cast<std::size_t>(bucketCount), 0);
    for (const int value : values) {
        ++counts[static_cast<std::size_t>(value + offset)];
        ++tracer.metrics.swaps;
        tracer.snapshot(values, "count", {value + offset});
    }

    for (std::size_t i = 1; i < counts.size(); ++i) {
        counts[i] += counts[i - 1];
    }

    std::vector<int> output(values.size());
    for (int index = static_cast<int>(values.size()) - 1; index >= 0; --index) {
        const int bucket = values[static_cast<std::size_t>(index)] + offset;
        const int targetIndex = --counts[static_cast<std::size_t>(bucket)];
        output[static_cast<std::size_t>(targetIndex)] = values[static_cast<std::size_t>(index)];
        ++tracer.metrics.swaps;
        tracer.snapshot(output, "write", {targetIndex});
    }
    values = std::move(output);
}

void radixCountingPass(std::vector<int>& values, int exponent, int shift, SortTracer& tracer) {
    std::vector<int> output(values.size());
    std::array<int, 10> counts{};
    for (const int value : values) {
        const int digit = ((value + shift) / exponent) % 10;
        ++counts[static_cast<std::size_t>(digit)];
        ++tracer.metrics.swaps;
        tracer.snapshot(unshiftValues(values, shift), "count", {digit});
    }
    for (std::size_t i = 1; i < counts.size(); ++i) {
        counts[i] += counts[i - 1];
    }
    for (int index = static_cast<int>(values.size()) - 1; index >= 0; --index) {
        const int digit = ((values[static_cast<std::size_t>(index)] + shift) / exponent) % 10;
        const int targetIndex = --counts[static_cast<std::size_t>(digit)];
        output[static_cast<std::size_t>(targetIndex)] = values[static_cast<std::size_t>(index)];
        ++tracer.metrics.swaps;
        tracer.snapshot(unshiftValues(output, shift), "write", {targetIndex});
    }
    values = std::move(output);
}

void radixSortTrace(std::vector<int>& values, SortTracer& tracer) {
    if (values.empty()) {
        return;
    }
    const auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
    const int minValue = *minIt;
    const int shift = minValue < 0 ? -minValue : 0;

    std::vector<int> shifted = values;
    for (int& value : shifted) {
        value += shift;
    }

    const int maxShifted = *std::max_element(shifted.begin(), shifted.end());
    for (int exponent = 1; maxShifted / exponent > 0; exponent *= 10) {
        radixCountingPass(shifted, exponent, shift, tracer);
    }

    values = unshiftValues(shifted, shift);
}

std::vector<int> flattenBuckets(const std::vector<std::vector<int>>& buckets) {
    std::vector<int> result;
    for (const auto& bucket : buckets) {
        result.insert(result.end(), bucket.begin(), bucket.end());
    }
    return result;
}

void bucketSortTrace(std::vector<int>& values, SortTracer& tracer) {
    if (values.empty()) {
        return;
    }
    const auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
    const int minValue = *minIt;
    const int maxValue = *maxIt;
    if (minValue == maxValue) {
        return;
    }

    const int bucketCount = std::max(4, static_cast<int>(std::sqrt(static_cast<double>(values.size()))));
    std::vector<std::vector<int>> buckets(static_cast<std::size_t>(bucketCount));

    const int range = maxValue - minValue + 1;
    for (const int value : values) {
        int bucketIndex = static_cast<int>((static_cast<long long>(value - minValue) * bucketCount) / range);
        if (bucketIndex >= bucketCount) {
            bucketIndex = bucketCount - 1;
        }
        buckets[static_cast<std::size_t>(bucketIndex)].push_back(value);
        ++tracer.metrics.swaps;
        tracer.snapshot(flattenBuckets(buckets), "scatter", {bucketIndex});
    }

    std::vector<int> rebuilt;
    rebuilt.reserve(values.size());
    std::size_t flattenedIndex = 0;
    for (std::size_t bucketIndex = 0; bucketIndex < buckets.size(); ++bucketIndex) {
        auto& bucket = buckets[bucketIndex];
        for (std::size_t i = 1; i < bucket.size(); ++i) {
            const int key = bucket[i];
            int j = static_cast<int>(i) - 1;
            while (j >= 0) {
                ++tracer.metrics.comparisons;
                tracer.snapshot(flattenBuckets(buckets), "compare", {static_cast<int>(flattenedIndex + static_cast<std::size_t>(j)), static_cast<int>(flattenedIndex + i)});
                if (bucket[static_cast<std::size_t>(j)] > key) {
                    bucket[static_cast<std::size_t>(j + 1)] = bucket[static_cast<std::size_t>(j)];
                    ++tracer.metrics.swaps;
                    tracer.snapshot(flattenBuckets(buckets), "swap", {static_cast<int>(flattenedIndex + static_cast<std::size_t>(j)), static_cast<int>(flattenedIndex + static_cast<std::size_t>(j + 1))});
                    --j;
                } else {
                    break;
                }
            }
            bucket[static_cast<std::size_t>(j + 1)] = key;
            ++tracer.metrics.swaps;
            tracer.snapshot(flattenBuckets(buckets), "write", {static_cast<int>(flattenedIndex + static_cast<std::size_t>(j + 1))});
        }
        for (int value : bucket) {
            rebuilt.push_back(value);
            ++tracer.metrics.swaps;
            tracer.snapshot(rebuilt, "write", {static_cast<int>(rebuilt.size() - 1)});
        }
        flattenedIndex += bucket.size();
    }
    values = std::move(rebuilt);
}

SortResult runSortTrace(const std::string& algorithm, const std::vector<int>& input) {
    std::vector<int> values = input;
    SortTracer tracer;
    tracer.snapshot(values, "start", {});

    const auto start = std::chrono::high_resolution_clock::now();

    const std::string normalized = toLower(algorithm);
    if (normalized == "merge_sort") {
        if (!values.empty()) {
            mergeSortTrace(values, 0, static_cast<int>(values.size()) - 1, tracer);
        }
    } else if (normalized == "quick_sort") {
        if (!values.empty()) {
            quickSortTrace(values, 0, static_cast<int>(values.size()) - 1, tracer);
        }
    } else if (normalized == "heap_sort") {
        heapSortTrace(values, tracer);
    } else if (normalized == "counting_sort") {
        countingSortTrace(values, tracer);
    } else if (normalized == "radix_sort") {
        radixSortTrace(values, tracer);
    } else if (normalized == "bucket_sort") {
        bucketSortTrace(values, tracer);
    }

    const auto end = std::chrono::high_resolution_clock::now();
    tracer.snapshot(values, "done", {});

    SortResult result;
    result.algorithm = algorithm;
    result.input = input;
    result.output = values;
    result.metrics = tracer.metrics;
    result.milliseconds = std::chrono::duration<double, std::milli>(end - start).count();
    result.steps = std::move(tracer.steps);
    result.stable = (normalized == "merge_sort" || normalized == "counting_sort" || normalized == "radix_sort" || normalized == "bucket_sort");
    return result;
}

std::string sortResultToJson(const SortResult& result) {
    std::ostringstream out;
    out << '{';
    out << "\"algorithm\":" << jsonString(result.algorithm) << ',';
    out << "\"stable\":" << (result.stable ? "true" : "false") << ',';
    out << "\"durationMs\":" << result.milliseconds << ',';
    out << "\"metrics\":{"
        << "\"comparisons\":" << result.metrics.comparisons << ','
        << "\"swaps\":" << result.metrics.swaps << "},";
    out << "\"input\":" << jsonArray(result.input) << ',';
    out << "\"output\":" << jsonArray(result.output) << ',';
    out << "\"steps\":" << jsonArray(result.steps);
    out << '}';
    return out.str();
}

std::string compareResultToJson(const SortResult& left, const SortResult& right) {
    const std::string winner = (left.milliseconds <= right.milliseconds) ? left.algorithm : right.algorithm;
    const std::ostringstream summary;
    std::ostringstream out;
    out << '{';
    out << "\"left\":" << sortResultToJson(left) << ',';
    out << "\"right\":" << sortResultToJson(right) << ',';
    out << "\"winner\":" << jsonString(winner) << ',';
    out << "\"summary\":" << jsonString((winner == left.algorithm ? left.algorithm : right.algorithm) + std::string(" performed better for this dataset")) << ',';
    out << "\"fasterMs\":" << std::min(left.milliseconds, right.milliseconds);
    out << '}';
    return out.str();
}

std::string readTextFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string guessContentType(const fs::path& path) {
    const std::string ext = toLower(path.extension().string());
    if (ext == ".html" || ext == ".htm") {
        return "text/html; charset=utf-8";
    }
    if (ext == ".css") {
        return "text/css; charset=utf-8";
    }
    if (ext == ".js") {
        return "application/javascript; charset=utf-8";
    }
    if (ext == ".json") {
        return "application/json; charset=utf-8";
    }
    if (ext == ".csv") {
        return "text/csv; charset=utf-8";
    }
    if (ext == ".svg") {
        return "image/svg+xml";
    }
    if (ext == ".png") {
        return "image/png";
    }
    if (ext == ".jpg" || ext == ".jpeg") {
        return "image/jpeg";
    }
    return "text/plain; charset=utf-8";
}

#ifdef _WIN32
std::string lastSocketError() {
    return "socket error " + std::to_string(WSAGetLastError());
}
#else
std::string lastSocketError() {
    return std::string("socket error");
}
#endif

bool sendAll(SocketHandle socket, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
#ifdef _WIN32
        const int result = send(socket, data.c_str() + sent, static_cast<int>(data.size() - sent), 0);
#else
        const ssize_t result = send(socket, data.c_str() + sent, data.size() - sent, 0);
#endif
        if (result <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(result);
    }
    return true;
}

struct HttpRequest {
    std::string method;
    std::string target;
    std::map<std::string, std::string> headers;
    std::string body;
};

std::string lowerHeaderKey(std::string key) {
    for (char& ch : key) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return key;
}

bool readHttpRequest(SocketHandle socket, HttpRequest& request) {
    std::string data;
    std::array<char, 4096> buffer{};
    std::size_t headerEnd = std::string::npos;

    while (headerEnd == std::string::npos) {
#ifdef _WIN32
        const int received = recv(socket, buffer.data(), static_cast<int>(buffer.size()), 0);
#else
        const ssize_t received = recv(socket, buffer.data(), buffer.size(), 0);
#endif
        if (received <= 0) {
            return false;
        }
        data.append(buffer.data(), received);
        headerEnd = data.find("\r\n\r\n");
        if (data.size() > 1024 * 1024) {
            return false;
        }
    }

    const std::string headersPart = data.substr(0, headerEnd);
    request.body = data.substr(headerEnd + 4);

    std::istringstream headerStream(headersPart);
    std::string line;
    if (!std::getline(headerStream, line)) {
        return false;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    {
        std::istringstream requestLine(line);
        requestLine >> request.method >> request.target;
    }

    while (std::getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        std::string key = lowerHeaderKey(trim(line.substr(0, colonPos)));
        std::string value = trim(line.substr(colonPos + 1));
        request.headers[key] = value;
    }

    const auto contentLengthIt = request.headers.find("content-length");
    if (contentLengthIt != request.headers.end()) {
        const std::size_t contentLength = static_cast<std::size_t>(std::stoul(contentLengthIt->second));
        while (request.body.size() < contentLength) {
#ifdef _WIN32
            const int received = recv(socket, buffer.data(), static_cast<int>(buffer.size()), 0);
#else
            const ssize_t received = recv(socket, buffer.data(), buffer.size(), 0);
#endif
            if (received <= 0) {
                return false;
            }
            request.body.append(buffer.data(), received);
        }
        request.body.resize(contentLength);
    }
    return true;
}

void sendHttpResponse(SocketHandle socket, const std::string& status, const std::string& contentType, const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Cache-Control: no-store, no-cache, must-revalidate\r\n";
    response << "Pragma: no-cache\r\n";
    response << "Expires: 0\r\n";
    response << "Connection: close\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type\r\n\r\n";
    response << body;
    sendAll(socket, response.str());
}

fs::path detectProjectRoot() {
#ifdef _WIN32
    std::array<char, 4096> buffer{};
    const DWORD length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    fs::path current = fs::path(std::string(buffer.data(), length)).parent_path();
#else
    std::array<char, 4096> buffer{};
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    fs::path current;
    if (length > 0) {
        buffer[static_cast<std::size_t>(length)] = '\0';
        current = fs::path(buffer.data()).parent_path();
    } else {
        current = fs::current_path();
    }
#endif
    for (int i = 0; i < 5; ++i) {
        if (fs::exists(current / "web" / "index.html")) {
            return current;
        }
        if (!current.has_parent_path()) {
            break;
        }
        current = current.parent_path();
    }
    return fs::current_path();
}

std::string makeErrorJson(const std::string& message) {
    return std::string("{\"error\":") + jsonString(message) + '}';
}

std::string healthJson() {
    return "{\"status\":\"ok\",\"service\":\"Smart Sorting Studio\"}";
}

std::string handleSortRequest(const std::string& body) {
    const auto algorithm = extractJsonString(body, "algorithm");
    const auto dataset = extractJsonIntArray(body, "dataset");
    if (!algorithm || !dataset) {
        return makeErrorJson("Invalid sort request payload");
    }
    return sortResultToJson(runSortTrace(*algorithm, *dataset));
}

std::string handleCompareRequest(const std::string& body) {
    const auto algorithm1 = extractJsonString(body, "algorithm1");
    const auto algorithm2 = extractJsonString(body, "algorithm2");
    const auto dataset = extractJsonIntArray(body, "dataset");
    if (!algorithm1 || !algorithm2 || !dataset) {
        return makeErrorJson("Invalid compare request payload");
    }
    const SortResult left = runSortTrace(*algorithm1, *dataset);
    const SortResult right = runSortTrace(*algorithm2, *dataset);
    return compareResultToJson(left, right);
}

std::string handleFileRequest(const fs::path& projectRoot, const std::string& requestTarget) {
    std::string path = requestTarget;
    if (path == "/") {
        path = "/web/index.html";
    }
    if (path.rfind("/assets", 0) == 0) {
        path = "/web" + path;
    }
    if (path.rfind("/data", 0) == 0) {
        path = path;
    } else if (path == "/index.html") {
        path = "/web/index.html";
    }

    const fs::path resolved = projectRoot / path.substr(1);
    if (!fs::exists(resolved) || fs::is_directory(resolved)) {
        return makeErrorJson("File not found");
    }
    return readTextFile(resolved);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::max(1, std::atoi(argv[1]));
    }

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize WinSock: " << lastSocketError() << '\n';
        return 1;
    }
#endif

    const fs::path projectRoot = detectProjectRoot();
    const fs::path webRoot = projectRoot / "web";
    std::cout << "Smart Sorting Studio backend running at http://localhost:" << port << '\n';
    std::cout << "Project root: " << projectRoot.string() << '\n';

    SocketHandle serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == invalidSocket) {
        std::cerr << "Unable to create server socket: " << lastSocketError() << '\n';
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int reuse = 1;
#ifdef _WIN32
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(static_cast<unsigned short>(port));

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        std::cerr << "Unable to bind server socket: " << lastSocketError() << '\n';
#ifdef _WIN32
        closesocket(serverSocket);
        WSACleanup();
#else
        close(serverSocket);
#endif
        return 1;
    }

    if (listen(serverSocket, 8) != 0) {
        std::cerr << "Unable to listen on server socket: " << lastSocketError() << '\n';
#ifdef _WIN32
        closesocket(serverSocket);
        WSACleanup();
#else
        close(serverSocket);
#endif
        return 1;
    }

    while (true) {
        sockaddr_in clientAddress{};
#ifdef _WIN32
        int clientLength = sizeof(clientAddress);
#else
        socklen_t clientLength = sizeof(clientAddress);
#endif
        SocketHandle clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientLength);
        if (clientSocket == invalidSocket) {
            continue;
        }

        HttpRequest request;
        if (!readHttpRequest(clientSocket, request)) {
#ifdef _WIN32
            closesocket(clientSocket);
#else
            close(clientSocket);
#endif
            continue;
        }

        if (request.method == "OPTIONS") {
            sendHttpResponse(clientSocket, "204 No Content", "text/plain; charset=utf-8", "");
        } else if (request.method == "GET" && request.target == "/api/health") {
            sendHttpResponse(clientSocket, "200 OK", "application/json; charset=utf-8", healthJson());
        } else if (request.method == "POST" && request.target == "/api/sort") {
            sendHttpResponse(clientSocket, "200 OK", "application/json; charset=utf-8", handleSortRequest(request.body));
        } else if (request.method == "POST" && request.target == "/api/compare") {
            sendHttpResponse(clientSocket, "200 OK", "application/json; charset=utf-8", handleCompareRequest(request.body));
        } else if (request.method == "GET") {
            std::string target = request.target;
            const std::size_t queryPos = target.find('?');
            if (queryPos != std::string::npos) {
                target = target.substr(0, queryPos);
            }
            if (target == "/") {
                target = "/web/index.html";
            }
            if (target == "/index.html") {
                target = "/web/index.html";
            }
            if (target.rfind("/data/", 0) == 0 || target.rfind("/web/", 0) == 0 || target == "/styles.css" || target == "/app.js") {
                fs::path filePath = projectRoot / target.substr(1);
                if (target == "/styles.css") {
                    filePath = webRoot / "styles.css";
                } else if (target == "/app.js") {
                    filePath = webRoot / "app.js";
                }
                if (fs::exists(filePath) && !fs::is_directory(filePath)) {
                    sendHttpResponse(clientSocket, "200 OK", guessContentType(filePath), readTextFile(filePath));
                } else {
                    sendHttpResponse(clientSocket, "404 Not Found", "application/json; charset=utf-8", makeErrorJson("File not found"));
                }
            } else {
                sendHttpResponse(clientSocket, "200 OK", "text/html; charset=utf-8", readTextFile(webRoot / "index.html"));
            }
        } else {
            sendHttpResponse(clientSocket, "405 Method Not Allowed", "application/json; charset=utf-8", makeErrorJson("Method not allowed"));
        }

#ifdef _WIN32
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif
    }

#ifdef _WIN32
    closesocket(serverSocket);
    WSACleanup();
#else
    close(serverSocket);
#endif
    return 0;
}
